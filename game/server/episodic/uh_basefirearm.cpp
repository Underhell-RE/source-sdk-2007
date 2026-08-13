//========= Copyright 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Underhell firearm base class (Episodic build).
//
//  Adds the Underhell "weapon system" glue on top of the HL2 combat weapon:
//  - per-weapon damage ConVars (sk_plr_dmg_* / sk_npc_dmg_*)
//  - script-driven recoil (PunchPitch/Yaw + SnapPitch/Yaw + CrouchRecoilMult)
//  - script-driven accuracy (CrouchAccuracyMult / RunAccuracyMult / ExpOffset.accuracy)
//  - bullet penetration driven by UH_Weapon_Special.Penetration
//
//  See docs/underhell-weapons-aiming.md and game/shared/episodic/uh_weapon_parse.*.
//
//=============================================================================//

#include "cbase.h"
#include "uh_basefirearm.h"
#include "hl2_player.h"
#include "shot_manipulator.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// Penetration tuning. Underhell drives the *budget* (how many surfaces a bullet
// may pass through) from the weapon script (UH_Weapon_Special.Penetration);
// these constants describe the physical behavior of a single pass.
//-----------------------------------------------------------------------------
#define UH_PENETRATION_DAMAGE_FALLOFF		0.5f	// damage retained after passing one surface
#define UH_PENETRATION_STEP_OUT				1.0f	// units to step past a penetrated surface
#define UH_PENETRATION_STEP_OUT_ATTEMPTS	16		// max steps to clear a thick solid


CUhFirearmWeapon::CUhFirearmWeapon()
{
}


//-----------------------------------------------------------------------------
// Purpose: Underhell PrimaryAttack. Mirrors CBaseCombatWeapon::PrimaryAttack
//          but routes the shot through CUhFirearmWeapon::FireBullets so that
//          per-weapon damage, accuracy and penetration apply.
//-----------------------------------------------------------------------------
void CUhFirearmWeapon::PrimaryAttack( void )
{
	// If my clip is empty (and I use clips) start reload.
	if ( UsesClipsForAmmo1() && !m_iClip1 )
	{
		Reload();
		return;
	}

	// Only the player fires this way so we can cast.
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
	{
		return;
	}

	pPlayer->DoMuzzleFlash();

	SendWeaponAnim( GetPrimaryAttackActivity() );

	// Player "shoot" animation.
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	FireBulletsInfo_t info;
	info.m_vecSrc = pPlayer->Weapon_ShootPosition();
	info.m_vecDirShooting = pPlayer->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );

	// To make the firing framerate independent, we may have to fire more than one
	// bullet here on low-framerate systems.
	info.m_iShots = 0;
	float fireRate = GetFireRate();

	while ( m_flNextPrimaryAttack <= gpGlobals->curtime )
	{
		// MUST call sound before removing a round from the clip of a machinegun.
		WeaponSound( SINGLE, m_flNextPrimaryAttack );
		m_flNextPrimaryAttack = m_flNextPrimaryAttack + fireRate;
		info.m_iShots++;
		if ( !fireRate )
			break;
	}

	// Make sure we don't fire more than the amount in the clip.
	if ( UsesClipsForAmmo1() )
	{
		info.m_iShots = min( info.m_iShots, m_iClip1 );
		m_iClip1 -= info.m_iShots;
	}
	else
	{
		info.m_iShots = min( info.m_iShots, pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) );
		pPlayer->RemoveAmmo( info.m_iShots, m_iPrimaryAmmoType );
	}

	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_iAmmoType = m_iPrimaryAmmoType;
	info.m_iTracerFreq = 2;

	// Fire the bullets (server).
	info.m_vecSpread = pPlayer->GetAttackSpread( this );

	FireBullets( info );

	if ( !m_iClip1 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		// HEV suit - indicate out of ammo condition.
		pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );
	}

	// Add our view kick in.
	AddViewKick();
}


//-----------------------------------------------------------------------------
// Purpose: Underhell FireBullets. Injects per-weapon damage, applies the
//          script-driven accuracy multipliers (stance + ironsight) and
//          dispatches to the penetration path when the weapon has a
//          UH_Weapon_Special.Penetration budget.
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

		// Underhell OTS free-aim: rotate the shot toward the free-aim point.
		if ( pHL2Player )
		{
			const QAngle &angFreeAim = pHL2Player->GetFreeAimOffset();
			if ( angFreeAim != vec3_angle )
			{
				QAngle angDir;
				VectorAngles( uhInfo.m_vecDirShooting, angDir );
				angDir += angFreeAim;
				AngleVectors( angDir, &uhInfo.m_vecDirShooting );
			}
		}
	}

	int iPenetration = GetUHWpnData().m_iPenetration;
	if ( iPenetration > 0 )
	{
		FireBulletsPenetrating( uhInfo, iPenetration );
		return;
	}

	// Fire through the owner so both players and NPCs work.
	CBaseCombatCharacter *pOwner = GetOwner();
	if ( pOwner )
	{
		pOwner->FireBullets( uhInfo );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Fire one bullet that passes through up to iPenetration solid
//          surfaces, losing damage each pass, stopping at NPCs/players.
//          Each segment is fired through the engine so impact effects, decals
//          and ammo damage handling stay identical to a regular shot.
//-----------------------------------------------------------------------------
void CUhFirearmWeapon::FireBulletsPenetrating( const FireBulletsInfo_t &info, int iPenetration )
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if ( !pOwner )
	{
		return;
	}

	// One direction per pellet, computed once (prediction seed for players).
	Vector vecDir = info.m_vecDirShooting;

	// Fire each shot separately so pellets penetrate independently.
	for ( int iShot = 0; iShot < info.m_iShots; iShot++ )
	{
		if ( pOwner->IsPlayer() )
		{
			RandomSeed( CBaseEntity::GetPredictionRandomSeed() & 255 );
		}

		CShotManipulator manipulator( info.m_vecDirShooting );
		vecDir = manipulator.ApplySpread( info.m_vecSpread );

		Vector vecSrc = info.m_vecSrc;
		float flDamageScale = 1.0f;
		int iPens = 0;

		while ( iPens <= iPenetration )
		{
			Vector vecEnd = vecSrc + vecDir * info.m_flDistance;

			trace_t tr;
			UTIL_TraceLine( vecSrc, vecEnd, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr );

			// Clear shot to the end: fire the remaining segment and stop.
			if ( tr.fraction == 1.0f )
			{
				FireSegment( info, vecSrc, vecDir, vecEnd, flDamageScale );
				break;
			}

			bool bHitActor = tr.m_pEnt &&
				( tr.m_pEnt->IsPlayer() || tr.m_pEnt->IsNPC() );

			// Stop at an actor, or when the penetration budget is spent.
			if ( bHitActor || iPens == iPenetration )
			{
				FireSegment( info, vecSrc, vecDir, tr.endpos, flDamageScale );
				break;
			}

			// Impact the surface we're about to pass through (decal + reduced damage).
			FireSegment( info, vecSrc, vecDir, tr.endpos, flDamageScale );

			// Pass through the surface.
			iPens++;
			flDamageScale *= UH_PENETRATION_DAMAGE_FALLOFF;

			// Step out of the solid.
			Vector vecNewSrc = tr.endpos + vecDir * UH_PENETRATION_STEP_OUT;
			for ( int i = 0; i < UH_PENETRATION_STEP_OUT_ATTEMPTS; i++ )
			{
				if ( UTIL_PointContents( vecNewSrc ) != CONTENTS_SOLID )
					break;
				vecNewSrc += vecDir * UH_PENETRATION_STEP_OUT;
			}
			vecSrc = vecNewSrc;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Fire a single pre-aimed bullet segment. Spread is left as
//          VECTOR_CONE_PRECALCULATED because the direction was already computed.
//-----------------------------------------------------------------------------
void CUhFirearmWeapon::FireSegment( const FireBulletsInfo_t &info, const Vector &vecSrc,
									const Vector &vecDir, const Vector &vecEnd, float flDamageScale )
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if ( !pOwner )
		return;

	FireBulletsInfo_t seg = info;
	seg.m_iShots = 1;
	seg.m_vecSrc = vecSrc;
	seg.m_vecDirShooting = vecDir;
	seg.m_vecSpread = VECTOR_CONE_PRECALCULATED;
	seg.m_flDistance = ( vecEnd - vecSrc ).Length();
	seg.m_iPlayerDamage = (int)( GetPlayerDamage() * flDamageScale );
	seg.m_iDamage = (int)( GetNPCDamage() * flDamageScale );

	pOwner->FireBullets( seg );
}


//-----------------------------------------------------------------------------
// Purpose: NPC fire path (mirrors the vanilla SMG1/AR2 implementations).
//-----------------------------------------------------------------------------
void CUhFirearmWeapon::Operator_ForceNPCFire( CBaseCombatCharacter *pOperator, bool bSecondary )
{
	if ( !pOperator )
		return;

	// Ensure we have a round to fire.
	m_iClip1++;

	Vector vecShootOrigin, vecShootDir;
	QAngle angShootDir;
	if ( !GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir ) )
	{
		vecShootOrigin = pOperator->Weapon_ShootPosition();
		pOperator->EyeVectors( &vecShootDir, NULL, NULL );
	}
	else
	{
		AngleVectors( angShootDir, &vecShootDir );
	}

	FireBulletsInfo_t info;
	info.m_iShots = 1;
	info.m_vecSrc = vecShootOrigin;
	info.m_vecDirShooting = vecShootDir;
	info.m_vecSpread = VECTOR_CONE_PRECALCULATED;
	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_iAmmoType = m_iPrimaryAmmoType;
	info.m_iTracerFreq = 2;

	FireBullets( info );

	WeaponSound( SINGLE_NPC );
	pOperator->DoMuzzleFlash();
	m_iClip1--;
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
