//========= Copyright 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Underhell firearm base class (Episodic build).
//
//=============================================================================//

#include "cbase.h"
#include "uh_basefirearm.h"
#include "hl2_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


CUhFirearmWeapon::CUhFirearmWeapon()
{
}


//-----------------------------------------------------------------------------
// Purpose: Underhell FireBullets. Injects per-weapon damage and applies the
//          script-driven accuracy multipliers (stance + ironsight), then fires.
//-----------------------------------------------------------------------------
void CUhFirearmWeapon::FireBullets( const FireBulletsInfo_t &info )
{
	FireBulletsInfo_t uhInfo = info;

	// Per-weapon damage (0 -> fall back to the ammo definition).
	uhInfo.m_iPlayerDamage = (int)GetPlayerDamage();
	uhInfo.m_iDamage = (int)GetNPCDamage();

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( pPlayer )
	{
		const CUHWeaponInfo &wpn = GetUHWpnData();
		float flAccuracyMult = 1.0f;

		if ( pPlayer->GetFlags() & FL_DUCKING )
		{
			flAccuracyMult *= wpn.m_flCrouchAccuracyMult;
		}
		else if ( pPlayer->GetAbsVelocity().Length2DSqr() > Square( pPlayer->MaxSpeed() * 0.5f ) )
		{
			flAccuracyMult *= wpn.m_flRunAccuracyMult;
		}

		CHL2_Player *pHL2Player = dynamic_cast< CHL2_Player * >( pPlayer );
		if ( pHL2Player && pHL2Player->IsIronSighted() )
		{
			flAccuracyMult *= wpn.m_flExpAccuracy;
		}

		uhInfo.m_vecSpread *= flAccuracyMult;
	}

	BaseClass::FireBullets( uhInfo );
}


//-----------------------------------------------------------------------------
// Purpose: Underhell view kick. Punch + snap recoil from the weapon script,
//          scaled by the crouch multiplier.
//-----------------------------------------------------------------------------
void CUhFirearmWeapon::AddViewKick( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
		return;

	const CUHWeaponInfo &wpn = GetUHWpnData();

	float flPunchPitch = random->RandomFloat( wpn.m_flPunchPitchMin, wpn.m_flPunchPitchMax )
					   + random->RandomFloat( wpn.m_flSnapPitchMin, wpn.m_flSnapPitchMax );
	float flPunchYaw = random->RandomFloat( wpn.m_flPunchYawMin, wpn.m_flPunchYawMax )
					 + random->RandomFloat( wpn.m_flSnapYawMin, wpn.m_flSnapYawMax );

	if ( pPlayer->GetFlags() & FL_DUCKING )
	{
		flPunchPitch *= wpn.m_flCrouchRecoilMult;
		flPunchYaw *= wpn.m_flCrouchRecoilMult;
	}

	pPlayer->ViewPunch( QAngle( flPunchPitch, flPunchYaw, 0.0f ) );
}
