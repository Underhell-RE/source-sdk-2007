//========= Copyright 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Underhell shotguns (Episodic build).
//
//  Decompiled classes (klaxons1/underhell-hexrays): CWeaponShotgunM3 /
//  CWeaponShotgunM5 / CWeaponShotgunSpas12 / CWeaponShotgunXM1014, all derived
//  from CBaseHLCombatWeapon.
//
//=============================================================================//

#include "cbase.h"
#include "uh_basefirearm.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


extern ConVar sk_plr_num_shotgun_pellets;

// Player damage ConVars (names recovered from the decompiled binary; per-pellet
// damage). NPC damage falls back to the ammo definition.
ConVar sk_plr_dmg_shotgun_m3( "sk_plr_dmg_shotgun_m3", "8" );
ConVar sk_plr_dmg_shotgun_m5( "sk_plr_dmg_shotgun_m5", "9" );
ConVar sk_plr_dmg_shotgun_spas12( "sk_plr_dmg_shotgun_spas12", "7" );
ConVar sk_plr_dmg_shotgun_xm1014( "sk_plr_dmg_shotgun_xm1014", "7" );


//-----------------------------------------------------------------------------
// Underhell shotgun base: multi-pellet PrimaryAttack.
//-----------------------------------------------------------------------------
class CUhShotgunWeapon : public CUhFirearmWeapon
{
	DECLARE_CLASS( CUhShotgunWeapon, CUhFirearmWeapon );
public:
	CUhShotgunWeapon() {}

	virtual void PrimaryAttack( void );

	virtual int GetNumPellets( void ) { return sk_plr_num_shotgun_pellets.GetInt(); }

	virtual const Vector &GetBulletSpread( void )
	{
		static Vector npcCone = VECTOR_CONE_8DEGREES;
		if ( GetOwner() && GetOwner()->IsNPC() )
			return npcCone;
		static Vector cone = VECTOR_CONE_8DEGREES;
		return cone;
	}
};

//-----------------------------------------------------------------------------
void CUhShotgunWeapon::PrimaryAttack( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
		return;

	if ( m_iClip1 <= 0 )
	{
		if ( !m_bFireOnEmpty )
		{
			Reload();
		}
		else
		{
			WeaponSound( EMPTY );
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2f;
		}
		return;
	}

	WeaponSound( SINGLE );
	pPlayer->DoMuzzleFlash();

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	m_iClip1 -= 1;

	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector vecAiming = pPlayer->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );

	FireBulletsInfo_t info( GetNumPellets(), vecSrc, vecAiming, GetBulletSpread(), MAX_TRACE_LENGTH, m_iPrimaryAmmoType );
	FireBullets( info );

	AddViewKick();

	if ( !m_iClip1 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );
	}
}


// Full vanilla shotgun activity table (readiness states included, otherwise
// NPCs fall back to T-pose when they are not actively fighting).
#define UH_DEFINE_SHOTGUN_ACTTABLE( cls )							\
	acttable_t cls::m_acttable[] =									\
	{																\
		{ ACT_IDLE,					ACT_IDLE_SMG1,				true },	\
		{ ACT_RANGE_ATTACK1,		ACT_RANGE_ATTACK_SHOTGUN,	true },	\
		{ ACT_RELOAD,				ACT_RELOAD_SHOTGUN,			false },	\
		{ ACT_WALK,					ACT_WALK_RIFLE,				true },	\
		{ ACT_IDLE_ANGRY,			ACT_IDLE_ANGRY_SHOTGUN,		true },	\
		{ ACT_IDLE_RELAXED,			ACT_IDLE_SHOTGUN_RELAXED,	false },	\
		{ ACT_IDLE_STIMULATED,		ACT_IDLE_SHOTGUN_STIMULATED,false },	\
		{ ACT_IDLE_AGITATED,		ACT_IDLE_SHOTGUN_AGITATED,	false },	\
		{ ACT_WALK_RELAXED,			ACT_WALK_RIFLE_RELAXED,		false },	\
		{ ACT_WALK_STIMULATED,		ACT_WALK_RIFLE_STIMULATED,	false },	\
		{ ACT_WALK_AGITATED,		ACT_WALK_AIM_RIFLE,			false },	\
		{ ACT_RUN_RELAXED,			ACT_RUN_RIFLE_RELAXED,		false },	\
		{ ACT_RUN_STIMULATED,		ACT_RUN_RIFLE_STIMULATED,	false },	\
		{ ACT_RUN_AGITATED,			ACT_RUN_AIM_RIFLE,			false },	\
		{ ACT_IDLE_AIM_RELAXED,		ACT_IDLE_SMG1_RELAXED,		false },	\
		{ ACT_IDLE_AIM_STIMULATED,	ACT_IDLE_AIM_RIFLE_STIMULATED, false },	\
		{ ACT_IDLE_AIM_AGITATED,	ACT_IDLE_ANGRY_SMG1,		false },	\
		{ ACT_WALK_AIM_RELAXED,		ACT_WALK_RIFLE_RELAXED,		false },	\
		{ ACT_WALK_AIM_STIMULATED,	ACT_WALK_AIM_RIFLE_STIMULATED, false },	\
		{ ACT_WALK_AIM_AGITATED,	ACT_WALK_AIM_RIFLE,			false },	\
		{ ACT_RUN_AIM_RELAXED,		ACT_RUN_RIFLE_RELAXED,		false },	\
		{ ACT_RUN_AIM_STIMULATED,	ACT_RUN_AIM_RIFLE_STIMULATED, false },	\
		{ ACT_RUN_AIM_AGITATED,		ACT_RUN_AIM_RIFLE,			false },	\
		{ ACT_WALK_AIM,				ACT_WALK_AIM_SHOTGUN,		true },		\
		{ ACT_WALK_CROUCH,			ACT_WALK_CROUCH_RIFLE,		true },		\
		{ ACT_WALK_CROUCH_AIM,		ACT_WALK_CROUCH_AIM_RIFLE,	true },		\
		{ ACT_RUN,					ACT_RUN_RIFLE,				true },		\
		{ ACT_RUN_AIM,				ACT_RUN_AIM_SHOTGUN,		true },		\
		{ ACT_RUN_CROUCH,			ACT_RUN_CROUCH_RIFLE,		true },		\
		{ ACT_RUN_CROUCH_AIM,		ACT_RUN_CROUCH_AIM_RIFLE,	true },		\
		{ ACT_GESTURE_RANGE_ATTACK1,ACT_GESTURE_RANGE_ATTACK_SHOTGUN,true },	\
		{ ACT_RANGE_ATTACK1_LOW,	ACT_RANGE_ATTACK_SHOTGUN_LOW,true },	\
		{ ACT_RELOAD_LOW,			ACT_RELOAD_SHOTGUN_LOW,		false },	\
		{ ACT_GESTURE_RELOAD,		ACT_GESTURE_RELOAD_SHOTGUN,	false },	\
	};																\
	IMPLEMENT_ACTTABLE( cls )


//-----------------------------------------------------------------------------
// Weapon: M3 Super 90 (weapon_shotgun_m3)
//-----------------------------------------------------------------------------
class CWeaponShotgunM3 : public CUhShotgunWeapon
{
	DECLARE_CLASS( CWeaponShotgunM3, CUhShotgunWeapon );
public:
	DECLARE_SERVERCLASS();
	DECLARE_ACTTABLE();

	virtual float GetPlayerDamage( void ) { return sk_plr_dmg_shotgun_m3.GetFloat(); }
	virtual float GetFireRate( void ) { return 1.2f; }
};

IMPLEMENT_SERVERCLASS_ST( CWeaponShotgunM3, DT_WeaponShotgunM3 )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_shotgun_m3, CWeaponShotgunM3 );
PRECACHE_WEAPON_REGISTER( weapon_shotgun_m3 );

UH_DEFINE_SHOTGUN_ACTTABLE( CWeaponShotgunM3 );


//-----------------------------------------------------------------------------
// Weapon: M5 (weapon_shotgun_m5)
//-----------------------------------------------------------------------------
class CWeaponShotgunM5 : public CUhShotgunWeapon
{
	DECLARE_CLASS( CWeaponShotgunM5, CUhShotgunWeapon );
public:
	DECLARE_SERVERCLASS();
	DECLARE_ACTTABLE();

	virtual float GetPlayerDamage( void ) { return sk_plr_dmg_shotgun_m5.GetFloat(); }
	virtual float GetFireRate( void ) { return 1.0f; }
};

IMPLEMENT_SERVERCLASS_ST( CWeaponShotgunM5, DT_WeaponShotgunM5 )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_shotgun_m5, CWeaponShotgunM5 );
PRECACHE_WEAPON_REGISTER( weapon_shotgun_m5 );

UH_DEFINE_SHOTGUN_ACTTABLE( CWeaponShotgunM5 );


//-----------------------------------------------------------------------------
// Weapon: SPAS-12 (weapon_shotgun_spas12)
//-----------------------------------------------------------------------------
class CWeaponShotgunSpas12 : public CUhShotgunWeapon
{
	DECLARE_CLASS( CWeaponShotgunSpas12, CUhShotgunWeapon );
public:
	DECLARE_SERVERCLASS();
	DECLARE_ACTTABLE();

	virtual float GetPlayerDamage( void ) { return sk_plr_dmg_shotgun_spas12.GetFloat(); }
	virtual float GetFireRate( void ) { return 0.8f; }
};

IMPLEMENT_SERVERCLASS_ST( CWeaponShotgunSpas12, DT_WeaponShotgunSpas12 )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_shotgun_spas12, CWeaponShotgunSpas12 );
PRECACHE_WEAPON_REGISTER( weapon_shotgun_spas12 );

UH_DEFINE_SHOTGUN_ACTTABLE( CWeaponShotgunSpas12 );


//-----------------------------------------------------------------------------
// Weapon: XM1014 (weapon_shotgun_xm1014)
//-----------------------------------------------------------------------------
class CWeaponShotgunXM1014 : public CUhShotgunWeapon
{
	DECLARE_CLASS( CWeaponShotgunXM1014, CUhShotgunWeapon );
public:
	DECLARE_SERVERCLASS();
	DECLARE_ACTTABLE();

	virtual float GetPlayerDamage( void ) { return sk_plr_dmg_shotgun_xm1014.GetFloat(); }
	virtual float GetFireRate( void ) { return 0.7f; }
};

IMPLEMENT_SERVERCLASS_ST( CWeaponShotgunXM1014, DT_WeaponShotgunXM1014 )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_shotgun_xm1014, CWeaponShotgunXM1014 );
PRECACHE_WEAPON_REGISTER( weapon_shotgun_xm1014 );

UH_DEFINE_SHOTGUN_ACTTABLE( CWeaponShotgunXM1014 );
