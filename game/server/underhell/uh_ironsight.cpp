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

// Minimum sane ironsight FOV (guards against a user/map setting
// uh_ironsight_zoom_focus >= the default FOV, which would produce FOV <= 0
// and hit SetFOV's FOV==0 unzoom sentinel).
#define UH_IRONSIGHT_MIN_FOV 10

// Ironsight FOV transition rate (original "uh_ironsight_zoom", default 0.9).
static ConVar uh_ironsight_zoom( "uh_ironsight_zoom", "0.9", FCVAR_ARCHIVE,
	"FOV transition rate when entering/leaving ironsight." );

// Minimum time between ironsight toggles (original sub_101ECF40 debounce).
#define UH_IRONSIGHT_TOGGLE_DELAY 0.1f

//-----------------------------------------------------------------------------
// Purpose: Toggle ironsight (called from the "ironsight_toggle" client
// command). Mirrors sub_101ECF40: debounce, stop sprinting, toggle the
// networked flag, zoom the FOV, and play the enter/leave sound.
//
// The FOV is set directly (m_iFOV + m_flFOVTime + m_flFOVRate), NOT through
// CBasePlayer::SetFOV: SetFOV claims m_hZoomOwner, which is shared with the
// vanilla suit zoom (StartZooming/StopZooming/IsZooming), so going through it
// makes IsZooming() return true while only ironsighted and corrupts the suit
// zoom state. The original uses a raw FOV set (sub_100F8040) for the same
// reason.
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

		// Clamp so a large uh_ironsight_zoom_focus can never produce FOV <= 0.
		int iFOV = max( UH_IRONSIGHT_MIN_FOV, GetDefaultFOV() - uh_ironsight_zoom_focus.GetInt() );

		m_iFOVStart = GetFOV();
		m_flFOVTime = gpGlobals->curtime;
		m_iFOV = iFOV;
		m_Local.m_flFOVRate = uh_ironsight_zoom.GetFloat();
	}
	else
	{
		m_bIronSighted = false;

		EmitSound( "HL2Player.Ironsightoff" );

		// FOV 0 = "use default" in GetFOV().
		m_iFOVStart = GetFOV();
		m_flFOVTime = gpGlobals->curtime;
		m_iFOV = 0;
		m_Local.m_flFOVRate = uh_ironsight_zoom.GetFloat();
	}

	m_fIronsightedTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: Force ironsight off without the debounce/sound (used when the
// weapon changes or is dropped so the FOV can't stay zoomed). The viewmodel
// resets to hip on its own; only the authoritative flag + FOV need clearing.
//-----------------------------------------------------------------------------
void CHL2_Player::UH_DisableIronsight( void )
{
	if ( !m_bIronSighted )
		return;

	m_bIronSighted = false;

	m_iFOVStart = GetFOV();
	m_flFOVTime = gpGlobals->curtime;
	m_iFOV = 0;
	m_Local.m_flFOVRate = uh_ironsight_zoom.GetFloat();

	m_fIronsightedTime = gpGlobals->curtime;
}
