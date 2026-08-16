//========= Copyright (c) 2008, Mxthe (Underhell). All rights reserved. ============//
//
// Purpose: Underhell corpse dragging "weight" — while the player carries a
//          heavy ragdoll, movement speed and mouse sensitivity are reduced.
//
// Decompiled reference (servero_diaphora.dll.c):
//   * pickup path @740190:
//       *((float *)this + 1343) = <current "sensitivity" value>;   // save it
//       if ( *(int *)(ragdoll + 1132) > 10 )                       // mass > 10
//       {
//           SetMaxSpeed( hl2_normspeed * 0.33333334 );
//           ConVar::SetValue( sensitivity,
//                             *((float *)this + 1343) / uh_bodymousedamper );
//       }
//   * release path @965831 and destructor @735824:
//       ConVar::SetValue( sensitivity, *((float *)this + 1343) );  // restore
//
// Note the saved player float @1343 (byte offset 5372) is the SAVED MOUSE
// SENSITIVITY, not a saved movement speed: the restore paths feed it straight
// back into the "sensitivity" convar, and the speed is recomputed from
// hl2_normspeed rather than from a stored value. An earlier port treated this
// field as a saved max speed, which both mis-scaled the speed restore and left
// the sensitivity damping unimplemented.
//
//=============================================================================//

#include "cbase.h"
#include "hl2_player.h"

#include "tier0/memdbgon.h"

// NOTE: uh_bodymousedamper is defined once, in uh_ai.cpp. Declaring it a second
// time here registered the same convar name twice in a single server.dll, which
// the engine rejects at startup ("ConVar uh_bodymousedamper already registered")
// and which can abort game DLL initialization. Reference the existing one.
extern ConVar uh_bodymousedamper;

//-----------------------------------------------------------------------------
// Purpose: Resolve the client "sensitivity" convar (a client convar, but in the
// listen-server/singleplayer process the server DLL reaches it through the
// shared cvar interface, which is exactly what the original does).
//-----------------------------------------------------------------------------
static ConVar *UH_GetSensitivityConVar( void )
{
	static ConVar *s_pSensitivity = NULL;
	if ( !s_pSensitivity )
		s_pSensitivity = cvar->FindVar( "sensitivity" );
	return s_pSensitivity;
}

void CHL2_Player::UH_BeginCarryRagdoll( CBaseEntity *pRagdoll )
{
	if ( !pRagdoll || m_hCarryingRagdoll.Get() == pRagdoll )
		return;

	m_hCarryingRagdoll = pRagdoll;

	// Save the current sensitivity before damping it (original: player @1343).
	ConVar *pSensitivity = UH_GetSensitivityConVar();
	m_flCarryingRagdollSavedSensitivity = pSensitivity ? pSensitivity->GetFloat() : 0.0f;

	IPhysicsObject *pPhysics = pRagdoll->VPhysicsGetObject();
	if ( !pPhysics || pPhysics->GetMass() <= 10.0f )
		return;

	// Speed comes from hl2_normspeed * 1/3, not from a stored speed value.
	ConVar *pNormSpeed = cvar->FindVar( "hl2_normspeed" );
	if ( pNormSpeed )
		SetMaxSpeed( pNormSpeed->GetFloat() * 0.33333334f );

	// Mouse damping: sensitivity / uh_bodymousedamper.
	const float flDamper = uh_bodymousedamper.GetFloat();
	if ( pSensitivity && flDamper > 0.0f && m_flCarryingRagdollSavedSensitivity > 0.0f )
		pSensitivity->SetValue( m_flCarryingRagdollSavedSensitivity / flDamper );
}

void CHL2_Player::UH_UpdateCarryRagdollWeight( void )
{
	CBaseEntity *pRagdoll = m_hCarryingRagdoll.Get();
	if ( pRagdoll && IsHoldingEntity( pRagdoll ) )
		return;

	// Restore sensitivity (original release path writes the saved value back).
	if ( m_flCarryingRagdollSavedSensitivity > 0.0f )
	{
		ConVar *pSensitivity = UH_GetSensitivityConVar();
		if ( pSensitivity )
			pSensitivity->SetValue( m_flCarryingRagdollSavedSensitivity );
	}

	// Restore movement speed from hl2_normspeed, matching the original.
	ConVar *pNormSpeed = cvar->FindVar( "hl2_normspeed" );
	if ( pNormSpeed )
		SetMaxSpeed( pNormSpeed->GetFloat() );

	m_hCarryingRagdoll = NULL;
	m_flCarryingRagdollSavedSensitivity = 0.0f;
}
