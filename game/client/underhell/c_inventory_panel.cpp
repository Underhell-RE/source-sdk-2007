//========= Copyright (c) 2008, Mxthe (Underhell). All rights reserved. ============//
//
// Purpose: Underhell inventory UI panel — implementation.
//
// Reconstructed 1:1 from the original client.dll and mod assets:
//   * CInventoryPanel        — hexrays sub_1012EDC0 (ctor), sub_1012E360
//                              (slot creation), sub_1012E6C0 (OnThink)
//   * layout                 — resource/UI/InventoryPanel.res (1024x512 @ 368,84)
//   * sprites                — materials/Sprites/Hud/Items/*.vmt
//   * cl_inventoryToggle     — hexrays sub_1012E690
//   * "UpdateInventory" cmd  — hexrays sub_102BC600 (registration) / sub_1012E660
//
// Drawing notes: the sprites are plain material paths, not vgui scheme
// images, so they are painted directly through ISurface::DrawSetTextureFile
// (same resolution path the original HUD sprite API used).
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "c_basehlplayer.h"
#include "vgui/ISurface.h"
#include "inputsystem/buttoncode.h"
#include "c_inventory_panel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Per-id slot visuals. Sprite paths and localization tokens are the original
// strings (hexrays sub_1012E6C0 switch), all present in the mod's materials
// and Underhell_english.txt. Note: the original's icon colours for ids 14..18
// do not match the print names (id 14 "Red" shows the Green icon) — preserved
// verbatim for 1:1 behaviour. Ids 24/25 (painkillers/syringe) have text only.
//-----------------------------------------------------------------------------
struct UHInventorySlotInfo_t
{
	const char *pszSprite;		// "Sprites/Hud/Items/..."
	const char *pszTextToken;	// "#UnderHell_Inventory_..."
};

static const UHInventorySlotInfo_t s_InventorySlotInfo[UH_INVENTORY_ITEM_TABLE_SIZE] =
{
	{ NULL,									NULL },									// 0: none
	{ "Sprites/Hud/Items/AppleRed",			"#UnderHell_Inventory_Apple" },				// 1
	{ "Sprites/Hud/Items/AppleGreen",		"#UnderHell_Inventory_Apple" },				// 2
	{ "Sprites/Hud/Items/Banana",			"#UnderHell_Inventory_Banana" },			// 3
	{ "Sprites/Hud/Items/Burritos",			"#UnderHell_Inventory_Burrito" },			// 4
	{ "Sprites/Hud/Items/Sandwich",			"#UnderHell_Inventory_Sandwich" },			// 5
	{ "Sprites/Hud/Items/Banana",			"#UnderHell_Inventory_BananaBunch" },		// 6
	{ "Sprites/Hud/Items/Soda1",			"#UnderHell_Inventory_Soda" },				// 7
	{ "Sprites/Hud/Items/Soda2",			"#UnderHell_Inventory_Soda" },				// 8
	{ "Sprites/Hud/Items/Soda3",			"#UnderHell_Inventory_Soda" },				// 9
	{ "Sprites/Hud/Items/Soda4",			"#UnderHell_Inventory_Soda" },				// 10
	{ "Sprites/Hud/Items/Soda5",			"#UnderHell_Inventory_Soda" },				// 11
	{ "Sprites/Hud/Items/SodaPowerPunch",	"#UnderHell_Inventory_MegaSoda" },			// 12
	{ "Sprites/Hud/Items/Flares",			"#UnderHell_Inventory_Flare" },				// 13
	{ "Sprites/Hud/Items/GlowstickGreen",	"#UnderHell_Inventory_Glowstick_Green" },	// 14
	{ "Sprites/Hud/Items/GlowstickRed",		"#UnderHell_Inventory_Glowstick_Red" },		// 15
	{ "Sprites/Hud/Items/GlowstickBlue",	"#UnderHell_Inventory_Glowstick_Blue" },	// 16
	{ "Sprites/Hud/Items/GlowstickYellow",	"#UnderHell_Inventory_Glowstick_Yellow" },	// 17
	{ "Sprites/Hud/Items/GlowstickPurple",	"#UnderHell_Inventory_Glowstick_Purple" },	// 18
	{ "Sprites/Hud/Items/GlowstickGreenLit","#UnderHell_Inventory_Glowstick_Green" },	// 19
	{ "Sprites/Hud/Items/GlowstickRedLit",	"#UnderHell_Inventory_Glowstick_Red" },		// 20
	{ "Sprites/Hud/Items/GlowstickBlueLit",	"#UnderHell_Inventory_Glowstick_Blue" },	// 21
	{ "Sprites/Hud/Items/GlowstickYellowLit","#UnderHell_Inventory_Glowstick_Yellow" },	// 22
	{ "Sprites/Hud/Items/GlowstickPurpleLit","#UnderHell_Inventory_Glowstick_Purple" },	// 23
	{ NULL,									"#UnderHell_Inventory_Painkillers" },		// 24
	{ NULL,									"#UnderHell_Inventory_Syringe" },			// 25
	{ "Sprites/Hud/Items/Bandages",			"#UnderHell_Inventory_Bandages" },			// 26
	{ "Sprites/Hud/Items/HealthKit",		"#UnderHell_Inventory_HealthKit" },			// 27
	{ "Sprites/Hud/Items/HealthSpray",		"#UnderHell_Inventory_HealthVial" },		// 28
	{ "Sprites/Hud/Items/Chocobar",			"#UnderHell_Inventory_Chocobar" },			// 29
	{ "Sprites/Hud/Items/Orange",			"#UnderHell_Inventory_Orange" },			// 30
	{ "Sprites/Hud/Items/FMRadios",			"#UnderHell_Inventory_FMRadio" },			// 31
	{ "Sprites/Hud/Items/RadioCrackers",	"#UnderHell_Inventory_RadioCracker" },		// 32
};

// Empty slots show the blank sprite (alpha 0 in the mod's material — invisible,
// but 1:1 with the original code path).
#define UH_INVENTORY_BLANK_SPRITE "Sprites/Hud/Inventory/Blank"

#define UH_INVENTORY_BG_SPRITE    "Sprites/Hud/Inventory/Inventory"
#define UH_INVENTORY_ICON_SIZE    28	// original icon size (hexrays sub_1012E360)

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
CON_COMMAND( cl_inventoryToggle, "Toggles the inventory." )
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
	m_pszSprite = NULL;
	m_iTextureId = vgui::surface()->CreateNewTextureID();

	m_pLabel = new vgui::Label( this, "ItemName", "" );
	m_pLabel->SetContentAlignment( vgui::Label::a_northwest );
	m_pLabel->SetWrap( false );

	// Slots never take input — clicks pass through to the frame.
	SetMouseInputEnabled( false );
}

CInventorySlotPanel::~CInventorySlotPanel()
{
}

void CInventorySlotPanel::PerformLayout( void )
{
	BaseClass::PerformLayout();

	// Original drew the icons at 28x28 (hexrays sub_1012E360).
	m_pLabel->SetBounds( 2, UH_INVENTORY_ICON_SIZE + 2, GetWide() - 4, GetTall() - UH_INVENTORY_ICON_SIZE - 2 );
}

void CInventorySlotPanel::PaintBackground( void )
{
	if ( !m_pszSprite || !*m_pszSprite )
		return;

	vgui::surface()->DrawSetColor( 255, 255, 255, 255 );
	vgui::surface()->DrawSetTextureFile( m_iTextureId, m_pszSprite, true, false );
	vgui::surface()->DrawTexturedRect( 0, 0, UH_INVENTORY_ICON_SIZE, UH_INVENTORY_ICON_SIZE );
}

void CInventorySlotPanel::SetSlotContents( const char *pszSprite, const char *pszTextToken )
{
	m_pszSprite = pszSprite;
	m_pLabel->SetText( pszTextToken ? pszTextToken : "" );
}

void CInventorySlotPanel::Clear( void )
{
	// Original first paints the blank sprite on every slot, then fills the
	// non-empty ones (hexrays sub_1012E6C0). The mod's blank material has
	// alpha 0 — empty slots are invisible, like in the original.
	m_pszSprite = UH_INVENTORY_BLANK_SPRITE;
	m_pLabel->SetText( "" );
}

//-----------------------------------------------------------------------------
// CInventoryPanel
//-----------------------------------------------------------------------------
CInventoryPanel::CInventoryPanel( vgui::VPANEL parent )
	: BaseClass( NULL, "InventoryPanel", false )
{
	SetParent( parent );

	// The original starts hidden; the .res says visible=1, so force-hide
	// both before and after loading it.
	SetVisible( false );

	m_iBackgroundTextureId = vgui::surface()->CreateNewTextureID();

	// Load the original layout (size, position, title bar). The ControlName
	// in the .res is "CInventoryPanel", matching the original factory name.
	LoadControlSettings( "resource/UI/InventoryPanel.res" );

	// Fallback in case the .res is missing from a mod install.
	if ( GetWide() < 100 )
	{
		SetBounds( 368, 84, 1024, 512 );
	}

	// Original ctor colours the frame ARGB(128, 255, 255, 255)
	// (hexrays sub_1012EDC0) — only shows through if the texture is missing.
	SetBgColor( Color( 255, 255, 255, 128 ) );

	// 28 slot panels: a 4x6 grid (slots 0..23) plus a row of 4 (24..27),
	// mirroring the original creation loops (hexrays sub_1012E360).
	for ( int i = 0; i < UH_INVENTORY_SLOTS; ++i )
	{
		char szName[32];
		Q_snprintf( szName, sizeof( szName ), "InventorySlot%d", i );
		m_pSlots[i] = new CInventorySlotPanel( this, szName );
	}

	m_bNeedsRefresh = true;

	// The .res sets visible=1 — keep the panel hidden until toggled.
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

	// 4 columns x 7 rows: slots 0..23 fill rows 0..5, slots 24..27 the last row.
	int iCols = 4;
	int iRows = 7;
	int iSlotW = GetWide() / iCols;
	int iSlotH = GetTall() / iRows;

	for ( int i = 0; i < UH_INVENTORY_SLOTS; ++i )
	{
		int iX = ( i % iCols ) * iSlotW;
		int iY = ( i / iCols ) * iSlotH;

		m_pSlots[i]->SetBounds( iX, iY, iSlotW, iSlotH );
	}
}

void CInventoryPanel::PaintBackground( void )
{
	// Background texture from the original .res, stretched over the panel.
	vgui::surface()->DrawSetColor( 255, 255, 255, 255 );
	vgui::surface()->DrawSetTextureFile( m_iBackgroundTextureId, UH_INVENTORY_BG_SPRITE, true, false );
	vgui::surface()->DrawTexturedRect( 0, 0, GetWide(), GetTall() );
}

void CInventoryPanel::OnKeyCodePressed( vgui::KeyCode code )
{
	// ESC closes the inventory (the original's frame handled this too).
	if ( code == (vgui::KeyCode)KEY_ESCAPE )
	{
		SetVisible( false );
		return;
	}

	BaseClass::OnKeyCodePressed( code );
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
	if ( IsVisible() )
	{
		SetVisible( false );
	}
	else
	{
		// Refresh immediately when opened so the grid is never stale.
		ClearSlots();
		RefreshSlots();

		// Activate() shows the frame, moves it to the front and requests
		// focus — so ESC (and the titlebar close button) work right away.
		Activate();
	}
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
