//========= Copyright 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Underhell firearm base class (Episodic build).
//
//  Adds the Underhell "weapon system" glue on top of the HL2 combat weapon:
//  - per-weapon damage ConVars (sk_plr_dmg_* / sk_npc_dmg_*)
//  - script-driven recoil (PunchPitch/Yaw + SnapPitch/Yaw + CrouchRecoilMult)
//  - script-driven accuracy (CrouchAccuracyMult / RunAccuracyMult / ExpOffset.accuracy)
//
//  See docs/underhell-weapons-aiming.md and game/shared/episodic/uh_weapon_parse.*.
//
//=============================================================================//

#ifndef UH_BASEFIREARM_H
#define UH_BASEFIREARM_H
#ifdef _WIN32
#pragma once
#endif

#include "basehlcombatweapon.h"
#include "episodic/uh_weapon_parse.h"


//-----------------------------------------------------------------------------
// Underhell firearm base.
//-----------------------------------------------------------------------------
class CUhFirearmWeapon : public CBaseHLCombatWeapon
{
	DECLARE_CLASS( CUhFirearmWeapon, CBaseHLCombatWeapon );
public:
	CUhFirearmWeapon();

	virtual void	FireBullets( const FireBulletsInfo_t &info );
	virtual void	AddViewKick( void );

	virtual int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	// Per-weapon damage. Return 0 to fall back to the ammo definition.
	virtual float	GetPlayerDamage( void ) { return 0.0f; }
	virtual float	GetNPCDamage( void ) { return 0.0f; }

protected:
	const CUHWeaponInfo &GetUHWpnData( void ) const { return GetUHWeaponInfo( this ); }
};


#endif // UH_BASEFIREARM_H
