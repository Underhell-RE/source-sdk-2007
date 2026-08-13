//========= Copyright (c) 2008, Mxthe (Underhell). All rights reserved. ============//
//
// Purpose: Underhell inventory UI panel — implementation.
//
// Reconstructed 1:1 from the original client.dll:
//   * CInventoryPanel        — hexrays sub_1012EDC0 (ctor), sub_1012E6C0 (OnThink)
//   * cl_inventoryToggle     — hexrays sub_1012E690
//   * "UpdateInventory" cmd  — hexrays sub_102BC600 (registration) / sub_1012E660
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "c_basehlplayer.h"
#include "vgui/ISurface.h"
#include "c_inventory_panel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Per-id slot visuals. Sprite paths and localization tokens are the original
// strings (hexrays sub_1012E6C0 switch). Note: the original's icon colours for
// ids 14..18 do not match the print names (id 14 "Red" shows the Green icon) —
// preserved verbatim for 1:1 behaviour.
//-----------------------------------------------------------------------------
struct UHInventorySlotInfo_t
{
	const char *pszSprite;		// "../Sprites/Hud/Items/..." in the original
	const char *pszTextToken;	// "#UnderHell_Inventory_..."
};

static const UHInventorySlotInfo_t s_InventorySlotInfo[UH_INVENTORY_ITEM_TABLE_SIZE] =
{
	{ NULL,								NULL },									// 0: none
	{ "../Sprites/Hud/Items/AppleRed",			"#UnderHell_Inventory_Apple" },				// 1
	{ "../Sprites/Hud/Items/AppleGreen",		"#UnderHell_Inventory_Apple" },				// 2
	{ "../Sprites/Hud/Items/Banana",			"#UnderHell_Inventory_Banana" },			// 3
	{ "../Sprites/Hud/Items/Burritos",			"#UnderHell_Inventory_Burrito" },			// 4
	{ "../Sprites/Hud/Items/Sandwich",			"#UnderHell_Inventory_Sandwich" },			// 5
	{ "../Sprites/Hud/Items/Banana",			"#UnderHell_Inventory_BananaBunch" },		// 6
	{ "../Sprites/Hud/Items/Soda1",				"#UnderHell_Inventory_Soda" },				// 7
	{ "../Sprites/Hud/Items/Soda2",				"#UnderHell_Inventory_Soda" },				// 8
	{ "../Sprites/Hud/Items/Soda3",				"#UnderHell_Inventory_Soda" },				// 9
	{ "../Sprites/Hud/Items/Soda4",				"#UnderHell_Inventory_Soda" },				// 10
	{ "../Sprites/Hud/Items/Soda5",				"#UnderHell_Inventory_Soda" },				// 11
	{ "../Sprites/Hud/Items/SodaPowerPunch",	"#UnderHell_Inventory_MegaSoda" },			// 12
	{ "../Sprites/Hud/Items/Flares",			"#UnderHell_Inventory_Flare" },				// 13
	{ "../Sprites/Hud/Items/GlowstickGreen",	"#UnderHell_Inventory_Glowstick_Green" },	// 14
	{ "../Sprites/Hud/Items/GlowstickRed",		"#UnderHell_Inventory_Glowstick_Red" },		// 15
	{ "../Sprites/Hud/Items/GlowstickBlue",		"#UnderHell_Inventory_Glowstick_Blue" },	// 16
	{ "../Sprites/Hud/Items/GlowstickYellow",	"#UnderHell_Inventory_Glowstick_Yellow" },	// 17
	{ "../Sprites/Hud/Items/GlowstickPurple",	"#UnderHell_Inventory_Glowstick_Purple" },	// 18
	{ "../Sprites/Hud/Items/GlowstickGreenLit",	"#UnderHell_Inventory_Glowstick_Green" },	// 19
	{ "../Sprites/Hud/Items/GlowstickRedLit",	"#UnderHell_Inventory_Glowstick_Red" },		// 20
	{ "../Sprites/Hud/Items/GlowstickBlueLit",	"#UnderHell_Inventory_Glowstick_Blue" },	// 21
	{ "../Sprites/Hud/Items/GlowstickYellowLit","#UnderHell_Inventory_Glowstick_Yellow" },	// 22
	{ "../Sprites/Hud/Items/GlowstickPurpleLit","#UnderHell_Inventory_Glowstick_Purple" },	// 23
	{ NULL,								"#UnderHell_Inventory_Painkillers" },			// 24
	{ NULL,								"#UnderHell_Inventory_Syringe" },				// 25
	{ "../Sprites/Hud/Items/Bandages",			"#UnderHell_Inventory_Bandages" },			// 26
	{ "../Sprites/Hud/Items/Healthkit",			"#UnderHell_Inventory_Healthkit" },			// 27
	{ "../Sprites/Hud/Items/HealthSpray",		"#UnderHell_Inventory_HealthVial" },			// 28
	{ "../Sprites/Hud/Items/Chocobar",			"#UnderHell_Inventory_Chocobar" },			// 29
	{ "../Sprites/Hud/Items/Orange",			"#UnderHell_Inventory_Orange" },				// 30
	{ "../Sprites/Hud/Items/FMRadios",			"#UnderHell_Inventory_FMRadio" },			// 31
	{ "../Sprites/Hud/Items/RadioCrackers",		"#UnderHell_Inventory_RadioCracker" },		// 32
};

//-----------------------------------------------------------------------------
// Singleton.
//-----------------------------------------------------------------------------
static CInventoryPanel *s_pInventoryPanel = NULL;

CInventoryPanel *GetInventoryPanel( void )
{
	if ( !s_pInventoryPanel )
	{
		s_pInventoryPanel = new CInventoryPanel( vgui::surface()->GetEmbeddedPanel() );
	}

	return s_pInventoryPanel;
}

//-----------------------------------------------------------------------------
// Console commands.
//-----------------------------------------------------------------------------
CON_COMMAND( cl_inventoryToggle, "#Inventory" )
{
	// Original (hexrays sub_1012E690): ask the server for a resync, then flip
	// the panel's visibility.
	engine->ClientCmd( "UpdateInventory" );

	if ( GetInventoryPanel() )
	{
		GetInventoryPanel()->Toggle();
	}
}

CON_COMMAND( UpdateInventory, "Updates the inventory" )
{
	// Original (hexrays sub_102BC600/sub_1012E660): flags a full refresh.
	if ( GetInventoryPanel() )
	{
		GetInventoryPanel()->RequestRefresh();
	}
}

//-----------------------------------------------------------------------------
// CInventorySlotPanel
//-----------------------------------------------------------------------------
CInventorySlotPanel::CInventorySlotPanel( vgui::Panel *pParent, const char *pszName )
	: BaseClass( pParent, pszName )
{
	m_pIcon = new vgui::ImagePanel( this, "Icon" );
	m_pLabel = new vgui::Label( this, "ItemName", "" );
}

CInventorySlotPanel::~CInventorySlotPanel()
{
}

void CInventorySlotPanel::PerformLayout( void )
{
	BaseClass::PerformLayout();

	m_pIcon->SetBounds( 0, 0, GetWide(), GetTall() );
	m_pLabel->SetBounds( 0, GetTall() - 16, GetWide(), 16 );
}

// TODO: original drew the sprites through the HUD sprite API — verify how
// "../Sprites/Hud/Items/..." maps onto material names (likely lowercase
// "sprites/hud/items/..." without extension).
static void UH_ApplySlotSprite( vgui::ImagePanel *pIcon, const char *pszSprite )
{
	if ( !pszSprite || !*pszSprite )
	{
		pIcon->SetVisible( false );
		return;
	}

	static char szMaterial[MAX_PATH];
	Q_strncpy( szMaterial, pszSprite, sizeof( szMaterial ) );

	// Strip the "../Sprites/" prefix the original used.
	const char *pszName = szMaterial;
	if ( !Q_strnicmp( pszName, "../Sprites/", 11 ) )
	{
		pszName += 11;
	}

	Q_strlower( (char *)pszName );

	pIcon->SetImage( pszName );
	pIcon->SetVisible( true );
}

void CInventorySlotPanel::SetSlotContents( const char *pszSprite, const char *pszTextToken )
{
	UH_ApplySlotSprite( m_pIcon, pszSprite );
	m_pLabel->SetText( pszTextToken ? pszTextToken : "" );
}

void CInventorySlotPanel::Clear( void )
{
	m_pIcon->SetVisible( false );
	m_pLabel->SetText( "" );
}

//-----------------------------------------------------------------------------
// CInventoryPanel
//-----------------------------------------------------------------------------
CInventoryPanel::CInventoryPanel( vgui::VPANEL parent )
	: BaseClass( NULL, "InventoryPanel", false )
{
	SetParent( parent );

	// Original ctor colours the frame ARGB(128, 255, 255, 255) (hexrays sub_1012EDC0).
	SetBgColor( Color( 255, 255, 255, 128 ) );

	for ( int i = 0; i < UH_INVENTORY_SLOTS; ++i )
	{
		char szName[32];
		Q_snprintf( szName, sizeof( szName ), "InventorySlot%d", i );
		m_pSlots[i] = new CInventorySlotPanel( this, szName );
	}

	m_bNeedsRefresh = false;
	SetVisible( false );

	DevMsg( "InventoryPanel has been constructed\n" );
}

CInventoryPanel::~CInventoryPanel( void )
{
	s_pInventoryPanel = NULL;
}

void CInventoryPanel::PerformLayout( void )
{
	BaseClass::PerformLayout();

	// 7 x 4 grid of slots.
	// TODO: match the original panel/slot geometry.
	int iSlotW = GetWide() / 7;
	int iSlotH = GetTall() / 4;

	for ( int i = 0; i < UH_INVENTORY_SLOTS; ++i )
	{
		int iX = ( i % 7 ) * iSlotW;
		int iY = ( i / 7 ) * iSlotH;
		m_pSlots[i]->SetBounds( iX, iY, iSlotW, iSlotH );
	}
}

void CInventoryPanel::OnThink( void )
{
	BaseClass::OnThink();

	C_BaseHLPlayer *pPlayer = dynamic_cast<C_BaseHLPlayer *>( C_BasePlayer::GetLocalPlayer() );
	if ( !pPlayer )
	{
		SetVisible( false );
		return;
	}

	// Original gates the panel on the player's health and inventory flag
	// (hexrays sub_1012E6C0). It also checked a bool at client player offset
	// 3681 that is not yet identified — see docs/UNDERHELL.md.
	// TODO: identify and honour that flag.
	if ( !pPlayer->IsAlive() || !pPlayer->m_bInventoryEnabled )
	{
		SetVisible( false );
		return;
	}

	if ( m_bNeedsRefresh )
	{
		ClearSlots();
		RefreshSlots();
		m_bNeedsRefresh = false;
	}
}

void CInventoryPanel::Toggle( void )
{
	SetVisible( !IsVisible() );
}

void CInventoryPanel::ClearSlots( void )
{
	for ( int i = 0; i < UH_INVENTORY_SLOTS; ++i )
	{
		m_pSlots[i]->Clear();
	}
}

void CInventoryPanel::RefreshSlots( void )
{
	C_BaseHLPlayer *pPlayer = dynamic_cast<C_BaseHLPlayer *>( C_BasePlayer::GetLocalPlayer() );
	if ( !pPlayer )
		return;

	for ( int i = 0; i < UH_INVENTORY_SLOTS; ++i )
	{
		int iItem = pPlayer->m_iInventory[i];
		if ( !UH_IsValidInventoryItem( iItem ) )
			continue;

		const UHInventorySlotInfo_t &info = s_InventorySlotInfo[iItem];
		SetSlot( i, info.pszSprite, info.pszTextToken );
	}
}

void CInventoryPanel::SetSlot( int iSlot, const char *pszSprite, const char *pszTextToken )
{
	if ( iSlot >= 0 && iSlot < UH_INVENTORY_SLOTS )
	{
		m_pSlots[iSlot]->SetSlotContents( pszSprite, pszTextToken );
	}
}

//-----------------------------------------------------------------------------
// vgui messages the original panel handled.
// TODO: reconstruct the selection/mouse behaviour (slot drag, use on click).
//-----------------------------------------------------------------------------
void CInventoryPanel::OnNewSelection( void )
{
}

void CInventoryPanel::OnNewMouseReleased( void )
{
}
