//========= Copyright 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Underhell weapon client stubs (Episodic build).
//          Mirrors the server-side classes in game/server/episodic/.
//
//=============================================================================//

#include "cbase.h"
#include "c_weapon__stubs.h"
#include "basehlcombatweapon_shared.h"
#include "c_basehlcombatweapon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Melee
STUB_WEAPON_CLASS( weapon_melee_baton, WeaponBaton, C_BaseHLBludgeonWeapon );
STUB_WEAPON_CLASS( weapon_melee_pipe, WeaponPipe, C_BaseHLBludgeonWeapon );
STUB_WEAPON_CLASS( weapon_melee_axe, WeaponAxe, C_BaseHLBludgeonWeapon );
STUB_WEAPON_CLASS( weapon_melee_wrench, WeaponWrench, C_BaseHLBludgeonWeapon );
STUB_WEAPON_CLASS( weapon_cleaver, WeaponCleaver, C_BaseHLBludgeonWeapon );
