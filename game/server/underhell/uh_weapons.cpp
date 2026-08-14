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
// damage (skill.cfg sk_plr_dmg_*), recoil + accuracy. Mirrors the vanilla
// CBaseCombatWeapon::PrimaryAttack but injects the per-weapon damage the
// base leaves to the ammo definition.
//-----------------------------------------------------------------------------
void CUHGunWeapon::PrimaryAttack( void )
{
	// If the clip is empty (and we use clips), start a reload.
	if ( UsesClipsForAmmo1() && !m_iClip1 )
	{
		Reload();
		return;
	}

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
		return;

	// Prevent aim drift from rapid fire (same as the vanilla pistol).
	pPlayer->ViewPunchReset();

	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), SOUNDENT_VOLUME_PISTOL, 0.2, pPlayer );

	WeaponSound( m_bSilenced ? SINGLE_SILENCED : SINGLE );
	pPlayer->DoMuzzleFlash();
	SendWeaponAnim( GetPrimaryAttackActivity() );
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	// Build the shot with the per-weapon damage (the original reads the
	// sk_plr_dmg_<weapon> skill convar; the value is baked in per class).
	FireBulletsInfo_t info;
	info.m_vecSrc			= pPlayer->Weapon_ShootPosition();
	info.m_vecDirShooting	= pPlayer->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );
	info.m_vecSpread		= GetBulletSpread();
	info.m_flDistance		= MAX_TRACE_LENGTH;
	info.m_iAmmoType		= m_iPrimaryAmmoType;
	info.m_iTracerFreq		= 2;
	info.m_iShots			= 1;
	info.m_iDamage			= (int)GetDamage();
	info.m_iPlayerDamage	= (int)GetDamage();

	pPlayer->FireBullets( info );

	// Consume a round.
	if ( UsesClipsForAmmo1() )
	{
		m_iClip1--;
	}
	else
	{
		pPlayer->RemoveAmmo( 1, m_iPrimaryAmmoType );
	}

	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();

	// Out of ammo indicator.
	if ( !m_iClip1 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );
	}

	AddViewKick();

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
// Damage values are from the original's skill.cfg (sk_plr_dmg_*). Fire rates
// extracted from serveror.dll:
//   - pistols: 0.2 s (shared fire routine sub_1027AEC0)
//   - SMGs + BFG minigun: 0.075 s (GetFireRate override, vtable slot 277 ->
//     sub_102801F0 -> "flds 0x105300E4" = 0.075)
//   - G36K: 0.1 s (select-fire GetFireRate sub_103F5150 -> 0x1048775C = 0.1)
// Shotgun / sniper / BFG MGL use custom pump/delay fire paths (not GetFireRate);
// their values below are close estimates — TODO: recover the exact delay.
//-----------------------------------------------------------------------------
#define UH_IMPLEMENT_WEAPON( _className, _entityName, _shortName, _fireRate, _damage, _weaponType ) \
	IMPLEMENT_SERVERCLASS_ST( _className, DT_##_shortName ) \
	END_SEND_TABLE() \
	LINK_ENTITY_TO_CLASS( _entityName, _className ); \
	PRECACHE_WEAPON_REGISTER( _entityName ); \
	_className::_className() { m_flFireRate = _fireRate; m_iDamage = _damage; m_iWeaponType = _weaponType; m_bSilenced = false; m_flAccuracyPenalty = 0.0f; }

#define UH_IMPLEMENT_MELEE( _className, _entityName, _shortName, _damage ) \
	IMPLEMENT_SERVERCLASS_ST( _className, DT_##_shortName ) \
	END_SEND_TABLE() \
	LINK_ENTITY_TO_CLASS( _entityName, _className ); \
	PRECACHE_WEAPON_REGISTER( _entityName ); \
	_className::_className() { m_flMeleeDamage = _damage; }

//-----------------------------------------------------------------------------
// Melee — damage from skill.cfg: axe 35, baton 13, pipe 15, wrench 25, cleaver 50.
//-----------------------------------------------------------------------------
UH_IMPLEMENT_MELEE( CWeaponAxe,		weapon_melee_axe,		WeaponAxe,		35.0f )
UH_IMPLEMENT_MELEE( CWeaponBaton,		weapon_melee_baton,		WeaponBaton,	13.0f )
UH_IMPLEMENT_MELEE( CWeaponPipe,		weapon_melee_pipe,		WeaponPipe,		15.0f )
UH_IMPLEMENT_MELEE( CWeaponWrench,		weapon_melee_wrench,	WeaponWrench,	25.0f )
UH_IMPLEMENT_MELEE( CWeaponCleaver,		weapon_cleaver,			WeaponCleaver,	50.0f )

//-----------------------------------------------------------------------------
// Pistols — semi-auto, shared fire routine (0.2 s). Damage from skill.cfg.
// Weapon type 1 = pistol (silencer-gated on m_bHavePistolSilencer).
//-----------------------------------------------------------------------------
UH_IMPLEMENT_WEAPON( CWeaponPistolGlock,	weapon_pistol_glock,		WeaponPistolGlock,		0.2f, 10, 1 )
UH_IMPLEMENT_WEAPON( CWeaponPistolBeretta,	weapon_pistol_beretta,		WeaponPistolBeretta,	0.2f, 15, 1 )
UH_IMPLEMENT_WEAPON( CWeaponPistolSocom,	weapon_pistol_socom,		WeaponPistolSocom,		0.2f, 20, 1 )
UH_IMPLEMENT_WEAPON( CWeaponPython,			weapon_pistol_python,		WeaponPython,			0.5f, 120, 1 )
UH_IMPLEMENT_WEAPON( CWeaponPistolDualies,	weapon_pistol_dualberetta,	WeaponPistolDualies,	0.2f, 20, 1 )

//-----------------------------------------------------------------------------
// SMGs — full auto, 0.075 s (exact, GetFireRate). Damage: mp5 12, mp5_eod 10, mp7 8.
//-----------------------------------------------------------------------------
UH_IMPLEMENT_WEAPON( CWeaponSMGMP5,			weapon_smg_mp5,		WeaponSMGMP5,		0.075f, 12, 0 )
UH_IMPLEMENT_WEAPON( CWeaponSMGMP5EOD,		weapon_smg_mp5_eod,	WeaponSMGMP5EOD,	0.075f, 10, 0 )
UH_IMPLEMENT_WEAPON( CWeaponSMGMP7,			weapon_smg_mp7,		WeaponSMGMP7,		0.075f, 8, 0 )

//-----------------------------------------------------------------------------
// Shotguns — pump. Damage: m3 12, m5 16, spas12 14, xm1014 12.
//-----------------------------------------------------------------------------
UH_IMPLEMENT_WEAPON( CWeaponShotgunM3,		weapon_shotgun_m3,		WeaponShotgunM3,		0.75f, 12, 0 )
UH_IMPLEMENT_WEAPON( CWeaponShotgunM5,		weapon_shotgun_m5,		WeaponShotgunM5,		0.75f, 16, 0 )
UH_IMPLEMENT_WEAPON( CWeaponShotgunSpas12,	weapon_shotgun_spas12,	WeaponShotgunSpas12,	0.6f, 14, 0 )
UH_IMPLEMENT_WEAPON( CWeaponShotgunXM1014,	weapon_shotgun_xm1014,	WeaponShotgunXM1014,	0.55f, 12, 0 )

//-----------------------------------------------------------------------------
// Rifles — G36K is select-fire (0.1 s full-auto). Damage: g36k 20, sniper 80.
// Weapon type 4 = rifle (silencer-gated on m_bHaveRifleSilencer).
//-----------------------------------------------------------------------------
UH_IMPLEMENT_WEAPON( CWeaponG36K,			weapon_rifle_g36k,		WeaponG36K,		0.1f, 20, 4 )
UH_IMPLEMENT_WEAPON( CWeaponSniper,			weapon_rifle_sniper,	WeaponSniper,	1.5f, 80, 4 )

//-----------------------------------------------------------------------------
// BFG — mgl 200, minigun 50. Minigun is 0.075 s (exact, GetFireRate); MGL is an
// estimate (custom fire path).
//-----------------------------------------------------------------------------
UH_IMPLEMENT_WEAPON( CWeaponBfgMgl,			weapon_bfg_mgl,			WeaponBfgMgl,		1.0f, 200, 0 )
UH_IMPLEMENT_WEAPON( CWeaponBfgMinigun,		weapon_bfg_minigun,		WeaponBfgMinigun,	0.075f, 50, 0 )

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
