//========= Copyright 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Underhell BFG weapons (Episodic build).
//
//  Decompiled classes (klaxons1/underhell-hexrays): CWeaponBfgMgl /
//  CWeaponBfgMinigun, both derived from CBaseHLCombatWeapon.
//
//  NOTE: the MGL fires SMG1_Grenade ammo; the arcing-grenade projectile is a
//  follow-up (this stub fires hitscan for now).
//
//=============================================================================//

#include "cbase.h"
#include "uh_basefirearm.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


// Player damage ConVars (names recovered from the decompiled binary; defaults
// match the original DLL registrations - "0" falls back to the ammo definition).
ConVar sk_plr_dmg_bfg_mgl( "sk_plr_dmg_bfg_mgl", "0" );
ConVar sk_plr_dmg_bfg_minigun( "sk_plr_dmg_bfg_minigun", "0" );


// Full vanilla machinegun activity table (readiness states included).
#define UH_DEFINE_RIFLE_ACTTABLE( cls )								\
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
// Weapon: MGL grenade launcher (weapon_bfg_mgl)
//-----------------------------------------------------------------------------
class CWeaponBfgMgl : public CUhFirearmWeapon
{
	DECLARE_CLASS( CWeaponBfgMgl, CUhFirearmWeapon );
public:
	DECLARE_SERVERCLASS();
	DECLARE_ACTTABLE();

	virtual float GetPlayerDamage( void ) { return sk_plr_dmg_bfg_mgl.GetFloat(); }
	virtual float GetFireRate( void ) { return 0.8f; }

	virtual const Vector &GetBulletSpread( void )
	{
		static Vector cone = VECTOR_CONE_2DEGREES;
		return cone;
	}
};

IMPLEMENT_SERVERCLASS_ST( CWeaponBfgMgl, DT_WeaponBfgMgl )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_bfg_mgl, CWeaponBfgMgl );
PRECACHE_WEAPON_REGISTER( weapon_bfg_mgl );

UH_DEFINE_RIFLE_ACTTABLE( CWeaponBfgMgl );


//-----------------------------------------------------------------------------
// Weapon: Minigun (weapon_bfg_minigun)
//-----------------------------------------------------------------------------
class CWeaponBfgMinigun : public CUhFirearmWeapon
{
	DECLARE_CLASS( CWeaponBfgMinigun, CUhFirearmWeapon );
public:
	DECLARE_SERVERCLASS();
	DECLARE_ACTTABLE();

	virtual float GetPlayerDamage( void ) { return sk_plr_dmg_bfg_minigun.GetFloat(); }
	virtual float GetFireRate( void ) { return 0.06f; }

	virtual const Vector &GetBulletSpread( void )
	{
		static Vector npcCone = VECTOR_CONE_5DEGREES;
		if ( GetOwner() && GetOwner()->IsNPC() )
			return npcCone;
		static Vector cone = VECTOR_CONE_3DEGREES;
		return cone;
	}
};

IMPLEMENT_SERVERCLASS_ST( CWeaponBfgMinigun, DT_WeaponBfgMinigun )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_bfg_minigun, CWeaponBfgMinigun );
PRECACHE_WEAPON_REGISTER( weapon_bfg_minigun );

UH_DEFINE_RIFLE_ACTTABLE( CWeaponBfgMinigun );
