//========= Copyright (c) 2008, Mxthe (Underhell). All rights reserved. ============//
//
// Purpose: Underhell night vision + gas mask full-screen overlays (client).
//
// Decoded from the original client (sub_10141600): while the local player has
// m_bNightVisionOn / m_bGasMaskOn set (networked from the server), the client
// draws the "shader/nightvision" and "shader/gasmask" materials full-screen.
// The materials + their shaders live in the game install (Underhell/materials
// + shaders), so this only needs to draw them — the shader does the tint /
// depth effect / mask vignette.
//
// Implemented as an IScreenSpaceEffect registered with the screen-space effect
// manager (rendered from CViewRender::PerformScreenSpaceEffects each frame).
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "screenspaceeffects.h"
#include "materialsystem/materialsystemutil.h"
#include "materialsystem/imaterial.h"
#include "view_scene.h"
#include "c_basehlplayer.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define UH_NIGHTVISION_MAT "shader/nightvision"
#define UH_GASMASK_MAT     "shader/gasmask"

//-----------------------------------------------------------------------------
// CUHGearOverlayEffect — draws the night vision / gas mask overlays.
//-----------------------------------------------------------------------------
class CUHGearOverlayEffect : public IScreenSpaceEffect
{
public:
	CUHGearOverlayEffect( void ) {}

	virtual void Init( void );
	virtual void Shutdown( void );
	virtual void SetParameters( KeyValues *params ) {}
	// Always enabled — the Render() path gates itself on the player's networked
	// night-vision / gas-mask flags (same pattern as CStunEffect).
	virtual void Enable( bool bEnable ) {}
	virtual bool IsEnabled( ) { return true; }

	virtual void Render( int x, int y, int w, int h );

private:
	CMaterialReference m_NightVisionMaterial;
	CMaterialReference m_GasMaskMaterial;
};

ADD_SCREENSPACE_EFFECT( CUHGearOverlayEffect, underhell_gear );

//-----------------------------------------------------------------------------
void CUHGearOverlayEffect::Init( void )
{
	m_NightVisionMaterial.Init( UH_NIGHTVISION_MAT, TEXTURE_GROUP_OTHER );
	m_GasMaskMaterial.Init( UH_GASMASK_MAT, TEXTURE_GROUP_OTHER );
}

//-----------------------------------------------------------------------------
void CUHGearOverlayEffect::Shutdown( void )
{
	m_NightVisionMaterial.Shutdown();
	m_GasMaskMaterial.Shutdown();
}

//-----------------------------------------------------------------------------
// Purpose: Draw the overlays when the local player has the gear active.
//-----------------------------------------------------------------------------
void CUHGearOverlayEffect::Render( int x, int y, int w, int h )
{
	C_BaseHLPlayer *pPlayer = dynamic_cast<C_BaseHLPlayer *>( C_BasePlayer::GetLocalPlayer() );
	if ( !pPlayer || !pPlayer->IsAlive() )
		return;

	if ( pPlayer->m_bNightVisionOn && m_NightVisionMaterial.IsValid() )
	{
		DrawScreenEffectMaterial( m_NightVisionMaterial, x, y, w, h );
	}

	if ( pPlayer->m_bGasMaskOn && m_GasMaskMaterial.IsValid() )
	{
		DrawScreenEffectMaterial( m_GasMaskMaterial, x, y, w, h );
	}
}
