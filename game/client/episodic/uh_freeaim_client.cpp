//========= Copyright 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Underhell OTS free-aim — client accessors + per-frame update.
//
//  Free-aim is a small, self-recentering aim offset layered ON TOP of normal
//  mouse look (we never replace ApplyMouse). The viewmodel and bullets rotate
//  by this offset while in hip-fire; ironsighting locks them back to center.
//
//  See docs/underhell-weapons-aiming.md §2.8.
//
//=============================================================================//

#include "cbase.h"
#include "episodic/uh_freeaim.h"
#include "usercmd.h"
#include "c_basehlplayer.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static ConVar cam_ots_freeaim_enable( "cam_ots_freeaim_enable", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE );
static ConVar cam_ots_freeaim_speed_turn( "cam_ots_freeaim_speed_turn", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE );

// Server-side clamps recovered from the decompile (sub_101F1D70).
#define UH_FREEAIM_MAX_PITCH		25.0f
#define UH_FREEAIM_MAX_YAW			12.0f
#define UH_FREEAIM_DECAY			3.0f	// recenter rate (1/s)

static QAngle s_FreeAimOffset( 0, 0, 0 );


QAngle UH_FreeAim_GetOffset( void )
{
	return s_FreeAimOffset;
}

bool UH_FreeAim_IsEnabled( void )
{
	return cam_ots_freeaim_enable.GetBool();
}

void UH_FreeAim_Reset( void )
{
	s_FreeAimOffset.Init();
}

//-----------------------------------------------------------------------------
// Purpose: Accumulate the free-aim offset from the (already processed) mouse
//          delta and gently decay it back to center. Called from CreateMove.
//-----------------------------------------------------------------------------
void UH_FreeAim_Update( CUserCmd *pCmd, float flFrameTime )
{
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
		return;
	}

	static ConVarRef m_pitch( "m_pitch" );
	float flScale = m_pitch.GetFloat() * cam_ots_freeaim_speed_turn.GetFloat();

	s_FreeAimOffset.y += pCmd->mousedx * flScale;
	s_FreeAimOffset.x -= pCmd->mousedy * flScale;

	// Decay back toward center (recenter when the mouse stops).
	float flDecay = min( 1.0f, flFrameTime * UH_FREEAIM_DECAY );
	s_FreeAimOffset *= ( 1.0f - flDecay );

	// Clamp (server-side limits from the decompile).
	s_FreeAimOffset.x = clamp( s_FreeAimOffset.x, -UH_FREEAIM_MAX_PITCH, UH_FREEAIM_MAX_PITCH );
	s_FreeAimOffset.y = clamp( s_FreeAimOffset.y, -UH_FREEAIM_MAX_YAW, UH_FREEAIM_MAX_YAW );
}
