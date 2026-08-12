//========= Copyright 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Underhell extended weapon script parsing (shared by game & client dlls).
//
//=============================================================================//

#include "cbase.h"
#include <KeyValues.h>
#include "uh_weapon_parse.h"
#include "basecombatweapon_shared.h"
#include "util_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//--------------------------------------------------------------------------------------------------------
// Cast a weapon's script data block to the Underhell extension.
//--------------------------------------------------------------------------------------------------------
const CUHWeaponInfo &GetUHWeaponInfo( const CBaseCombatWeapon *pWeapon )
{
	const FileWeaponInfo_t *pWeaponInfo = &pWeapon->GetWpnData();
	const CUHWeaponInfo *pUHInfo;

	#ifdef _DEBUG
		pUHInfo = dynamic_cast< const CUHWeaponInfo* >( pWeaponInfo );
		Assert( pUHInfo );
	#else
		pUHInfo = static_cast< const CUHWeaponInfo* >( pWeaponInfo );
	#endif

	return *pUHInfo;
}


//--------------------------------------------------------------------------------------------------------
// Replaces the stock FileWeaponInfo_t with the Underhell-extended block so every
// weapon script transparently carries recoil/ironsight/melee data.
//--------------------------------------------------------------------------------------------------------
FileWeaponInfo_t* CreateWeaponInfo()
{
	return new CUHWeaponInfo;
}


CUHWeaponInfo::CUHWeaponInfo()
{
	m_flPunchPitchMin = 0.0f;
	m_flPunchPitchMax = 0.0f;
	m_flPunchYawMin = 0.0f;
	m_flPunchYawMax = 0.0f;
	m_flSnapPitchMin = 0.0f;
	m_flSnapPitchMax = 0.0f;
	m_flSnapYawMin = 0.0f;
	m_flSnapYawMax = 0.0f;

	m_flCrouchRecoilMult = 1.0f;
	m_flCrouchAccuracyMult = 1.0f;
	m_flRunAccuracyMult = 1.0f;

	m_iPenetration = 0;

	m_expOffset = Vector( 0, 0, 0 );
	m_expOriOffset = QAngle( 0, 0, 0 );

	m_bOneHanded = false;
	m_flExpAccuracy = 1.0f;

	m_flMeleeDelayedFire = 0.0f;
	m_flMeleeRoF = 0.0f;
	m_flMeleeRange = 32.0f;
	m_flStaminaToDrain = 15.0f;
}


// Helper: "min, max" -> two floats. Empty/absent string yields (0, 0).
static void ParseRange( KeyValues *pKV, const char *szKey, float &flMin, float &flMax )
{
	float tmp[2] = { 0.0f, 0.0f };
	UTIL_StringToFloatArray( tmp, 2, pKV->GetString( szKey, "" ) );
	flMin = tmp[0];
	flMax = tmp[1];
}


void CUHWeaponInfo::Parse( KeyValues *pKeyValuesData, const char *szWeaponName )
{
	BaseClass::Parse( pKeyValuesData, szWeaponName );

	// Recoil / viewpunch ("PunchPitch"/"PunchYaw").
	ParseRange( pKeyValuesData, "PunchPitch", m_flPunchPitchMin, m_flPunchPitchMax );
	ParseRange( pKeyValuesData, "PunchYaw", m_flPunchYawMin, m_flPunchYawMax );

	// Snap ("SnapPitch"/"SnapYaw").
	ParseRange( pKeyValuesData, "SnapPitch", m_flSnapPitchMin, m_flSnapPitchMax );
	ParseRange( pKeyValuesData, "SnapYaw", m_flSnapYawMin, m_flSnapYawMax );

	// Stance multipliers.
	m_flCrouchRecoilMult = pKeyValuesData->GetFloat( "CrouchRecoilMult", 1.0f );
	m_flCrouchAccuracyMult = pKeyValuesData->GetFloat( "CrouchAccuracyMult", 1.0f );
	m_flRunAccuracyMult = pKeyValuesData->GetFloat( "RunAccuracyMult", 1.0f );

	// Ironsight offsets.
	KeyValues *pExpOffset = pKeyValuesData->FindKey( "ExpOffset" );
	if ( pExpOffset )
	{
		m_expOffset.x = pExpOffset->GetFloat( "x", 0.0f );
		m_expOffset.y = pExpOffset->GetFloat( "y", 0.0f );
		m_expOffset.z = pExpOffset->GetFloat( "z", 0.0f );

		m_expOriOffset.x = pExpOffset->GetFloat( "xori", 0.0f );
		m_expOriOffset.y = pExpOffset->GetFloat( "yori", 0.0f );
		m_expOriOffset.z = pExpOffset->GetFloat( "zori", 0.0f );

		m_flExpAccuracy = pExpOffset->GetFloat( "accuracy", 1.0f );
	}
	else
	{
		m_expOffset = Vector( 0, 0, 0 );
		m_expOriOffset = QAngle( 0, 0, 0 );
	}

	// Melee.
	m_bOneHanded = pKeyValuesData->GetInt( "OneHanded", 0 ) != 0;
	m_flMeleeDelayedFire = pKeyValuesData->GetFloat( "MeleeDelayedFire", 0.0f );
	m_flMeleeRoF = pKeyValuesData->GetFloat( "MeleeRoF", 0.0f );
	m_flMeleeRange = pKeyValuesData->GetFloat( "MeleeRange", 32.0f );
	m_flStaminaToDrain = pKeyValuesData->GetFloat( "StaminaToDrain", 15.0f );

	// "UH_Weapon_Special".
	KeyValues *pSpecial = pKeyValuesData->FindKey( "UH_Weapon_Special" );
	if ( pSpecial )
	{
		m_iPenetration = pSpecial->GetInt( "Penetration", 0 );
	}
}
