// Underhell standalone grenade-ammo HUD (original CHudGrenadeAmmo).
#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "hud_numericdisplay.h"
#include "c_basehlplayer.h"
#include "ammodef.h"
#include "iclientmode.h"
#include <vgui/ISurface.h>
#include <vgui_controls/AnimationController.h>

#include "tier0/memdbgon.h"

class CHudGrenadeAmmo : public CHudNumericDisplay, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CHudGrenadeAmmo, CHudNumericDisplay );
public:
	CHudGrenadeAmmo( const char *name ) : BaseClass( NULL, "HudGrenadeAmmo" ), CHudElement( name )
	{
		m_iAmmo = -1;
		m_iTexture = -1;
		SetHiddenBits( HIDEHUD_WEAPONSELECTION | HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );
	}
	void Init() { m_iAmmo = -1; SetAlpha( 255 ); }
	void Reset() { m_iAmmo = -1; SetDisplayValue( 0 ); SetAlpha( 255 ); }
	void OnThink()
	{
		C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
		if ( !player ) return;
		int ammoType = GetAmmoDef()->Index( "grenade" );
		int ammo = ammoType >= 0 ? player->GetAmmoCount( ammoType ) : 0;
		if ( ammo != m_iAmmo )
		{
			const char *sequence = ammo <= 0 ? "GrenadeEmpty" :
				m_iAmmo >= 0 && ammo < m_iAmmo ? "GrenadeDecreased" : "GrenadeIncreased";
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( sequence );
			m_iAmmo = ammo;
			SetDisplayValue( ammo );
		}
	}
	bool ShouldDraw() { return m_iAmmo > 0 && CHudElement::ShouldDraw(); }
	void Paint()
	{
		BaseClass::Paint();
		if ( m_iTexture < 0 )
		{
			m_iTexture = vgui::surface()->CreateNewTextureID();
			vgui::surface()->DrawSetTextureFile( m_iTexture, "sprites/hud/weapons/frag", 1, false );
		}
		vgui::surface()->DrawSetTexture( m_iTexture );
		vgui::surface()->DrawSetColor( 255, 255, 255, 255 );
		vgui::surface()->DrawTexturedRect( 2, 2, 34, 34 );
	}
private:
	int m_iAmmo;
	int m_iTexture;
};

DECLARE_HUDELEMENT( CHudGrenadeAmmo );
