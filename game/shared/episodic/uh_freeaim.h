//========= Copyright 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Underhell OTS free-aim (cam_ots_freeaim_*) — client-side accessors.
//
//  In hip-fire the weapon is offset from the view center and auto-turns back;
//  ironsighting locks it to center. See docs/underhell-weapons-aiming.md §2.8.
//
//  The crosshair/cursor state lives in CInput (game/client/in_camera.cpp);
//  these helpers expose it to the viewmodel (weapon angle) and to the
//  C_BaseHLPlayer::CreateMove sync (update_freeaim to the server).
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

// Free-aim angle offset (pitch/yaw degrees) for the local player's weapon/bullets.
QAngle	UH_FreeAim_GetOffset( void );

// True when the OTS free-aim system is enabled.
bool	UH_FreeAim_IsEnabled( void );

// Force the free-aim cursor back to center (e.g. when ironsighting).
void	UH_FreeAim_Reset( void );

// Accumulate/decay the free-aim offset from the mouse delta (CreateMove).
void	UH_FreeAim_Update( CUserCmd *pCmd, float flFrameTime );

#endif // CLIENT_DLL

#endif // UH_FREEAIM_H
