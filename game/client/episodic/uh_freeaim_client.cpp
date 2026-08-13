//========= Copyright 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Underhell OTS free-aim — client accessors.
//
//=============================================================================//

#include "cbase.h"
#include "episodic/uh_freeaim.h"
#include "iinput.h"
#include "c_basehlplayer.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// Purpose: Angle from the screen center to the free-aim cursor, in degrees.
//          The viewmodel and bullet direction rotate by this amount.
//-----------------------------------------------------------------------------
QAngle UH_FreeAim_GetOffset( void )
{
	QAngle angOffset( 0, 0, 0 );

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return angOffset;

	float flFOV = pPlayer->GetFOV();
	Vector2D cursor = ::input->CAM_GetFreeAimCursor();

	// cursor.x/y are normalized screen coords in [-move_max, move_max];
	// convert to an angle: half-screen spans FOV/2.
	angOffset.y = cursor.x * ( flFOV * 0.5f );
	angOffset.x = -cursor.y * ( flFOV * 0.5f );

	return angOffset;
}

bool UH_FreeAim_IsEnabled( void )
{
	return ::input->CAM_IsFreeAiming();
}

void UH_FreeAim_Reset( void )
{
	::input->CAM_ResetFreeAimCursor();
}
