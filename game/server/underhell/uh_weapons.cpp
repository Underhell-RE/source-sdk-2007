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
// Gun fire: a single shot through the engine bullet path, with Underhell
// recoil + accuracy.
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

	// Accumulate an accuracy penalty so spamming a gun spreads it out (the
	// vanilla pistol's model; the Underhell scripts carry the accuracy
	// multipliers, not the penalty curve).
	m_flAccuracyPenalty += 0.1f;
	m_flAccuracyPenalty = clamp( m_flAccuracyPenalty, 0.0f, 1.0f );
}

//-----------------------------------------------------------------------------
// Spread: base cone scaled by crouch / run / ironsight accuracy multipliers
// from the weapon script, plus a ramped penalty while spamming.
//-----------------------------------------------------------------------------
const Vector &CUHGunWeapon::GetBulletSpread( void )
{
	static Vector cone;
	static Vector baseCone = VECTOR_CONE_4DEGREES;

	// NPCs ignore the player tuning.
	if ( GetOwner() && GetOwner()->IsNPC() )
	{
		cone = baseCone;
		return cone;
	}

	const FileWeaponInfo_t &info = GetWpnData();

	// Spam penalty ramps the cone from ~1 to ~6 degrees.
	float ramp = RemapValClamped( m_flAccuracyPenalty, 0.0f, 1.0f, 0.0f, 1.0f );
	VectorLerp( VECTOR_CONE_1DEGREES, VECTOR_CONE_6DEGREES, ramp, cone );

	// Crouching is more accurate (lower value = tighter cone).
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( pPlayer )
	{
		if ( pPlayer->GetFlags() & FL_DUCKING )
		{
			cone *= info.m_flCrouchAccuracyMult;
		}

		// Moving fast spreads the cone (RunAccuracyMult > 1).
		float flSpeed = pPlayer->GetAbsVelocity().Length2D();
		if ( flSpeed > 150.0f )
		{
			float flRunRamp = RemapValClamped( flSpeed, 150.0f, 400.0f, 1.0f, info.m_flRunAccuracyMult );
			cone *= flRunRamp;
		}
	}

	// Ironsighted: apply the ExpOffset accuracy multiplier (usually < 1).
	CHL2_Player *pHL2Player = ToBasePlayer( GetOwner() ) ? dynamic_cast<CHL2_Player *>( GetOwner() ) : NULL;
	if ( pHL2Player && pHL2Player->UH_IsIronSighted() && info.m_bHasExpOffset )
	{
		cone *= info.m_flAccuracy;
	}

	return cone;
}

//-----------------------------------------------------------------------------
// Recoil: PunchPitch/PunchYaw ranges from the weapon script, scaled by
// CrouchRecoilMult while ducked.
//-----------------------------------------------------------------------------
void CUHGunWeapon::AddViewKick( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
		return;

	const FileWeaponInfo_t &info = GetWpnData();

	QAngle viewPunch;
	viewPunch.x = random->RandomFloat( info.m_flPunchPitchMin, info.m_flPunchPitchMax );
	viewPunch.y = random->RandomFloat( info.m_flPunchYawMin, info.m_flPunchYawMax );
	viewPunch.z = 0.0f;

	if ( pPlayer->GetFlags() & FL_DUCKING )
	{
		viewPunch *= info.m_flCrouchRecoilMult;
	}

	pPlayer->ViewPunch( viewPunch );
}

//-----------------------------------------------------------------------------
// Registers one weapon class under its entity name and its send table.
//-----------------------------------------------------------------------------
#define UH_IMPLEMENT_WEAPON( _className, _entityName, _shortName, _fireRate ) \
	IMPLEMENT_SERVERCLASS_ST( _className, DT_##_shortName ) \
	END_SEND_TABLE() \
	LINK_ENTITY_TO_CLASS( _entityName, _className ); \
	PRECACHE_WEAPON_REGISTER( _entityName ); \
	_className::_className() { m_flFireRate = _fireRate; m_flAccuracyPenalty = 0.0f; }

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
// Pistols (semi-auto; ~0.15s between shots)
//-----------------------------------------------------------------------------
UH_IMPLEMENT_WEAPON( CWeaponPistolGlock,	weapon_pistol_glock,		WeaponPistolGlock,		0.15f )
UH_IMPLEMENT_WEAPON( CWeaponPistolBeretta,	weapon_pistol_beretta,		WeaponPistolBeretta,	0.15f )
UH_IMPLEMENT_WEAPON( CWeaponPistolSocom,	weapon_pistol_socom,		WeaponPistolSocom,		0.18f )
UH_IMPLEMENT_WEAPON( CWeaponPython,			weapon_pistol_python,		WeaponPython,			0.3f )
UH_IMPLEMENT_WEAPON( CWeaponPistolDualies,	weapon_pistol_dualberetta,	WeaponPistolDualies,	0.12f )

//-----------------------------------------------------------------------------
// SMGs (full auto)
//-----------------------------------------------------------------------------
UH_IMPLEMENT_WEAPON( CWeaponSMGMP5,			weapon_smg_mp5,		WeaponSMGMP5,		0.08f )
UH_IMPLEMENT_WEAPON( CWeaponSMGMP5EOD,		weapon_smg_mp5_eod,	WeaponSMGMP5EOD,	0.08f )
UH_IMPLEMENT_WEAPON( CWeaponSMGMP7,			weapon_smg_mp7,		WeaponSMGMP7,		0.07f )

//-----------------------------------------------------------------------------
// Shotguns (pump)
//-----------------------------------------------------------------------------
UH_IMPLEMENT_WEAPON( CWeaponShotgunM3,		weapon_shotgun_m3,		WeaponShotgunM3,		0.9f )
UH_IMPLEMENT_WEAPON( CWeaponShotgunM5,		weapon_shotgun_m5,		WeaponShotgunM5,		0.9f )
UH_IMPLEMENT_WEAPON( CWeaponShotgunSpas12,	weapon_shotgun_spas12,	WeaponShotgunSpas12,	0.6f )
UH_IMPLEMENT_WEAPON( CWeaponShotgunXM1014,	weapon_shotgun_xm1014,	WeaponShotgunXM1014,	0.55f )

//-----------------------------------------------------------------------------
// Rifles
//-----------------------------------------------------------------------------
UH_IMPLEMENT_WEAPON( CWeaponG36K,			weapon_rifle_g36k,		WeaponG36K,		0.1f )
UH_IMPLEMENT_WEAPON( CWeaponSniper,			weapon_rifle_sniper,	WeaponSniper,	1.2f )

//-----------------------------------------------------------------------------
// BFG
//-----------------------------------------------------------------------------
UH_IMPLEMENT_WEAPON( CWeaponBfgMgl,			weapon_bfg_mgl,			WeaponBfgMgl,		0.5f )
UH_IMPLEMENT_WEAPON( CWeaponBfgMinigun,		weapon_bfg_minigun,		WeaponBfgMinigun,	0.07f )

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
