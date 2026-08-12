//========= Copyright 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Underhell melee weapons (Episodic build).
//
//  These derive from the HL2 bludgeon base and are data-driven:
//  - reach and rate of fire come from the weapon script (MeleeRange / MeleeRoF),
//    parsed by CUHWeaponInfo (see game/shared/episodic/uh_weapon_parse.*)
//  - damage comes from per-weapon ConVars (sk_plr_dmg_* / sk_npc_dmg_*)
//
//  Decompiled classes (klaxons1/underhell-hexrays): CWeaponBaton / CWeaponPipe /
//  CWeaponAxe / CWeaponWrench / CWeaponCleaver, all derived from CBaseHLBludgeonWeapon.
//
//=============================================================================//

#include "cbase.h"
#include "basebludgeonweapon.h"
#include "basehlcombatweapon.h"
#include "episodic/uh_weapon_parse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


// Damage ConVars (names recovered from the decompiled binary; defaults are
// tunable game-config values, exact Underhell defaults were not recoverable).
ConVar sk_plr_dmg_baton( "sk_plr_dmg_baton", "22" );
ConVar sk_npc_dmg_baton( "sk_npc_dmg_baton", "8" );

ConVar sk_plr_dmg_pipe( "sk_plr_dmg_pipe", "28" );
ConVar sk_npc_dmg_pipe( "sk_npc_dmg_pipe", "10" );

ConVar sk_plr_dmg_axe( "sk_plr_dmg_axe", "40" );
ConVar sk_npc_dmg_axe( "sk_npc_dmg_axe", "14" );

ConVar sk_plr_dmg_wrench( "sk_plr_dmg_wrench", "30" );
ConVar sk_npc_dmg_wrench( "sk_npc_dmg_wrench", "12" );

ConVar sk_plr_dmg_cleaver( "sk_plr_dmg_cleaver", "45" );
ConVar sk_npc_dmg_cleaver( "sk_npc_dmg_cleaver", "16" );


//-----------------------------------------------------------------------------
// Underhell melee base: bludgeon behavior driven by the weapon script.
//-----------------------------------------------------------------------------
class CUHMeleeWeapon : public CBaseHLBludgeonWeapon
{
	DECLARE_CLASS( CUHMeleeWeapon, CBaseHLBludgeonWeapon );
public:
	CUHMeleeWeapon() {}

	virtual void	Spawn( void );

	virtual float	GetRange( void )		{ return GetUHWeaponInfo( this ).m_flMeleeRange; }
	virtual float	GetFireRate( void )		{ return GetUHWeaponInfo( this ).m_flMeleeRoF; }
};

//-----------------------------------------------------------------------------
// Purpose: Let the AI know the actual reach of this weapon.
//-----------------------------------------------------------------------------
void CUHMeleeWeapon::Spawn( void )
{
	m_fMinRange1 = 0;
	m_fMinRange2 = 0;
	m_fMaxRange1 = GetRange();
	m_fMaxRange2 = GetRange();

	BaseClass::Spawn();
}


//-----------------------------------------------------------------------------
// Weapon: Nightstick (weapon_melee_baton)
//-----------------------------------------------------------------------------
class CWeaponBaton : public CUHMeleeWeapon
{
	DECLARE_CLASS( CWeaponBaton, CUHMeleeWeapon );
public:
	DECLARE_SERVERCLASS();
	DECLARE_ACTTABLE();

	virtual float GetDamageForActivity( Activity hitActivity )
	{
		if ( GetOwner() && GetOwner()->IsPlayer() )
			return sk_plr_dmg_baton.GetFloat();
		return sk_npc_dmg_baton.GetFloat();
	}
};

IMPLEMENT_SERVERCLASS_ST( CWeaponBaton, DT_WeaponBaton )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_melee_baton, CWeaponBaton );
PRECACHE_WEAPON_REGISTER( weapon_melee_baton );

acttable_t CWeaponBaton::m_acttable[] =
{
	{ ACT_MELEE_ATTACK1, ACT_MELEE_ATTACK_SWING, true },
	{ ACT_IDLE, ACT_IDLE_ANGRY_MELEE, false },
	{ ACT_IDLE_ANGRY, ACT_IDLE_ANGRY_MELEE, false },
};
IMPLEMENT_ACTTABLE( CWeaponBaton );


//-----------------------------------------------------------------------------
// Weapon: Pipe (weapon_melee_pipe)
//-----------------------------------------------------------------------------
class CWeaponPipe : public CUHMeleeWeapon
{
	DECLARE_CLASS( CWeaponPipe, CUHMeleeWeapon );
public:
	DECLARE_SERVERCLASS();
	DECLARE_ACTTABLE();

	virtual float GetDamageForActivity( Activity hitActivity )
	{
		if ( GetOwner() && GetOwner()->IsPlayer() )
			return sk_plr_dmg_pipe.GetFloat();
		return sk_npc_dmg_pipe.GetFloat();
	}
};

IMPLEMENT_SERVERCLASS_ST( CWeaponPipe, DT_WeaponPipe )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_melee_pipe, CWeaponPipe );
PRECACHE_WEAPON_REGISTER( weapon_melee_pipe );

acttable_t CWeaponPipe::m_acttable[] =
{
	{ ACT_MELEE_ATTACK1, ACT_MELEE_ATTACK_SWING, true },
	{ ACT_IDLE, ACT_IDLE_ANGRY_MELEE, false },
	{ ACT_IDLE_ANGRY, ACT_IDLE_ANGRY_MELEE, false },
};
IMPLEMENT_ACTTABLE( CWeaponPipe );


//-----------------------------------------------------------------------------
// Weapon: Axe (weapon_melee_axe)
//-----------------------------------------------------------------------------
class CWeaponAxe : public CUHMeleeWeapon
{
	DECLARE_CLASS( CWeaponAxe, CUHMeleeWeapon );
public:
	DECLARE_SERVERCLASS();
	DECLARE_ACTTABLE();

	virtual float GetDamageForActivity( Activity hitActivity )
	{
		if ( GetOwner() && GetOwner()->IsPlayer() )
			return sk_plr_dmg_axe.GetFloat();
		return sk_npc_dmg_axe.GetFloat();
	}
};

IMPLEMENT_SERVERCLASS_ST( CWeaponAxe, DT_WeaponAxe )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_melee_axe, CWeaponAxe );
PRECACHE_WEAPON_REGISTER( weapon_melee_axe );

acttable_t CWeaponAxe::m_acttable[] =
{
	{ ACT_MELEE_ATTACK1, ACT_MELEE_ATTACK_SWING, true },
	{ ACT_IDLE, ACT_IDLE_ANGRY_MELEE, false },
	{ ACT_IDLE_ANGRY, ACT_IDLE_ANGRY_MELEE, false },
};
IMPLEMENT_ACTTABLE( CWeaponAxe );


//-----------------------------------------------------------------------------
// Weapon: Wrench (weapon_melee_wrench)
//-----------------------------------------------------------------------------
class CWeaponWrench : public CUHMeleeWeapon
{
	DECLARE_CLASS( CWeaponWrench, CUHMeleeWeapon );
public:
	DECLARE_SERVERCLASS();
	DECLARE_ACTTABLE();

	virtual float GetDamageForActivity( Activity hitActivity )
	{
		if ( GetOwner() && GetOwner()->IsPlayer() )
			return sk_plr_dmg_wrench.GetFloat();
		return sk_npc_dmg_wrench.GetFloat();
	}
};

IMPLEMENT_SERVERCLASS_ST( CWeaponWrench, DT_WeaponWrench )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_melee_wrench, CWeaponWrench );
PRECACHE_WEAPON_REGISTER( weapon_melee_wrench );

acttable_t CWeaponWrench::m_acttable[] =
{
	{ ACT_MELEE_ATTACK1, ACT_MELEE_ATTACK_SWING, true },
	{ ACT_IDLE, ACT_IDLE_ANGRY_MELEE, false },
	{ ACT_IDLE_ANGRY, ACT_IDLE_ANGRY_MELEE, false },
};
IMPLEMENT_ACTTABLE( CWeaponWrench );


//-----------------------------------------------------------------------------
// Weapon: Cleaver (weapon_cleaver)
//-----------------------------------------------------------------------------
class CWeaponCleaver : public CUHMeleeWeapon
{
	DECLARE_CLASS( CWeaponCleaver, CUHMeleeWeapon );
public:
	DECLARE_SERVERCLASS();
	DECLARE_ACTTABLE();

	virtual float GetDamageForActivity( Activity hitActivity )
	{
		if ( GetOwner() && GetOwner()->IsPlayer() )
			return sk_plr_dmg_cleaver.GetFloat();
		return sk_npc_dmg_cleaver.GetFloat();
	}
};

IMPLEMENT_SERVERCLASS_ST( CWeaponCleaver, DT_WeaponCleaver )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_cleaver, CWeaponCleaver );
PRECACHE_WEAPON_REGISTER( weapon_cleaver );

acttable_t CWeaponCleaver::m_acttable[] =
{
	{ ACT_MELEE_ATTACK1, ACT_MELEE_ATTACK_SWING, true },
	{ ACT_IDLE, ACT_IDLE_ANGRY_MELEE, false },
	{ ACT_IDLE_ANGRY, ACT_IDLE_ANGRY_MELEE, false },
};
IMPLEMENT_ACTTABLE( CWeaponCleaver );
