//========= Copyright 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Underhell pistols (Episodic build).
//
//  Decompiled classes (klaxons1/underhell-hexrays): CWeaponPistolGlock /
//  CWeaponPistolBeretta / CWeaponPistolSocom / CWeaponPython /
//  CWeaponPistolDualies, all derived from CBaseHLCombatWeapon.
//
//=============================================================================//

#include "cbase.h"
#include "uh_basefirearm.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


// Player damage ConVars (names recovered from the decompiled binary; defaults
// are tunable game-config values). NPC damage falls back to the ammo definition.
ConVar sk_plr_dmg_pistol_glock( "sk_plr_dmg_pistol_glock", "8" );
ConVar sk_plr_dmg_pistol_beretta( "sk_plr_dmg_pistol_beretta", "9" );
ConVar sk_plr_dmg_pistol_socom( "sk_plr_dmg_pistol_socom", "12" );
ConVar sk_plr_dmg_pistol_python( "sk_plr_dmg_pistol_python", "40" );
ConVar sk_plr_dmg_pistol_dualberetta( "sk_plr_dmg_pistol_dualberetta", "9" );


// Pistol NPC activity table (identical for all Underhell pistols).
#define UH_DEFINE_PISTOL_ACTTABLE( cls )								\
	acttable_t cls::m_acttable[] =										\
	{																	\
		{ ACT_IDLE,						ACT_IDLE_PISTOL,				true },	\
		{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_PISTOL,			true },	\
		{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_PISTOL,		true },	\
		{ ACT_RELOAD,					ACT_RELOAD_PISTOL,				true },	\
		{ ACT_WALK_AIM,					ACT_WALK_AIM_PISTOL,			true },	\
		{ ACT_RUN_AIM,					ACT_RUN_AIM_PISTOL,				true },	\
		{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_PISTOL,true },	\
		{ ACT_RELOAD_LOW,				ACT_RELOAD_PISTOL_LOW,			false },	\
		{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_PISTOL_LOW,	false },	\
		{ ACT_COVER_LOW,				ACT_COVER_PISTOL_LOW,			false },	\
		{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_PISTOL_LOW,		false },	\
		{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_PISTOL,		false },	\
		{ ACT_WALK,						ACT_WALK_PISTOL,				false },	\
		{ ACT_RUN,						ACT_RUN_PISTOL,					false },	\
	};																	\
	IMPLEMENT_ACTTABLE( cls )


//-----------------------------------------------------------------------------
// Weapon: Glock-17 (weapon_pistol_glock)
//-----------------------------------------------------------------------------
class CWeaponPistolGlock : public CUhFirearmWeapon
{
	DECLARE_CLASS( CWeaponPistolGlock, CUhFirearmWeapon );
public:
	DECLARE_SERVERCLASS();
	DECLARE_ACTTABLE();

	virtual float GetPlayerDamage( void ) { return sk_plr_dmg_pistol_glock.GetFloat(); }
	virtual float GetFireRate( void ) { return 0.15f; }

	virtual const Vector &GetBulletSpread( void )
	{
		static Vector npcCone = VECTOR_CONE_5DEGREES;
		if ( GetOwner() && GetOwner()->IsNPC() )
			return npcCone;
		static Vector cone = VECTOR_CONE_1DEGREES;
		return cone;
	}
};

IMPLEMENT_SERVERCLASS_ST( CWeaponPistolGlock, DT_WeaponPistolGlock )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_pistol_glock, CWeaponPistolGlock );
PRECACHE_WEAPON_REGISTER( weapon_pistol_glock );

UH_DEFINE_PISTOL_ACTTABLE( CWeaponPistolGlock );


//-----------------------------------------------------------------------------
// Weapon: Beretta (weapon_pistol_beretta)
//-----------------------------------------------------------------------------
class CWeaponPistolBeretta : public CUhFirearmWeapon
{
	DECLARE_CLASS( CWeaponPistolBeretta, CUhFirearmWeapon );
public:
	DECLARE_SERVERCLASS();
	DECLARE_ACTTABLE();

	virtual float GetPlayerDamage( void ) { return sk_plr_dmg_pistol_beretta.GetFloat(); }
	virtual float GetFireRate( void ) { return 0.15f; }

	virtual const Vector &GetBulletSpread( void )
	{
		static Vector npcCone = VECTOR_CONE_5DEGREES;
		if ( GetOwner() && GetOwner()->IsNPC() )
			return npcCone;
		static Vector cone = VECTOR_CONE_1DEGREES;
		return cone;
	}
};

IMPLEMENT_SERVERCLASS_ST( CWeaponPistolBeretta, DT_WeaponPistolBeretta )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_pistol_beretta, CWeaponPistolBeretta );
PRECACHE_WEAPON_REGISTER( weapon_pistol_beretta );

UH_DEFINE_PISTOL_ACTTABLE( CWeaponPistolBeretta );


//-----------------------------------------------------------------------------
// Weapon: SOCOM (weapon_pistol_socom)
//-----------------------------------------------------------------------------
class CWeaponPistolSocom : public CUhFirearmWeapon
{
	DECLARE_CLASS( CWeaponPistolSocom, CUhFirearmWeapon );
public:
	DECLARE_SERVERCLASS();
	DECLARE_ACTTABLE();

	virtual float GetPlayerDamage( void ) { return sk_plr_dmg_pistol_socom.GetFloat(); }
	virtual float GetFireRate( void ) { return 0.2f; }

	virtual const Vector &GetBulletSpread( void )
	{
		static Vector npcCone = VECTOR_CONE_5DEGREES;
		if ( GetOwner() && GetOwner()->IsNPC() )
			return npcCone;
		static Vector cone = VECTOR_CONE_2DEGREES;
		return cone;
	}
};

IMPLEMENT_SERVERCLASS_ST( CWeaponPistolSocom, DT_WeaponPistolSocom )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_pistol_socom, CWeaponPistolSocom );
PRECACHE_WEAPON_REGISTER( weapon_pistol_socom );

UH_DEFINE_PISTOL_ACTTABLE( CWeaponPistolSocom );


//-----------------------------------------------------------------------------
// Weapon: Python revolver (weapon_pistol_python)
//-----------------------------------------------------------------------------
class CWeaponPython : public CUhFirearmWeapon
{
	DECLARE_CLASS( CWeaponPython, CUhFirearmWeapon );
public:
	DECLARE_SERVERCLASS();
	DECLARE_ACTTABLE();

	virtual float GetPlayerDamage( void ) { return sk_plr_dmg_pistol_python.GetFloat(); }
	virtual float GetFireRate( void ) { return 0.75f; }

	virtual const Vector &GetBulletSpread( void )
	{
		static Vector npcCone = VECTOR_CONE_5DEGREES;
		if ( GetOwner() && GetOwner()->IsNPC() )
			return npcCone;
		static Vector cone = VECTOR_CONE_1DEGREES;
		return cone;
	}
};

IMPLEMENT_SERVERCLASS_ST( CWeaponPython, DT_WeaponPython )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_pistol_python, CWeaponPython );
PRECACHE_WEAPON_REGISTER( weapon_pistol_python );

UH_DEFINE_PISTOL_ACTTABLE( CWeaponPython );


//-----------------------------------------------------------------------------
// Weapon: Dual Berettas (weapon_pistol_dualberetta)
//-----------------------------------------------------------------------------
class CWeaponPistolDualies : public CUhFirearmWeapon
{
	DECLARE_CLASS( CWeaponPistolDualies, CUhFirearmWeapon );
public:
	DECLARE_SERVERCLASS();
	DECLARE_ACTTABLE();

	virtual float GetPlayerDamage( void ) { return sk_plr_dmg_pistol_dualberetta.GetFloat(); }
	virtual float GetFireRate( void ) { return 0.08f; }

	virtual const Vector &GetBulletSpread( void )
	{
		static Vector npcCone = VECTOR_CONE_5DEGREES;
		if ( GetOwner() && GetOwner()->IsNPC() )
			return npcCone;
		static Vector cone = VECTOR_CONE_3DEGREES;
		return cone;
	}
};

IMPLEMENT_SERVERCLASS_ST( CWeaponPistolDualies, DT_WeaponPistolDualies )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_pistol_dualberetta, CWeaponPistolDualies );
PRECACHE_WEAPON_REGISTER( weapon_pistol_dualberetta );

UH_DEFINE_PISTOL_ACTTABLE( CWeaponPistolDualies );
