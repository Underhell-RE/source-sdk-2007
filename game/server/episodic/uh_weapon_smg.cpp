//========= Copyright 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Underhell submachine guns (Episodic build).
//
//  Decompiled classes (klaxons1/underhell-hexrays): CWeaponSMGMP5 /
//  CWeaponSMGMP5EOD / CWeaponSMGMP7, all derived from CBaseHLCombatWeapon.
//
//=============================================================================//

#include "cbase.h"
#include "uh_basefirearm.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


// Player damage ConVars (names recovered from the decompiled binary; defaults
// match the original DLL registrations - "0" falls back to the ammo definition).
ConVar sk_plr_dmg_smg_mp5( "sk_plr_dmg_smg_mp5", "0" );
ConVar sk_plr_dmg_smg_mp5_eod( "sk_plr_dmg_smg_mp5_eod", "0" );
ConVar sk_plr_dmg_smg_mp7( "sk_plr_dmg_smg_mp7", "0" );


// Full vanilla SMG1 activity table (readiness states included, otherwise NPCs
// fall back to T-pose when they are not actively fighting).
#define UH_DEFINE_SMG_ACTTABLE( cls )								\
	acttable_t cls::m_acttable[] =									\
	{																\
		{ ACT_RANGE_ATTACK1,	ACT_RANGE_ATTACK_SMG1,	true },		\
		{ ACT_RELOAD,			ACT_RELOAD_SMG1,		true },		\
		{ ACT_IDLE,				ACT_IDLE_SMG1,			true },		\
		{ ACT_IDLE_ANGRY,		ACT_IDLE_ANGRY_SMG1,	true },		\
		{ ACT_WALK,				ACT_WALK_RIFLE,			true },		\
		{ ACT_WALK_AIM,			ACT_WALK_AIM_RIFLE,		true },		\
		{ ACT_IDLE_RELAXED,		ACT_IDLE_SMG1_RELAXED,		false },	\
		{ ACT_IDLE_STIMULATED,	ACT_IDLE_SMG1_STIMULATED,	false },	\
		{ ACT_IDLE_AGITATED,	ACT_IDLE_ANGRY_SMG1,		false },	\
		{ ACT_WALK_RELAXED,		ACT_WALK_RIFLE_RELAXED,		false },	\
		{ ACT_WALK_STIMULATED,	ACT_WALK_RIFLE_STIMULATED,	false },	\
		{ ACT_WALK_AGITATED,	ACT_WALK_AIM_RIFLE,			false },	\
		{ ACT_RUN_RELAXED,		ACT_RUN_RIFLE_RELAXED,		false },	\
		{ ACT_RUN_STIMULATED,	ACT_RUN_RIFLE_STIMULATED,	false },	\
		{ ACT_RUN_AGITATED,		ACT_RUN_AIM_RIFLE,			false },	\
		{ ACT_IDLE_AIM_RELAXED,		ACT_IDLE_SMG1_RELAXED,		false },	\
		{ ACT_IDLE_AIM_STIMULATED,	ACT_IDLE_AIM_RIFLE_STIMULATED, false },	\
		{ ACT_IDLE_AIM_AGITATED,	ACT_IDLE_ANGRY_SMG1,		false },	\
		{ ACT_WALK_AIM_RELAXED,		ACT_WALK_RIFLE_RELAXED,		false },	\
		{ ACT_WALK_AIM_STIMULATED,	ACT_WALK_AIM_RIFLE_STIMULATED, false },	\
		{ ACT_WALK_AIM_AGITATED,	ACT_WALK_AIM_RIFLE,			false },	\
		{ ACT_RUN_AIM_RELAXED,		ACT_RUN_RIFLE_RELAXED,		false },	\
		{ ACT_RUN_AIM_STIMULATED,	ACT_RUN_AIM_RIFLE_STIMULATED, false },	\
		{ ACT_RUN_AIM_AGITATED,		ACT_RUN_AIM_RIFLE,			false },	\
		{ ACT_WALK_CROUCH,			ACT_WALK_CROUCH_RIFLE,		true },		\
		{ ACT_WALK_CROUCH_AIM,		ACT_WALK_CROUCH_AIM_RIFLE,	true },		\
		{ ACT_RUN,					ACT_RUN_RIFLE,				true },		\
		{ ACT_RUN_AIM,				ACT_RUN_AIM_RIFLE,			true },		\
		{ ACT_RUN_CROUCH,			ACT_RUN_CROUCH_RIFLE,		true },		\
		{ ACT_RUN_CROUCH_AIM,		ACT_RUN_CROUCH_AIM_RIFLE,	true },		\
		{ ACT_GESTURE_RANGE_ATTACK1,ACT_GESTURE_RANGE_ATTACK_SMG1,true },	\
		{ ACT_RANGE_ATTACK1_LOW,	ACT_RANGE_ATTACK_SMG1_LOW,	true },		\
		{ ACT_COVER_LOW,			ACT_COVER_SMG1_LOW,			false },	\
		{ ACT_RANGE_AIM_LOW,		ACT_RANGE_AIM_SMG1_LOW,		false },	\
		{ ACT_RELOAD_LOW,			ACT_RELOAD_SMG1_LOW,		false },	\
		{ ACT_GESTURE_RELOAD,		ACT_GESTURE_RELOAD_SMG1,	true },		\
	};																\
	IMPLEMENT_ACTTABLE( cls )


//-----------------------------------------------------------------------------
// Weapon: MP5 (weapon_smg_mp5)
//-----------------------------------------------------------------------------
class CWeaponSMGMP5 : public CUhFirearmWeapon
{
	DECLARE_CLASS( CWeaponSMGMP5, CUhFirearmWeapon );
public:
	DECLARE_SERVERCLASS();
	DECLARE_ACTTABLE();

	virtual float GetPlayerDamage( void ) { return sk_plr_dmg_smg_mp5.GetFloat(); }
	virtual float GetFireRate( void ) { return 0.1f; }

	virtual const Vector &GetBulletSpread( void )
	{
		static Vector npcCone = VECTOR_CONE_5DEGREES;
		if ( GetOwner() && GetOwner()->IsNPC() )
			return npcCone;
		static Vector cone = VECTOR_CONE_2DEGREES;
		return cone;
	}
};

IMPLEMENT_SERVERCLASS_ST( CWeaponSMGMP5, DT_WeaponSMGMP5 )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_smg_mp5, CWeaponSMGMP5 );
PRECACHE_WEAPON_REGISTER( weapon_smg_mp5 );

UH_DEFINE_SMG_ACTTABLE( CWeaponSMGMP5 );


//-----------------------------------------------------------------------------
// Weapon: MP5 EOD (weapon_smg_mp5_eod)
//-----------------------------------------------------------------------------
class CWeaponSMGMP5EOD : public CUhFirearmWeapon
{
	DECLARE_CLASS( CWeaponSMGMP5EOD, CUhFirearmWeapon );
public:
	DECLARE_SERVERCLASS();
	DECLARE_ACTTABLE();

	virtual float GetPlayerDamage( void ) { return sk_plr_dmg_smg_mp5_eod.GetFloat(); }
	virtual float GetFireRate( void ) { return 0.1f; }

	virtual const Vector &GetBulletSpread( void )
	{
		static Vector npcCone = VECTOR_CONE_5DEGREES;
		if ( GetOwner() && GetOwner()->IsNPC() )
			return npcCone;
		static Vector cone = VECTOR_CONE_2DEGREES;
		return cone;
	}
};

IMPLEMENT_SERVERCLASS_ST( CWeaponSMGMP5EOD, DT_WeaponSMGMP5EOD )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_smg_mp5_eod, CWeaponSMGMP5EOD );
PRECACHE_WEAPON_REGISTER( weapon_smg_mp5_eod );

UH_DEFINE_SMG_ACTTABLE( CWeaponSMGMP5EOD );


//-----------------------------------------------------------------------------
// Weapon: MP7 (weapon_smg_mp7)
//-----------------------------------------------------------------------------
class CWeaponSMGMP7 : public CUhFirearmWeapon
{
	DECLARE_CLASS( CWeaponSMGMP7, CUhFirearmWeapon );
public:
	DECLARE_SERVERCLASS();
	DECLARE_ACTTABLE();

	virtual float GetPlayerDamage( void ) { return sk_plr_dmg_smg_mp7.GetFloat(); }
	virtual float GetFireRate( void ) { return 0.09f; }

	virtual const Vector &GetBulletSpread( void )
	{
		static Vector npcCone = VECTOR_CONE_5DEGREES;
		if ( GetOwner() && GetOwner()->IsNPC() )
			return npcCone;
		static Vector cone = VECTOR_CONE_2DEGREES;
		return cone;
	}
};

IMPLEMENT_SERVERCLASS_ST( CWeaponSMGMP7, DT_WeaponSMGMP7 )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_smg_mp7, CWeaponSMGMP7 );
PRECACHE_WEAPON_REGISTER( weapon_smg_mp7 );

UH_DEFINE_SMG_ACTTABLE( CWeaponSMGMP7 );
