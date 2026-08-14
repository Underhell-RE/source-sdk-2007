//========= Copyright (c) 2008, Mxthe (Underhell). All rights reserved. ============//
//
// Purpose: Underhell weapon classes — implementation.
//
// Each concrete weapon is a thin class: the weapon script provides the
// viewmodel / worldmodel / bucket / ammo, and the C++ class only registers the
// classname. Melee derives from CBaseHLBludgeonWeapon (swing + hit), guns from
// CBaseHLCombatWeapon (bullet fire via GetBulletSpread).
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "player.h"
#include "ammodef.h"
#include "in_buttons.h"
#include "soundent.h"
#include "hl2_player.h"
#include "uh_weapons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Melee swing: drain the weapon's StaminaToDrain from suit power, then swing.
//-----------------------------------------------------------------------------
void CUHMeleeWeapon::PrimaryAttack( void )
{
	CHL2_Player *pPlayer = ToBasePlayer( GetOwner() ) ? dynamic_cast<CHL2_Player *>( GetOwner() ) : NULL;
	if ( pPlayer )
	{
		// TODO: gate the swing on having enough stamina (original plays
		// "HL2Player.SprintNoPower" / a deny sound when drained).
		pPlayer->SuitPower_Drain( GetWpnData().m_flStaminaToDrain );
	}

	BaseClass::PrimaryAttack();
}

//-----------------------------------------------------------------------------
// Gun fire: a single shot through the engine bullet path.
//-----------------------------------------------------------------------------
void CUHGunWeapon::PrimaryAttack( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( pPlayer )
	{
		// Prevent aim drift from rapid fire (same as the vanilla pistol).
		pPlayer->ViewPunchReset();
	}

	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), SOUNDENT_VOLUME_PISTOL, 0.2, GetOwner() );

	BaseClass::PrimaryAttack();
}

//-----------------------------------------------------------------------------
// Registers one weapon class under its entity name and its send table.
//-----------------------------------------------------------------------------
#define UH_IMPLEMENT_WEAPON( _className, _entityName, _shortName ) \
	IMPLEMENT_SERVERCLASS_ST( _className, DT_##_shortName ) \
	END_SEND_TABLE() \
	LINK_ENTITY_TO_CLASS( _entityName, _className ); \
	PRECACHE_WEAPON_REGISTER( _entityName ); \
	_className::_className() {}

#define UH_IMPLEMENT_MELEE( _className, _entityName, _shortName, _damage ) \
	IMPLEMENT_SERVERCLASS_ST( _className, DT_##_shortName ) \
	END_SEND_TABLE() \
	LINK_ENTITY_TO_CLASS( _entityName, _className ); \
	PRECACHE_WEAPON_REGISTER( _entityName ); \
	_className::_className() { m_flMeleeDamage = _damage; }

//-----------------------------------------------------------------------------
// Melee
//-----------------------------------------------------------------------------
UH_IMPLEMENT_MELEE( CWeaponAxe,		weapon_melee_axe,		WeaponAxe,		40.0f )
UH_IMPLEMENT_MELEE( CWeaponBaton,		weapon_melee_baton,		WeaponBaton,	30.0f )
UH_IMPLEMENT_MELEE( CWeaponPipe,		weapon_melee_pipe,		WeaponPipe,		35.0f )
UH_IMPLEMENT_MELEE( CWeaponWrench,		weapon_melee_wrench,	WeaponWrench,	35.0f )
UH_IMPLEMENT_MELEE( CWeaponCleaver,		weapon_cleaver,			WeaponCleaver,	40.0f )

//-----------------------------------------------------------------------------
// Pistols
//-----------------------------------------------------------------------------
UH_IMPLEMENT_WEAPON( CWeaponPistolGlock,	weapon_pistol_glock,		WeaponPistolGlock )
UH_IMPLEMENT_WEAPON( CWeaponPistolBeretta,	weapon_pistol_beretta,		WeaponPistolBeretta )
UH_IMPLEMENT_WEAPON( CWeaponPistolSocom,	weapon_pistol_socom,		WeaponPistolSocom )
UH_IMPLEMENT_WEAPON( CWeaponPython,			weapon_pistol_python,		WeaponPython )
UH_IMPLEMENT_WEAPON( CWeaponPistolDualies,	weapon_pistol_dualberetta,	WeaponPistolDualies )

//-----------------------------------------------------------------------------
// SMGs
//-----------------------------------------------------------------------------
UH_IMPLEMENT_WEAPON( CWeaponSMGMP5,			weapon_smg_mp5,		WeaponSMGMP5 )
UH_IMPLEMENT_WEAPON( CWeaponSMGMP5EOD,		weapon_smg_mp5_eod,	WeaponSMGMP5EOD )
UH_IMPLEMENT_WEAPON( CWeaponSMGMP7,			weapon_smg_mp7,		WeaponSMGMP7 )

//-----------------------------------------------------------------------------
// Shotguns
//-----------------------------------------------------------------------------
UH_IMPLEMENT_WEAPON( CWeaponShotgunM3,		weapon_shotgun_m3,		WeaponShotgunM3 )
UH_IMPLEMENT_WEAPON( CWeaponShotgunM5,		weapon_shotgun_m5,		WeaponShotgunM5 )
UH_IMPLEMENT_WEAPON( CWeaponShotgunSpas12,	weapon_shotgun_spas12,	WeaponShotgunSpas12 )
UH_IMPLEMENT_WEAPON( CWeaponShotgunXM1014,	weapon_shotgun_xm1014,	WeaponShotgunXM1014 )

//-----------------------------------------------------------------------------
// Rifles
//-----------------------------------------------------------------------------
UH_IMPLEMENT_WEAPON( CWeaponG36K,			weapon_rifle_g36k,		WeaponG36K )
UH_IMPLEMENT_WEAPON( CWeaponSniper,			weapon_rifle_sniper,	WeaponSniper )

//-----------------------------------------------------------------------------
// BFG
//-----------------------------------------------------------------------------
UH_IMPLEMENT_WEAPON( CWeaponBfgMgl,			weapon_bfg_mgl,			WeaponBfgMgl )
UH_IMPLEMENT_WEAPON( CWeaponBfgMinigun,		weapon_bfg_minigun,		WeaponBfgMinigun )

//-----------------------------------------------------------------------------
// Purpose: Give every Underhell weapon (used by impulse 101). Each weapon is
// given the standard way so the script-derived clip/ammo apply.
//-----------------------------------------------------------------------------
static const char *s_UHAllWeapons[] =
{
	"weapon_melee_axe",
	"weapon_melee_baton",
	"weapon_melee_pipe",
	"weapon_melee_wrench",
	"weapon_cleaver",
	"weapon_pistol_glock",
	"weapon_pistol_beretta",
	"weapon_pistol_socom",
	"weapon_pistol_python",
	"weapon_pistol_dualberetta",
	"weapon_smg_mp5",
	"weapon_smg_mp5_eod",
	"weapon_smg_mp7",
	"weapon_shotgun_m3",
	"weapon_shotgun_m5",
	"weapon_shotgun_spas12",
	"weapon_shotgun_xm1014",
	"weapon_rifle_g36k",
	"weapon_rifle_sniper",
	"weapon_bfg_mgl",
	"weapon_bfg_minigun",
};

void UH_GiveAllWeapons( CBasePlayer *pPlayer )
{
	if ( !pPlayer )
		return;

	for ( int i = 0; i < ARRAYSIZE( s_UHAllWeapons ); i++ )
	{
		pPlayer->GiveNamedItem( s_UHAllWeapons[i] );
	}
}
