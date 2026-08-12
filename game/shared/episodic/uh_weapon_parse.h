//========= Copyright 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Underhell extended weapon script parsing (shared by game & client dlls).
//
// Underhell extends the HL2/Episodic weapon data block with recoil/viewpunch,
// ironsight offsets and melee/stamina data. See docs/underhell-weapons-aiming.md.
//
//=============================================================================//

#ifndef UH_WEAPON_PARSE_H
#define UH_WEAPON_PARSE_H
#ifdef _WIN32
#pragma once
#endif

#include "weapon_parse.h"
#include "networkvar.h"
#include "mathlib/vector.h"

class CBaseCombatWeapon;


//--------------------------------------------------------------------------------------------------------
// Underhell weapon info: FileWeaponInfo_t + the extra WeaponData keys.
// Created by CreateWeaponInfo() so every weapon script transparently carries this data.
//--------------------------------------------------------------------------------------------------------
class CUHWeaponInfo : public FileWeaponInfo_t
{
public:
	DECLARE_CLASS_GAMEROOT( CUHWeaponInfo, FileWeaponInfo_t );

	CUHWeaponInfo();

	virtual void Parse( ::KeyValues *pKeyValuesData, const char *szWeaponName );

public:
	// Recoil / viewpunch ("PunchPitch"/"PunchYaw" = random in [min,max]).
	// Field order matches the decompiled Underhell FileWeaponInfo_t layout 1:1.
	float		m_flPunchPitchMin;
	float		m_flPunchPitchMax;
	float		m_flPunchYawMin;
	float		m_flPunchYawMax;

	// Snap ("SnapPitch"/"SnapYaw") — the instant aim "kick" added to the punch.
	float		m_flSnapPitchMin;
	float		m_flSnapPitchMax;
	float		m_flSnapYawMin;
	float		m_flSnapYawMax;

	// Stance multipliers ("CrouchRecoilMult"/"CrouchAccuracyMult"/"RunAccuracyMult").
	float		m_flCrouchRecoilMult;
	float		m_flCrouchAccuracyMult;
	float		m_flRunAccuracyMult;

	// "UH_Weapon_Special.Penetration" — surfaces the bullet can pass through (0 = off).
	int			m_iPenetration;

	// Ironsight offsets ("ExpOffset": x/y/z/xori/yori/zori).
	Vector		m_expOffset;
	QAngle		m_expOriOffset;

	// "OneHanded".
	bool		m_bOneHanded;

	// "ExpOffset.accuracy" — spread multiplier while ironsighted.
	float		m_flExpAccuracy;

	// Melee ("MeleeDelayedFire"/"MeleeRoF"/"MeleeRange"/"StaminaToDrain").
	float		m_flMeleeDelayedFire;
	float		m_flMeleeRoF;
	float		m_flMeleeRange;
	float		m_flStaminaToDrain;
};


//--------------------------------------------------------------------------------------------------------
// Accessor: cast a weapon's script data block to the Underhell extension.
// Only valid in this (Episodic) build, where CreateWeaponInfo() returns CUHWeaponInfo.
//--------------------------------------------------------------------------------------------------------
const CUHWeaponInfo &GetUHWeaponInfo( const CBaseCombatWeapon *pWeapon );


#endif // UH_WEAPON_PARSE_H
