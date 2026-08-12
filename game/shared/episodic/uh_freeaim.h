//========= Copyright 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Underhell OTS free-aim (cam_ots_freeaim_*).
//
//  In hip-fire the weapon is offset from the view center and auto-turns back;
//  ironsighting locks it to center. See docs/underhell-weapons-aiming.md §2.8.
//
//  Client computes the offset (mouse-driven) and sends it to the server via
//  "update_freeaim" so bullets leave the barrel where the weapon points.
//
//=============================================================================//

#ifndef UH_FREEAIM_H
#define UH_FREEAIM_H
#ifdef _WIN32
#pragma once
#endif

#include "mathlib/vector.h"

class CUserCmd;

#if defined( CLIENT_DLL )

// Current free-aim angle offset (pitch/yaw, degrees) for the local player.
QAngle	UH_FreeAim_GetOffset( void );

// True when the OTS free-aim system is enabled.
bool	UH_FreeAim_IsEnabled( void );

// Accumulate mouse input / auto-turn back to center. Called from CreateMove.
void	UH_FreeAim_Update( CUserCmd *pCmd, float flFrameTime );

// Force the free-aim offset back to zero (e.g. while ironsighted).
void	UH_FreeAim_Reset( void );

#endif // CLIENT_DLL

#endif // UH_FREEAIM_H
