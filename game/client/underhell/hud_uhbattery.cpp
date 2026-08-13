//========= Copyright (c) 2008, Mxthe (Underhell). All rights reserved. ============//
//
// Purpose: Underhell flashlight battery HUD element — implementation.
//
// Behavioural re-implementation of CHudUHBattery (sub_100BDF90 think,
// sub_100BDC80 paint): a contour outline + a vertical charge bar + the
// discrete battery count ("x<N>"). The original also fades the gauge out when
// it is stable; the count and the flashlight/nightvision toggles bring it
// back to full alpha.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "hud_uhbattery.h"
#include "c_basehlplayer.h"
#include "iclientmode.h"
#include <vgui/ISurface.h>

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_HUDELEMENT( CHudUHBattery );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudUHBattery::CHudUHBattery( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudUHBattery" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );

	m_iBatteryCount = -1;
	m_iContourTexture = -1;
	m_iAlpha = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Init / reset
//-----------------------------------------------------------------------------
void CHudUHBattery::Init( void )
{
	m_iBatteryCount = -1;
	m_iAlpha = 0;
}

void CHudUHBattery::Reset( void )
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Persistent element; drawn whenever the HUD itself is not hidden.
//-----------------------------------------------------------------------------
bool CHudUHBattery::ShouldDraw( void )
{
	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: Track the battery count. When it (or the flashlight / nightvision)
// changes, bring the gauge to full alpha; otherwise fade it out.
//-----------------------------------------------------------------------------
void CHudUHBattery::OnThink( void )
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	if ( pPlayer->m_bFlashlightOn || pPlayer->m_iBatteryCount != m_iBatteryCount )
	{
		m_iAlpha = 255;
	}
	else
	{
		m_iAlpha = max( 0, m_iAlpha - 5 );
	}

	m_iBatteryCount = pPlayer->m_iBatteryCount;
}

//-----------------------------------------------------------------------------
// Purpose: Draw the battery outline, the charge bar and the count.
//-----------------------------------------------------------------------------
void CHudUHBattery::Paint()
{
	vgui::surface()->DrawSetColor( 255, 255, 255, m_iAlpha );

	// Battery outline (contour sprite).
	if ( m_iContourTexture < 0 )
	{
		m_iContourTexture = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile( m_iContourTexture, "sprites/hud/hud_battery_contour", 1, false );
	}

	vgui::surface()->DrawSetTexture( m_iContourTexture );
	vgui::surface()->DrawTexturedRect(
		(int)m_flContourX, (int)m_flContourY,
		(int)( m_flContourX + m_flContourWide ), (int)( m_flContourY + m_flContourTall ) );

	// Charge bar: each battery is one chunk, filling bottom-up.
	float flChunkStep = m_flBarChunkHeight + m_flBarChunkGap;
	int chunkCount = ( flChunkStep > 0.0f ) ? (int)( m_flBarHeight / flChunkStep ) : 0;
	int enabledChunks = clamp( m_iBatteryCount, 0, chunkCount );

	vgui::surface()->DrawSetColor( m_HullColor[0], m_HullColor[1], m_HullColor[2], m_HullColor[3] * m_iAlpha / 255 );
	for ( int i = 0; i < enabledChunks; i++ )
	{
		int yBottom = (int)( m_flBarInsetY - i * flChunkStep );
		int yTop = yBottom - (int)m_flBarChunkHeight;
		vgui::surface()->DrawFilledRect(
			(int)m_flBarInsetX, yTop,
			(int)( m_flBarInsetX + m_flBarWidth ), yBottom );
	}

	// Exhausted portion.
	vgui::surface()->DrawSetColor( m_HullColor[0], m_HullColor[1], m_HullColor[2], m_iHullDisabledAlpha );
	for ( int i = enabledChunks; i < chunkCount; i++ )
	{
		int yBottom = (int)( m_flBarInsetY - i * flChunkStep );
		int yTop = yBottom - (int)m_flBarChunkHeight;
		vgui::surface()->DrawFilledRect(
			(int)m_flBarInsetX, yTop,
			(int)( m_flBarInsetX + m_flBarWidth ), yBottom );
	}

	// Battery count: original prints "   x<N>" with the HudNumbers font.
	wchar_t szText[16];
	swprintf( szText, L"x%i", m_iBatteryCount );

	vgui::surface()->DrawSetTextFont( m_hNumberFont );
	vgui::surface()->DrawSetTextColor( m_HullColor[0], m_HullColor[1], m_HullColor[2], m_iAlpha );
	vgui::surface()->DrawSetTextPos( (int)( m_flBarInsetX + m_flBarWidth + 2 ), (int)m_flBarInsetY - 8 );
	vgui::surface()->DrawUnicodeString( szText );
}
