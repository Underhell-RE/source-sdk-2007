//========= Copyright (c) 2008, Mxthe (Underhell). All rights reserved. ============//
//
// Purpose: Underhell weapon classes. Class names + buckets + models + ammo all
//          come from the mod's weapon scripts (scripts/weapon_*.txt), which the
//          SDK's weapon_parse reads automatically. Each class below only pins
//          the classname and the C++ derivation; the tuning (StaminaToDrain,
//          MeleeRange, PunchPitch, ExpOffset ironsight, ...) is read from the
//          script via GetWpnData().
//
// Original class list (serveror.dll RTTI + FGD/Weapon List.txt):
//   melee:  axe / baton / pipe / wrench / cleaver
//   pistol: glock / beretta / socom / python / dualberetta
//   smg:    mp5 / mp5_eod / mp7
//   shotgun: m3 / m5 / spas12 / xm1014
//   rifle:  g36k / sniper
//   bfg:    mgl / minigun
// Vanilla HL2 weapons (weapon_pistol / weapon_shotgun / weapon_smg1 /
// weapon_357 / weapon_rpg / weapon_crossbow / weapon_crowbar) stay vanilla.
//
// $NoKeywords: $
//=============================================================================//

#ifndef UH_WEAPONS_H
#define UH_WEAPONS_H
#ifdef _WIN32
#pragma once
#endif

#include "basecombatweapon.h"
#include "basehlcombatweapon.h"
#include "basebludgeonweapon.h"

//-----------------------------------------------------------------------------
// Shared melee base: reads MeleeRange / MeleeRoF / StaminaToDrain from the
// weapon script. Damage is per-class (values from the original skill.cfg's
// sk_plr_dmg_* entries — see uh_weapons.cpp).
//-----------------------------------------------------------------------------
class CUHMeleeWeapon : public CBaseHLBludgeonWeapon
{
	DECLARE_CLASS( CUHMeleeWeapon, CBaseHLBludgeonWeapon );

public:
	virtual void	PrimaryAttack( void );	// drains StaminaToDrain, then swings
	virtual float	GetDamageForActivity( Activity hitActivity ) { return m_flMeleeDamage; }
	virtual float	GetDamage( void ) { return m_flMeleeDamage; }
	virtual float	GetRange( void ) { return GetWpnData().m_flMeleeRange; }
	virtual float	GetFireRate( void ) { return GetWpnData().m_flMeleeRoF; }

	float			m_flMeleeDamage;
};

//-----------------------------------------------------------------------------
// Shared gun base: fires bullets through the engine's bullet path with the
// per-weapon damage (skill.cfg sk_plr_dmg_*). Recoil (PunchPitch/PunchYaw) and
// spread (CrouchAccuracyMult/RunAccuracyMult/ExpOffset accuracy) are read from
// the weapon script via GetWpnData().
//-----------------------------------------------------------------------------
class CUHGunWeapon : public CBaseHLCombatWeapon
{
	DECLARE_CLASS( CUHGunWeapon, CBaseHLCombatWeapon );

public:
	virtual void	PrimaryAttack( void );
	virtual float	GetDamage( void ) { return (float)m_iDamage; }
	virtual float	GetFireRate( void ) { return m_flFireRate; }
	virtual const Vector &GetBulletSpread( void );
	virtual void	AddViewKick( void );

	// Underhell silencer. "silencer_toggle" gates pistols (type 1) and rifles
	// (type 4) on the player carrying the matching silencer (m_bHavePistol/
	// RifleSilencer); everything else toggles freely (decode sub_101E2F50).
	virtual int		GetWeaponType( void ) { return m_iWeaponType; }
	bool			IsSilenced( void ) const { return m_bSilenced; }
	void			SetSilenced( bool bSilenced ) { m_bSilenced = bSilenced; }

	// Silenced viewmodel activities (the viewmodel model carries the
	// ACT_VM_*_SILENCED sequences; SendWeaponAnim falls back to idle if a
	// model lacks one).
	virtual Activity GetPrimaryAttackActivity( void );
	virtual Activity GetDrawActivity( void );

	float			m_flFireRate;			// seconds between shots
	int				m_iDamage;				// per-shot damage (skill.cfg sk_plr_dmg_*)
	int				m_iWeaponType;			// 1 = pistol, 4 = rifle, 0 = other (silencer gating)
	bool			m_bSilenced;			// silencer fitted (server-only; sound switches)
	float			m_flAccuracyPenalty;	// grows per shot, decays over time
};

//-----------------------------------------------------------------------------
// Declares one concrete weapon. The weapon script (scripts/weapon_<classname>
// .txt) supplies viewmodel, worldmodel, bucket, clip size and ammo. The short
// name (_shortName) is the C++ identifier used for the send/recv table, and it
// must match the client stub's class name in c_uh_weapons.cpp.
//-----------------------------------------------------------------------------
#define UH_DECLARE_WEAPON( _className, _shortName ) \
	class _className : public CUHGunWeapon \
	{ \
		DECLARE_CLASS( _className, CUHGunWeapon ); \
	public: \
		DECLARE_SERVERCLASS(); \
		_className(); \
	};

#define UH_DECLARE_MELEE( _className, _shortName, _damage ) \
	class _className : public CUHMeleeWeapon \
	{ \
		DECLARE_CLASS( _className, CUHMeleeWeapon ); \
	public: \
		DECLARE_SERVERCLASS(); \
		_className(); \
	};

//-----------------------------------------------------------------------------
// Melee
//-----------------------------------------------------------------------------
UH_DECLARE_MELEE( CWeaponAxe,		WeaponAxe,		40.0f )
UH_DECLARE_MELEE( CWeaponBaton,		WeaponBaton,	30.0f )
UH_DECLARE_MELEE( CWeaponPipe,		WeaponPipe,		35.0f )
UH_DECLARE_MELEE( CWeaponWrench,	WeaponWrench,	35.0f )
UH_DECLARE_MELEE( CWeaponCleaver,	WeaponCleaver,	40.0f )

//-----------------------------------------------------------------------------
// Pistols
//-----------------------------------------------------------------------------
UH_DECLARE_WEAPON( CWeaponPistolGlock,		WeaponPistolGlock )
UH_DECLARE_WEAPON( CWeaponPistolBeretta,	WeaponPistolBeretta )
UH_DECLARE_WEAPON( CWeaponPistolSocom,		WeaponPistolSocom )
UH_DECLARE_WEAPON( CWeaponPython,			WeaponPython )
UH_DECLARE_WEAPON( CWeaponPistolDualies,	WeaponPistolDualies )

//-----------------------------------------------------------------------------
// SMGs
//-----------------------------------------------------------------------------
UH_DECLARE_WEAPON( CWeaponSMGMP5,		WeaponSMGMP5 )
UH_DECLARE_WEAPON( CWeaponSMGMP5EOD,	WeaponSMGMP5EOD )
UH_DECLARE_WEAPON( CWeaponSMGMP7,		WeaponSMGMP7 )

//-----------------------------------------------------------------------------
// Shotguns
//-----------------------------------------------------------------------------
UH_DECLARE_WEAPON( CWeaponShotgunM3,		WeaponShotgunM3 )
UH_DECLARE_WEAPON( CWeaponShotgunM5,		WeaponShotgunM5 )
UH_DECLARE_WEAPON( CWeaponShotgunSpas12,	WeaponShotgunSpas12 )
UH_DECLARE_WEAPON( CWeaponShotgunXM1014,	WeaponShotgunXM1014 )

//-----------------------------------------------------------------------------
// Rifles
//-----------------------------------------------------------------------------
UH_DECLARE_WEAPON( CWeaponG36K,		WeaponG36K )
UH_DECLARE_WEAPON( CWeaponSniper,		WeaponSniper )

//-----------------------------------------------------------------------------
// BFG
//-----------------------------------------------------------------------------
UH_DECLARE_WEAPON( CWeaponBfgMgl,		WeaponBfgMgl )
UH_DECLARE_WEAPON( CWeaponBfgMinigun,	WeaponBfgMinigun )

// Helper used by the "give all weapons" cheat (impulse 101).
void UH_GiveAllWeapons( CBasePlayer *pPlayer );

#endif // UH_WEAPONS_H
