//========= Copyright (c) 2008, Mxthe (Underhell). All rights reserved. ============//
//
// Purpose: Underhell dot reticle HUD element — implementation.
//
// Behavioural re-implementation of the original CHudDotReticle:
//   * a small centered dot,
//   * lights at full alpha when the player presses +use,
//   * fades linearly to zero over 3.0 s (alpha = (3.0 - elapsed) * 85, from
//     the original paint sub_100BC870),
//   * hidden while iron-sighted.
//
// Divergence (documented): the original stores the trigger timestamp on the
// player (client offset 3456) and stamps it from the free-aim input path;
// that free-aim camera is still TODO. This port detects the +use press edge
// directly in OnThink and keeps the timestamp on the panel, so the dot behaves
// identically without the free-aim dependency.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "hud_dotreticle.h"
#include "c_basehlplayer.h"
#include "iclientmode.h"
#include "in_buttons.h"
#include <vgui/ISurface.h>

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!! 
#include "tier0/memdbgon.h"

DECLARE_HUDELEMENT( CHudDotReticle );

// Fade window + rate, hard-coded in the original (sub_100BC870): full alpha at
// the trigger, linear to 0 over this many seconds.
#define UH_DOTRETICLE_FADE_TIME 3.0f
#define UH_DOTRETICLE_FADE_RATE 85.0f

// The original SetHiddenBits(4096) = (1<<12) — an Underhell custom hide bit
// beyond the vanilla HIDEHUD_BITCOUNT (12). The mod toggles it via its
// SetStatusVisibility / SetHudVisibility player inputs.
#define HIDEHUD_UH_RETICLE ( 1 << 12 )

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudDotReticle::CHudDotReticle( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudDotReticle" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_UH_RETICLE );

	m_flTriggerTime = -100.0f;
	m_bUseHeld = false;
}

//-----------------------------------------------------------------------------
// Purpose: Init / reset
//-----------------------------------------------------------------------------
void CHudDotReticle::Init( void )
{
	m_flTriggerTime = -100.0f;
	m_bUseHeld = false;
}

void CHudDotReticle::Reset( void )
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Persistent element; the dot is drawn (and faded) in Paint, which
// early-outs once the fade window has elapsed.
//-----------------------------------------------------------------------------
bool CHudDotReticle::ShouldDraw( void )
{
	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: Stamp the fade anchor on the +use press edge.
//-----------------------------------------------------------------------------
void CHudDotReticle::OnThink( void )
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	bool bUse = ( pPlayer->m_nButtons & IN_USE ) != 0;
	if ( bUse && !m_bUseHeld )
	{
		m_flTriggerTime = gpGlobals->curtime;
	}
	m_bUseHeld = bUse;
}

//-----------------------------------------------------------------------------
// Purpose: Draw the fading dot at the center of the panel.
//-----------------------------------------------------------------------------
void CHudDotReticle::Paint()
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	// The original skips the dot while iron-sighted (sub_100BC870 checks
	// m_bIronSighted @4140).
	if ( pPlayer->m_bIronSighted )
		return;

	float flElapsed = gpGlobals->curtime - m_flTriggerTime;
	if ( flElapsed < 0.0f || flElapsed >= UH_DOTRETICLE_FADE_TIME )
		return;

	// Linear fade: 255 right at the trigger, 0 at the end of the window.
	int iAlpha = clamp( (int)( ( UH_DOTRETICLE_FADE_TIME - flElapsed ) * UH_DOTRETICLE_FADE_RATE ), 0, 255 );
	if ( iAlpha <= 0 )
		return;

	// dotx/doty are the dot's centre (8,8 in the 16x16 centred panel = screen
	// centre); dotwide/dottall are its size. Draw a small centred square.
	int cx = (int)m_fdotx;
	int cy = (int)m_fdoty;
	int hw = (int)m_fdotwide / 2;
	int hh = (int)m_fdottall / 2;

	vgui::surface()->DrawSetColor( 255, 255, 255, iAlpha );
	vgui::surface()->DrawFilledRect(
		cx - hw, cy - hh,
		cx + hw, cy + hh );
}
