//========= Copyright (c) 2008, Mxthe (Underhell). All rights reserved. ============//
//
// Purpose: Underhell ironsight — server-side CHL2_Player.
//
// "ironsight_toggle" is a client command (not a ConCommand): the engine
// forwards it into CHL2_Player::ClientCommand, which toggles m_bIronSighted.
// The networked flag drives accuracy (fire functions read it) and the FOV
// zoom; the client viewmodel slides to the eye via the weapon's ExpOffset.
//
// Decoded from sub_101ECF40 (toggle) + sub_101F11D0 (ClientCommand dispatch).
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hl2_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Ironsight FOV: this value is subtracted from the default FOV when sighted
// (original convar "uh_ironsight_zoom_focus", default 40).
static ConVar uh_ironsight_zoom_focus( "uh_ironsight_zoom_focus", "40", FCVAR_ARCHIVE,
	"Not actually the zoom value. This value is subtracted from the default FOV." );

// Ironsight FOV transition rate (original "uh_ironsight_zoom", default 0.9).
static ConVar uh_ironsight_zoom( "uh_ironsight_zoom", "0.9", FCVAR_ARCHIVE,
	"FOV transition rate when entering/leaving ironsight." );

// Minimum time between ironsight toggles (original sub_101ECF40 debounce).
#define UH_IRONSIGHT_TOGGLE_DELAY 0.1f

//-----------------------------------------------------------------------------
// Purpose: Toggle ironsight (called from the "ironsight_toggle" client
// command). Mirrors sub_101ECF40: debounce, stop sprinting, toggle the
// networked flag, zoom the FOV, and play the enter/leave sound.
//-----------------------------------------------------------------------------
void CHL2_Player::UH_ToggleIronsight( void )
{
	// Debounce.
	if ( gpGlobals->curtime - m_fIronsightedTime < UH_IRONSIGHT_TOGGLE_DELAY )
		return;

	// Can't sight while sprinting — drop sprint first.
	if ( IsSprinting() )
	{
		StopSprinting();
	}

	if ( !m_bIronSighted )
	{
		m_bIronSighted = true;

		EmitSound( "HL2Player.Ironsighton" );

		int iFOV = GetDefaultFOV() - uh_ironsight_zoom_focus.GetInt();
		SetFOV( this, iFOV, uh_ironsight_zoom.GetFloat() );
	}
	else
	{
		m_bIronSighted = false;

		EmitSound( "HL2Player.Ironsightoff" );

		SetFOV( this, 0, uh_ironsight_zoom.GetFloat() );
	}

	m_fIronsightedTime = gpGlobals->curtime;
}
