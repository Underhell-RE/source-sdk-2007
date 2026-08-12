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


// Player damage ConVars (names recovered from the decompiled binary).
ConVar sk_plr_dmg_smg_mp5( "sk_plr_dmg_smg_mp5", "8" );
ConVar sk_plr_dmg_smg_mp5_eod( "sk_plr_dmg_smg_mp5_eod", "9" );
ConVar sk_plr_dmg_smg_mp7( "sk_plr_dmg_smg_mp7", "7" );


#define UH_DEFINE_SMG_ACTTABLE( cls )								\
	acttable_t cls::m_acttable[] =									\
	{																\
		{ ACT_RANGE_ATTACK1,	ACT_RANGE_ATTACK_SMG1,	true },		\
		{ ACT_RELOAD,			ACT_RELOAD_SMG1,		true },		\
		{ ACT_IDLE,				ACT_IDLE_SMG1,			true },		\
		{ ACT_IDLE_ANGRY,		ACT_IDLE_ANGRY_SMG1,	true },		\
		{ ACT_WALK,				ACT_WALK_RIFLE,			true },		\
		{ ACT_WALK_AIM,			ACT_WALK_AIM_RIFLE,		true },		\
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
