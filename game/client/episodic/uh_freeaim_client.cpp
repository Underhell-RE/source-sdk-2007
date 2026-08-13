//========= Copyright 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Underhell OTS free-aim — client accessors.
//
//  The free-aim "crosshair" (a small dead-zone cursor) is maintained by CInput
//  in game/client/in_camera.cpp (TryCursorMove + cam_ots_freeaim_* ConVars).
//  Here we turn that cursor into the weapon/bullet aim offset sent to the
//  server:
//
//    offset = angle from screen center to the crosshair  (cursor x FOV/2)
//
//  This matches the VDC 'Over the Shoulder View' tutorial and the decompiled
//  client (sub_100BC870 sends the cursor's world direction via ScreenToWorld;
//  we send the equivalent small-angle offset). Ironsighting resets the cursor
//  so the weapon locks back to the view center.
//
//  See docs/underhell-weapons-aiming.md §2.8.
//
//=============================================================================//

#include "cbase.h"
#include "episodic/uh_freeaim.h"
#include "c_basehlplayer.h"
#include "iinput.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


// The cam_ots_freeaim_* ConVars are registered exactly once, in in_camera.cpp.
// Reference them by name so every consumer reads the same values (a second
// registration would leave one of the copies stale when the user toggles it).
// NOTE: keep the ConVarRef function-local — a file-scope ConVarRef would be
// constructed during static init, possibly before in_camera.cpp registers the
// ConVar (cross-TU init order is unspecified).

//-----------------------------------------------------------------------------
// Purpose: Angle from screen center to the crosshair (pitch/yaw degrees).
//
//  Cursor convention (TryCursorMove): +x = right, +y = down on screen.
//  Source angle convention: +pitch = down, -yaw = right, so:
//      pitch = +cursor.y * FOV/2
//      yaw   = -cursor.x * FOV/2
//-----------------------------------------------------------------------------
QAngle UH_FreeAim_GetOffset( void )
{
	float flFOV = 75.0f;
	C_BasePlayer *pl = C_BasePlayer::GetLocalPlayer();
	if ( pl )
		flFOV = pl->GetFOV();

	Vector2D cursor = ::input->CAM_GetFreeAimCursor();
	QAngle ang;
	ang.x =  cursor.y * ( flFOV * 0.5f );	// pitch
	ang.y = -cursor.x * ( flFOV * 0.5f );	// yaw
	ang.z = 0.0f;
	return ang;
}

bool UH_FreeAim_IsEnabled( void )
{
	static ConVarRef cam_ots_freeaim_enable( "cam_ots_freeaim_enable" );
	return cam_ots_freeaim_enable.GetBool();
}

void UH_FreeAim_Reset( void )
{
	::input->CAM_ResetFreeAimCursor();
}

//-----------------------------------------------------------------------------
// Purpose: Called once per input frame from C_BaseHLPlayer::CreateMove.
//          Keeps the cursor centered while free-aim is disabled or while
//          aiming down the sights. The cursor itself is moved by CInput in
//          TryCursorMove (game/client/in_camera.cpp) — we must not integrate
//          the mouse a second time here, otherwise the weapon stops tracking
//          the crosshair.
//-----------------------------------------------------------------------------
void UH_FreeAim_Update( CUserCmd *pCmd, float flFrameTime )
{
	static ConVarRef cam_ots_freeaim_enable( "cam_ots_freeaim_enable" );

	(void)pCmd;
	(void)flFrameTime;

	if ( !cam_ots_freeaim_enable.GetBool() )
	{
		UH_FreeAim_Reset();
		return;
	}

	C_BaseHLPlayer *pPlayer = dynamic_cast< C_BaseHLPlayer * >( C_BasePlayer::GetLocalPlayer() );
	if ( pPlayer && pPlayer->IsIronSighted() )
	{
		// Ironsight locks the weapon to the view center.
		UH_FreeAim_Reset();
	}
}
