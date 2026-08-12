//========= Copyright 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Underhell rifles (Episodic build).
//
//  Decompiled classes (klaxons1/underhell-hexrays): CWeaponG36K / CWeaponSniper,
//  both derived from CBaseHLCombatWeapon.
//
//=============================================================================//

#include "cbase.h"
#include "uh_basefirearm.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


// Player damage ConVars (names recovered from the decompiled binary).
ConVar sk_plr_dmg_rifle_g36k( "sk_plr_dmg_rifle_g36k", "10" );
ConVar sk_plr_dmg_rifle_sniper( "sk_plr_dmg_rifle_sniper", "100" );


// Full vanilla AR2 activity table (readiness states included, otherwise NPCs
// fall back to T-pose when they are not actively fighting).
#define UH_DEFINE_RIFLE_ACTTABLE( cls )								\
	acttable_t cls::m_acttable[] =									\
	{																\
		{ ACT_RANGE_ATTACK1,	ACT_RANGE_ATTACK_AR2,	true },		\
		{ ACT_RELOAD,			ACT_RELOAD_SMG1,		true },		\
		{ ACT_IDLE,				ACT_IDLE_SMG1,			true },		\
		{ ACT_IDLE_ANGRY,		ACT_IDLE_ANGRY_SMG1,	true },		\
		{ ACT_WALK,				ACT_WALK_RIFLE,			true },		\
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
		{ ACT_WALK_AIM,				ACT_WALK_AIM_RIFLE,			true },		\
		{ ACT_WALK_CROUCH,			ACT_WALK_CROUCH_RIFLE,		true },		\
		{ ACT_WALK_CROUCH_AIM,		ACT_WALK_CROUCH_AIM_RIFLE,	true },		\
		{ ACT_RUN,					ACT_RUN_RIFLE,				true },		\
		{ ACT_RUN_AIM,				ACT_RUN_AIM_RIFLE,			true },		\
		{ ACT_RUN_CROUCH,			ACT_RUN_CROUCH_RIFLE,		true },		\
		{ ACT_RUN_CROUCH_AIM,		ACT_RUN_CROUCH_AIM_RIFLE,	true },		\
		{ ACT_GESTURE_RANGE_ATTACK1,ACT_GESTURE_RANGE_ATTACK_AR2,	false },	\
		{ ACT_COVER_LOW,			ACT_COVER_SMG1_LOW,			false },	\
		{ ACT_RANGE_AIM_LOW,		ACT_RANGE_AIM_AR2_LOW,		false },	\
		{ ACT_RANGE_ATTACK1_LOW,	ACT_RANGE_ATTACK_SMG1_LOW,	true },		\
		{ ACT_RELOAD_LOW,			ACT_RELOAD_SMG1_LOW,		false },	\
		{ ACT_GESTURE_RELOAD,		ACT_GESTURE_RELOAD_SMG1,	true },		\
	};																\
	IMPLEMENT_ACTTABLE( cls )


//-----------------------------------------------------------------------------
// Weapon: G36K (weapon_rifle_g36k)
//-----------------------------------------------------------------------------
class CWeaponG36K : public CUhFirearmWeapon
{
	DECLARE_CLASS( CWeaponG36K, CUhFirearmWeapon );
public:
	DECLARE_SERVERCLASS();
	DECLARE_ACTTABLE();

	virtual float GetPlayerDamage( void ) { return sk_plr_dmg_rifle_g36k.GetFloat(); }
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

IMPLEMENT_SERVERCLASS_ST( CWeaponG36K, DT_WeaponG36K )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_rifle_g36k, CWeaponG36K );
PRECACHE_WEAPON_REGISTER( weapon_rifle_g36k );

UH_DEFINE_RIFLE_ACTTABLE( CWeaponG36K );


//-----------------------------------------------------------------------------
// Weapon: Sniper rifle (weapon_rifle_sniper)
//-----------------------------------------------------------------------------
class CWeaponSniper : public CUhFirearmWeapon
{
	DECLARE_CLASS( CWeaponSniper, CUhFirearmWeapon );
public:
	DECLARE_SERVERCLASS();
	DECLARE_ACTTABLE();

	virtual float GetPlayerDamage( void ) { return sk_plr_dmg_rifle_sniper.GetFloat(); }
	virtual float GetFireRate( void ) { return 1.5f; }

	virtual const Vector &GetBulletSpread( void )
	{
		static Vector npcCone = VECTOR_CONE_3DEGREES;
		if ( GetOwner() && GetOwner()->IsNPC() )
			return npcCone;
		static Vector cone = VECTOR_CONE_1DEGREES;
		return cone;
	}
};

IMPLEMENT_SERVERCLASS_ST( CWeaponSniper, DT_WeaponSniper )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_rifle_sniper, CWeaponSniper );
PRECACHE_WEAPON_REGISTER( weapon_rifle_sniper );

UH_DEFINE_RIFLE_ACTTABLE( CWeaponSniper );
