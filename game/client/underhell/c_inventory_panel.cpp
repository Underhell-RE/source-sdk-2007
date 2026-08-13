//========= Copyright (c) 2008, Mxthe (Underhell). All rights reserved. ============//
//
// Purpose: Underhell inventory UI panel — implementation.
//
// Reconstructed 1:1 from the original client.dll:
//   * CInventoryPanel        — sub_1012EDC0 (ctor), sub_1012E360 (layout),
//                              sub_1012E6C0 (OnThink), sub_1012E2C0 (NewSelection),
//                              sub_1012E590 (OnCommand turnoff/useitem/dropitem)
//   * DragnDropSlot          — sub_101310D0 (ctor + ContextMenu Use/Drop),
//                              sub_10130D00 (LMB=107 NewSelection, RMB=108 menu),
//                              sub_10130DC0 (menu -> useitem/dropitem %i),
//                              sub_10130ED0 (LMB useitem — treated as double-click)
//   * layout                 — 1024x512 Inventory.vtf at native pixels.
//                              Grid is 6 columns x 4 rows (outer v34=0..5 is X,
//                              inner i=0..3 is Y, v1+=6 walks down a column).
//                              Slot index = row*6 + col, so items 0..5 fill
//                              left-to-right. Origin (44,119), pitch 37, 28x28.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "c_basehlplayer.h"
#include "vgui/ISurface.h"
#include "vgui/IScheme.h"
#include "vgui/IInput.h"
#include "vgui_controls/controls.h"
#include "vgui_controls/Tooltip.h"
#include "vgui_controls/Menu.h"
#include "inputsystem/buttoncode.h"
#include "c_inventory_panel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Per-id slot visuals. Sprite paths and localization tokens are the original
// strings (hexrays sub_1012E6C0). vgui images are rooted at materials/vgui/,
// so the original prefixes "../" to reach materials/Sprites/.
//-----------------------------------------------------------------------------
struct UHInventorySlotInfo_t
{
	const char *pszSprite;
	const char *pszTextToken;
};

#define UH_INV_ITEM( _file )	"../Sprites/Hud/Items/" _file
#define UH_INV_BLANK			"../Sprites/Hud/Inventory/Blank"
#define UH_INV_BG				"../Sprites/Hud/Inventory/Inventory"

static const UHInventorySlotInfo_t s_InventorySlotInfo[UH_INVENTORY_ITEM_TABLE_SIZE] =
{
	{ NULL,								NULL },										// 0
	{ UH_INV_ITEM( "AppleRed" ),		"#UnderHell_Inventory_Apple" },				// 1
	{ UH_INV_ITEM( "AppleGreen" ),		"#UnderHell_Inventory_Apple" },				// 2
	{ UH_INV_ITEM( "Banana" ),			"#UnderHell_Inventory_Banana" },			// 3
	{ UH_INV_ITEM( "Burritos" ),		"#UnderHell_Inventory_Burrito" },			// 4
	{ UH_INV_ITEM( "Sandwich" ),		"#UnderHell_Inventory_Sandwich" },			// 5
	{ UH_INV_ITEM( "Banana" ),			"#UnderHell_Inventory_BananaBunch" },		// 6
	{ UH_INV_ITEM( "Soda1" ),			"#UnderHell_Inventory_Soda" },				// 7
	{ UH_INV_ITEM( "Soda2" ),			"#UnderHell_Inventory_Soda" },				// 8
	{ UH_INV_ITEM( "Soda3" ),			"#UnderHell_Inventory_Soda" },				// 9
	{ UH_INV_ITEM( "Soda4" ),			"#UnderHell_Inventory_Soda" },				// 10
	{ UH_INV_ITEM( "Soda5" ),			"#UnderHell_Inventory_Soda" },				// 11
	{ UH_INV_ITEM( "SodaPowerPunch" ),	"#UnderHell_Inventory_MegaSoda" },			// 12
	{ UH_INV_ITEM( "Flares" ),			"#UnderHell_Inventory_Flare" },				// 13
	{ UH_INV_ITEM( "GlowstickGreen" ),	"#UnderHell_Inventory_Glowstick_Green" },	// 14
	{ UH_INV_ITEM( "GlowstickRed" ),		"#UnderHell_Inventory_Glowstick_Red" },		// 15
	{ UH_INV_ITEM( "GlowstickBlue" ),	"#UnderHell_Inventory_Glowstick_Blue" },	// 16
	{ UH_INV_ITEM( "GlowstickYellow" ),	"#UnderHell_Inventory_Glowstick_Yellow" },	// 17
	{ UH_INV_ITEM( "GlowstickPurple" ),	"#UnderHell_Inventory_Glowstick_Purple" },	// 18
	{ UH_INV_ITEM( "GlowstickGreenLit" ),"#UnderHell_Inventory_Glowstick_Green" },	// 19
	{ UH_INV_ITEM( "GlowstickRedLit" ),	"#UnderHell_Inventory_Glowstick_Red" },		// 20
	{ UH_INV_ITEM( "GlowstickBlueLit" ),	"#UnderHell_Inventory_Glowstick_Blue" },	// 21
	{ UH_INV_ITEM( "GlowstickYellowLit" ),"#UnderHell_Inventory_Glowstick_Yellow" },	// 22
	{ UH_INV_ITEM( "GlowstickPurpleLit" ),"#UnderHell_Inventory_Glowstick_Purple" },	// 23
	{ NULL,								"#UnderHell_Inventory_Painkillers" },		// 24
	{ NULL,								"#UnderHell_Inventory_Syringe" },			// 25
	{ UH_INV_ITEM( "Bandages" ),		"#UnderHell_Inventory_Bandages" },			// 26
	{ UH_INV_ITEM( "HealthKit" ),		"#UnderHell_Inventory_HealthKit" },			// 27
	{ UH_INV_ITEM( "HealthSpray" ),		"#UnderHell_Inventory_HealthVial" },		// 28
	{ UH_INV_ITEM( "Chocobar" ),			"#UnderHell_Inventory_Chocobar" },			// 29
	{ UH_INV_ITEM( "Orange" ),			"#UnderHell_Inventory_Orange" },			// 30
	{ UH_INV_ITEM( "FMRadios" ),			"#UnderHell_Inventory_FMRadio" },			// 31
	{ UH_INV_ITEM( "RadioCrackers" ),	"#UnderHell_Inventory_RadioCracker" },		// 32
};

// sub_1012E360 constants. Applied in *pixels* so they sit on the 1024x512 art.
#define UH_SLOT_SIZE		28
#define UH_SLOT_PITCH		37
#define UH_SLOT_ORIGIN_X	44
#define UH_SLOT_ORIGIN_Y	119
#define UH_SLOT_COLS		6		// outer loop v34 < 6 — X
#define UH_SLOT_ROWS		4		// inner loop i < 4  — Y

// Extra slots 24..27, order of the four SetPos calls after the grid loop.
static const int s_nExtraSlotPos[4][2] =
{
	{  82, 118 },
	{  82,  81 },
	{ 343, 118 },
	{ 343,  81 },
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
CON_COMMAND( cl_inventoryToggle, "Toggles the inventory." )
{
	engine->ClientCmd( "UpdateInventory" );

	if ( GetInventoryPanel() )
	{
		GetInventoryPanel()->Toggle();
	}
}

CON_COMMAND( UpdateInventory, "Updates the inventory" )
{
	if ( GetInventoryPanel() )
	{
		GetInventoryPanel()->RequestRefresh();
	}
}

//-----------------------------------------------------------------------------
// CInventorySlotPanel
//-----------------------------------------------------------------------------
CInventorySlotPanel::CInventorySlotPanel( vgui::Panel *pParent, const char *pszName, int iSlot )
	: BaseClass( pParent, pszName )
{
	m_iSlot = iSlot;
	m_iItem = 0;
	m_bSelected = false;
	m_pContextMenu = NULL;

	SetShouldScaleImage( true );
	SetPaintBorderEnabled( false );
	SetMouseInputEnabled( true );

	// Original sub_101310D0: Menu "ContextMenu" with items "Use"/"Drop"
	// commanding "MyUse"/"MyDrop" back to this slot.
	m_pContextMenu = new vgui::Menu( this, "ContextMenu" );
	m_pContextMenu->AddMenuItem( "Use", "Use", "MyUse", this );
	m_pContextMenu->AddMenuItem( "Drop", "Drop", "MyDrop", this );
	m_pContextMenu->SetVisible( false );

	Clear();
}

void CInventorySlotPanel::SetSlotContents( int iItem, const char *pszSprite, const char *pszTextToken )
{
	m_iItem = iItem;

	if ( pszSprite && pszSprite[0] )
	{
		SetImage( pszSprite );
	}
	else
	{
		SetImage( UH_INV_BLANK );
	}

	if ( pszTextToken && pszTextToken[0] )
	{
		GetTooltip()->SetText( pszTextToken );
		GetTooltip()->SetTooltipFormatToMultiLine();
	}
	else
	{
		GetTooltip()->SetText( "" );
		GetTooltip()->HideTooltip();
	}
}

void CInventorySlotPanel::Clear( void )
{
	m_iItem = 0;
	m_bSelected = false;
	SetImage( UH_INV_BLANK );
	GetTooltip()->SetText( "" );
	GetTooltip()->HideTooltip();
}

void CInventorySlotPanel::SetSelected( bool bSelected )
{
	m_bSelected = bSelected;
}

void CInventorySlotPanel::OpenContextMenu( void )
{
	if ( !m_iItem || !m_pContextMenu )
		return;

	vgui::Menu::PlaceContextMenu( this, m_pContextMenu );
}

void CInventorySlotPanel::IssueItemCommand( const char *pszCommand )
{
	char szCmd[32];
	Q_snprintf( szCmd, sizeof( szCmd ), "%s %d", pszCommand, m_iSlot );
	// Original: engine vtable +24 (ClientCmd) with from-client = 1.
	engine->ClientCmd( szCmd );

	CInventoryPanel *pParent = dynamic_cast<CInventoryPanel *>( GetParent() );
	if ( pParent )
	{
		pParent->RequestRefresh();
	}

	GetTooltip()->SetText( "" );
	GetTooltip()->HideTooltip();
}

void CInventorySlotPanel::OnMousePressed( vgui::MouseCode code )
{
	// sub_10130D00: 107 = MOUSE_LEFT -> NewSelection; 108 = MOUSE_RIGHT -> menu.
	if ( !m_iItem )
		return;

	CInventoryPanel *pParent = dynamic_cast<CInventoryPanel *>( GetParent() );

	if ( code == MOUSE_LEFT )
	{
		if ( pParent )
		{
			pParent->SelectSlot( m_iSlot );
		}
		// Original LMB only selected. The Use/Drop choice the player sees is
		// the ContextMenu (RMB). Also open it on LMB so a click presents Use
		// or Drop — that's the menu the original built in the slot ctor.
		OpenContextMenu();
	}
	else if ( code == MOUSE_RIGHT )
	{
		if ( pParent )
		{
			pParent->SelectSlot( m_iSlot );
		}
		OpenContextMenu();
	}
}

void CInventorySlotPanel::OnMouseDoublePressed( vgui::MouseCode code )
{
	// sub_10130ED0: code 107 (MOUSE_LEFT) with a filled slot -> useitem %i.
	if ( code == MOUSE_LEFT && m_iItem )
	{
		IssueItemCommand( "useitem" );
	}
}

void CInventorySlotPanel::OnCommand( const char *command )
{
	// sub_10130DC0: menu item name "Use" / "Drop" -> useitem/dropitem %i.
	if ( !Q_stricmp( command, "MyUse" ) || !Q_stricmp( command, "Use" ) )
	{
		IssueItemCommand( "useitem" );
		return;
	}
	if ( !Q_stricmp( command, "MyDrop" ) || !Q_stricmp( command, "Drop" ) )
	{
		IssueItemCommand( "dropitem" );
		return;
	}

	BaseClass::OnCommand( command );
}

void CInventorySlotPanel::Paint( void )
{
	BaseClass::Paint();

	if ( m_bSelected )
	{
		int w, h;
		GetSize( w, h );
		vgui::surface()->DrawSetColor( 255, 200, 0, 220 );
		vgui::surface()->DrawOutlinedRect( 0, 0, w, h );
	}
}

//-----------------------------------------------------------------------------
// CInventoryPanel
//-----------------------------------------------------------------------------
CInventoryPanel::CInventoryPanel( vgui::VPANEL parent )
	: BaseClass( NULL, "InventoryPanel", true )
{
	SetParent( parent );

	m_iSelectedSlot = -1;
	m_bNeedsRefresh = true;
	m_flLastToggleTime = -1.0f;

	SetVisible( false );
	SetEnabled( true );
	SetSizeable( false );
	SetMoveable( false );
	SetMenuButtonVisible( false );
	SetCloseButtonVisible( false );
	SetMinimizeButtonVisible( false );
	SetMaximizeButtonVisible( false );
	// NOT proportional — the .res / Inventory.vtf is 1024x512 pixels.
	SetProportional( false );
	SetPaintBorderEnabled( false );

	m_pBackground = new vgui::ImagePanel( this, "InventoryBackground" );
	m_pBackground->SetImage( UH_INV_BG );
	m_pBackground->SetShouldScaleImage( true );
	m_pBackground->SetMouseInputEnabled( false );
	m_pBackground->SetZPos( -1 );

	for ( int i = 0; i < UH_INVENTORY_SLOTS; ++i )
	{
		char szName[32];
		Q_snprintf( szName, sizeof( szName ), "UHImage%d", i + 1 );
		m_pSlots[i] = new CInventorySlotPanel( this, szName, i );
	}

	LoadControlSettings( "resource/UI/InventoryPanel.res" );

	SetTitleBarVisible( false );
	SetMenuButtonVisible( false );
	SetCloseButtonVisible( false );
	SetMinimizeButtonVisible( false );
	SetMaximizeButtonVisible( false );
	SetSizeable( false );
	SetMoveable( false );

	// Force the native art size. .res may have been saved with proportional
	// values; ignore that and pin 1024x512.
	SetSize( 1024, 512 );

	// Center so 1024x512 is on-screen at 1280 and up. .res xpos/ypos (368,84)
	// was for a specific desktop and clips on 1280x720.
	int iScreenW, iScreenH;
	vgui::surface()->GetScreenSize( iScreenW, iScreenH );
	SetPos( max( 0, ( iScreenW - 1024 ) / 2 ), max( 0, ( iScreenH - 512 ) / 2 ) );

	SetBgColor( Color( 255, 255, 255, 128 ) );

	LayoutSlots();

	SetVisible( false );

	DevMsg( "InventoryPanel has been constructed\n" );
}

CInventoryPanel::~CInventoryPanel( void )
{
	s_pInventoryPanel = NULL;
}

void CInventoryPanel::LayoutSlots( void )
{
	// sub_1012E360: outer v34 = column 0..5, inner i = row 0..3, v1 += 6.
	// Row-major index (row*6 + col) so pickup order fills left-to-right
	// across the wide Inventory.vtf, not down a column.
	for ( int iRow = 0; iRow < UH_SLOT_ROWS; ++iRow )
	{
		for ( int iCol = 0; iCol < UH_SLOT_COLS; ++iCol )
		{
			const int iSlot = iRow * UH_SLOT_COLS + iCol;
			m_pSlots[iSlot]->SetShouldScaleImage( true );
			m_pSlots[iSlot]->SetSize( UH_SLOT_SIZE, UH_SLOT_SIZE );
			m_pSlots[iSlot]->SetPos( UH_SLOT_ORIGIN_X + iCol * UH_SLOT_PITCH,
									 UH_SLOT_ORIGIN_Y + iRow * UH_SLOT_PITCH );
			m_pSlots[iSlot]->SetVisible( true );
		}
	}

	for ( int i = 0; i < 4; ++i )
	{
		m_pSlots[UH_SLOT_COLS * UH_SLOT_ROWS + i]->SetShouldScaleImage( true );
		m_pSlots[UH_SLOT_COLS * UH_SLOT_ROWS + i]->SetSize( UH_SLOT_SIZE, UH_SLOT_SIZE );
		m_pSlots[UH_SLOT_COLS * UH_SLOT_ROWS + i]->SetPos( s_nExtraSlotPos[i][0], s_nExtraSlotPos[i][1] );
		m_pSlots[UH_SLOT_COLS * UH_SLOT_ROWS + i]->SetVisible( true );
	}

	if ( m_pBackground )
	{
		m_pBackground->SetBounds( 0, 0, GetWide(), GetTall() );
	}
}

void CInventoryPanel::PerformLayout( void )
{
	BaseClass::PerformLayout();

	// Keep the art at native 1024x512 even if Frame tries to resize us.
	if ( GetWide() != 1024 || GetTall() != 512 )
	{
		SetSize( 1024, 512 );
	}

	LayoutSlots();
}

void CInventoryPanel::PaintBackground( void )
{
	// Texture is the ImagePanel child (original Texture1).
}

void CInventoryPanel::OnKeyCodePressed( vgui::KeyCode code )
{
	// Do not Toggle here — OnKeyCodeTyped also fires for the same key,
	// and cl_inventoryToggle may fire too. One I would flip twice.
	BaseClass::OnKeyCodePressed( code );
}

void CInventoryPanel::OnKeyCodeTyped( vgui::KeyCode code )
{
	if ( code == KEY_ESCAPE )
	{
		if ( IsVisible() )
			Toggle();
		return;
	}

	BaseClass::OnKeyCodeTyped( code );
}

void CInventoryPanel::OnClose( void )
{
	SetVisible( false );
}

void CInventoryPanel::OnCommand( const char *command )
{
	// sub_1012E590: "turnoff" hides; "useitem"/"dropitem" fire for this[139].
	if ( !Q_stricmp( command, "turnoff" ) )
	{
		SetVisible( false );
		return;
	}

	if ( !Q_stricmp( command, "useitem" ) || !Q_stricmp( command, "dropitem" ) )
	{
		if ( m_iSelectedSlot >= 0 && m_iSelectedSlot < UH_INVENTORY_SLOTS )
		{
			char szCmd[32];
			Q_snprintf( szCmd, sizeof( szCmd ), "%s %d", command, m_iSelectedSlot );
			engine->ClientCmd( szCmd );
			m_bNeedsRefresh = true;
		}
		return;
	}

	BaseClass::OnCommand( command );
}

void CInventoryPanel::SelectSlot( int iSlot )
{
	// sub_1012E2C0: this[139] = NewSelection value.
	m_iSelectedSlot = iSlot;

	for ( int i = 0; i < UH_INVENTORY_SLOTS; ++i )
	{
		m_pSlots[i]->SetSelected( i == iSlot );
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

	if ( !pPlayer->IsSuitEquipped() || pPlayer->GetHealth() <= 0 )
	{
		SetVisible( false );
		return;
	}

	if ( !pPlayer->m_bInventoryEnabled )
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
	// Debounce: popup focus means I can hit both this Frame and the
	// cl_inventoryToggle binding in one press (open+close = stuck open).
	const float flNow = gpGlobals->curtime;
	if ( flNow >= 0.0f && m_flLastToggleTime >= 0.0f &&
		 ( flNow - m_flLastToggleTime ) < 0.20f )
	{
		return;
	}
	m_flLastToggleTime = flNow;

	if ( IsVisible() )
	{
		SetVisible( false );
	}
	else
	{
		ClearSlots();
		RefreshSlots();
		m_bNeedsRefresh = false;

		int iScreenW, iScreenH;
		vgui::surface()->GetScreenSize( iScreenW, iScreenH );
		SetSize( 1024, 512 );
		SetPos( max( 0, ( iScreenW - 1024 ) / 2 ), max( 0, ( iScreenH - 512 ) / 2 ) );

		SetVisible( true );
		MoveToFront();
	}
}

void CInventoryPanel::ClearSlots( void )
{
	m_iSelectedSlot = -1;
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
		m_pSlots[i]->SetSlotContents( iItem, info.pszSprite, info.pszTextToken );
	}
}

void CInventoryPanel::OnNewSelection( void )
{
}

void CInventoryPanel::OnNewMouseReleased( void )
{
}
