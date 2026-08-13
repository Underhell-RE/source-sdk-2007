//========= Copyright (c) 2008, Mxthe (Underhell). All rights reserved. ============//
//
// Purpose: Underhell inventory UI panel — implementation.
//
// Reconstructed 1:1 from the original client.dll (see docs/UNDERHELL.md):
//   * CInventoryPanel        — hexrays sub_1012EDC0 (ctor), sub_1012E360
//                              (slot layout), sub_1012E6C0 (OnThink)
//   * slots                  — vgui::DragnDropSlot : ImageButton : ImagePanel
//                              (sub_101310D0 / sub_10131BB0), 28x28, scaled
//   * layout                 — 4x6 column-major grid, origin (44,119), pitch 37
//                              + 4 extra slots at (82,118)/(82,81)/(343,118)/(343,81)
//   * sprites                — scheme GetImage("../Sprites/Hud/...")
//   * cl_inventoryToggle     — hexrays sub_1012E690
//   * "UpdateInventory" cmd  — hexrays sub_1012E660
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "c_basehlplayer.h"
#include "vgui/ISurface.h"
#include "vgui/IScheme.h"
#include "vgui_controls/controls.h"
#include "vgui_controls/Tooltip.h"
#include "inputsystem/buttoncode.h"
#include "c_inventory_panel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Per-id slot visuals. Sprite paths and localization tokens are the original
// strings (hexrays sub_1012E6C0 switch). vgui images are rooted at
// materials/vgui/, so the original prefixes "../" to reach materials/Sprites/.
// Glowstick icon colours for ids 14..18 do not match the print names — kept.
// Ids 24/25 (painkillers/syringe) have no sprite (empty image + tooltip text).
//-----------------------------------------------------------------------------
struct UHInventorySlotInfo_t
{
	const char *pszSprite;		// "../Sprites/Hud/Items/...", NULL = no image
	const char *pszTextToken;	// "#UnderHell_Inventory_..."
};

#define UH_INV_ITEM( _file )	"../Sprites/Hud/Items/" _file
#define UH_INV_BLANK			"../Sprites/Hud/Inventory/Blank"
#define UH_INV_BG				"../Sprites/Hud/Inventory/Inventory"

static const UHInventorySlotInfo_t s_InventorySlotInfo[UH_INVENTORY_ITEM_TABLE_SIZE] =
{
	{ NULL,								NULL },										// 0: none
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

// Layout constants from sub_1012E360. All go through
// ISchemeManager::GetProportionalScaledValue (640x480 base).
#define UH_SLOT_SIZE		28
#define UH_SLOT_PITCH		37
#define UH_SLOT_ORIGIN_X	44
#define UH_SLOT_ORIGIN_Y	119
#define UH_SLOT_COLS		4
#define UH_SLOT_ROWS		6

// Extra slots 24..27 (this+134 in the original). Order = the four SetPos
// calls after the grid loop: (82,118), (82,81), (343,118), (343,81).
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
	// Original (hexrays sub_1012E690): ask the server for a resync, then flip
	// the panel's visibility. No FCVAR_CLIENTCMD_CAN_EXECUTE on the client's
	// UpdateInventory, so the ClientCmd string is forwarded to the server —
	// the original's exact message flow.
	engine->ClientCmd( "UpdateInventory" );

	if ( GetInventoryPanel() )
	{
		GetInventoryPanel()->Toggle();
	}
}

CON_COMMAND( UpdateInventory, "Updates the inventory" )
{
	// Original (hexrays sub_1012E660): flags a full refresh.
	// Executed locally when the server sends it via engine->ClientCommand.
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
	SetShouldScaleImage( true );
	SetPaintBorderEnabled( false );
	SetMouseInputEnabled( true );
	Clear();
}

void CInventorySlotPanel::SetSlotContents( const char *pszSprite, const char *pszTextToken )
{
	// Original ImageButton::SetImage via scheme GetImage. Empty / NULL sprite
	// leaves the slot blank (painkillers / syringe are text-only).
	if ( pszSprite && pszSprite[0] )
	{
		SetImage( pszSprite );
	}
	else
	{
		SetImage( UH_INV_BLANK );
	}

	// Tooltip carries the localized name + description (Underhell_english.txt
	// tokens are multi-line). Original wrote this through the slot's text
	// helper (sub_1025DAD0) onto the shared tooltip panel.
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
	// Original first paints the blank sprite on every slot, then fills the
	// non-empty ones (hexrays sub_1012E6C0) and hides the tooltip.
	SetImage( UH_INV_BLANK );
	GetTooltip()->SetText( "" );
	GetTooltip()->HideTooltip();
}

//-----------------------------------------------------------------------------
// CInventoryPanel
//-----------------------------------------------------------------------------
CInventoryPanel::CInventoryPanel( vgui::VPANEL parent )
	: BaseClass( NULL, "InventoryPanel", true )
{
	SetParent( parent );

	// Ctor sequence from hexrays sub_1012EDC0.
	SetVisible( false );
	SetEnabled( true );
	SetSizeable( false );
	SetMoveable( false );
	SetMenuButtonVisible( false );
	SetCloseButtonVisible( false );
	SetMinimizeButtonVisible( false );
	SetMaximizeButtonVisible( false );
	SetProportional( true );

	vgui::HScheme hScheme = vgui::scheme()->LoadSchemeFromFile( "resource/SourceScheme.res", "SourceScheme" );
	if ( hScheme )
	{
		SetScheme( hScheme );
	}

	// Background is the original Texture1 ("Sprites/Hud/Inventory/Inventory").
	// OB-era Frame has no Texture1 key, so a full-size ImagePanel stands in.
	m_pBackground = new vgui::ImagePanel( this, "InventoryBackground" );
	m_pBackground->SetImage( UH_INV_BG );
	m_pBackground->SetShouldScaleImage( true );
	m_pBackground->SetMouseInputEnabled( false );
	m_pBackground->SetZPos( -1 );

	// 28 slots named UHImage1.. (original increments a "UHImage0" buffer).
	// Each is constructed with the blank sprite, like sub_101310D0.
	for ( int i = 0; i < UH_INVENTORY_SLOTS; ++i )
	{
		char szName[32];
		Q_snprintf( szName, sizeof( szName ), "UHImage%d", i + 1 );
		m_pSlots[i] = new CInventorySlotPanel( this, szName );
	}

	LoadControlSettings( "resource/UI/InventoryPanel.res" );

	// .res sets settitlebarvisible 1 / title #Frame_Untitled. The original
	// Frame understands Texture1 as the chrome; ours would draw a real
	// title bar over the art. Hide it after the .res applies.
	SetTitleBarVisible( false );
	SetMenuButtonVisible( false );
	SetCloseButtonVisible( false );
	SetMinimizeButtonVisible( false );
	SetMaximizeButtonVisible( false );
	SetSizeable( false );

	// Fallback in case the .res is missing from a mod install.
	if ( GetWide() < 100 )
	{
		SetBounds( 368, 84, 1024, 512 );
	}

	// Original ctor colours the frame ARGB(128, 255, 255, 255)
	// (hexrays sub_1012EDC0 / sub_10237580(-2130706433)).
	SetBgColor( Color( 255, 255, 255, 128 ) );

	LayoutSlots();

	m_bNeedsRefresh = true;
	SetVisible( false );

	DevMsg( "InventoryPanel has been constructed\n" );
}

CInventoryPanel::~CInventoryPanel( void )
{
	s_pInventoryPanel = NULL;
}

void CInventoryPanel::LayoutSlots( void )
{
	// hexrays sub_1012E360. Every length goes through
	// ISchemeManager::GetProportionalScaledValue.
	const int iSize = vgui::scheme()->GetProportionalScaledValue( UH_SLOT_SIZE );
	const int iPitch = vgui::scheme()->GetProportionalScaledValue( UH_SLOT_PITCH );
	const int iOriginX = vgui::scheme()->GetProportionalScaledValue( UH_SLOT_ORIGIN_X );
	const int iOriginY = vgui::scheme()->GetProportionalScaledValue( UH_SLOT_ORIGIN_Y );

	// 4x6 grid, column-major: slot index = row + col*6.
	// Inner loop steps +6 through the pointer array (v1 += 6).
	for ( int iRow = 0; iRow < UH_SLOT_ROWS; ++iRow )
	{
		for ( int iCol = 0; iCol < UH_SLOT_COLS; ++iCol )
		{
			const int iSlot = iRow + iCol * UH_SLOT_ROWS;
			m_pSlots[iSlot]->SetShouldScaleImage( true );
			m_pSlots[iSlot]->SetSize( iSize, iSize );
			m_pSlots[iSlot]->SetPos( iOriginX + iCol * iPitch, iOriginY + iRow * iPitch );
			m_pSlots[iSlot]->SetVisible( true );
		}
	}

	for ( int i = 0; i < 4; ++i )
	{
		const int iX = vgui::scheme()->GetProportionalScaledValue( s_nExtraSlotPos[i][0] );
		const int iY = vgui::scheme()->GetProportionalScaledValue( s_nExtraSlotPos[i][1] );
		m_pSlots[UH_SLOT_COLS * UH_SLOT_ROWS + i]->SetShouldScaleImage( true );
		m_pSlots[UH_SLOT_COLS * UH_SLOT_ROWS + i]->SetSize( iSize, iSize );
		m_pSlots[UH_SLOT_COLS * UH_SLOT_ROWS + i]->SetPos( iX, iY );
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
	LayoutSlots();
}

void CInventoryPanel::PaintBackground( void )
{
	// Don't paint Frame chrome. The Inventory.vtf child is the background
	// (original Texture1 / PaintBackgroundType 1).
}

void CInventoryPanel::OnKeyCodePressed( vgui::KeyCode code )
{
	if ( code == (vgui::KeyCode)KEY_ESCAPE ||
		 code == (vgui::KeyCode)KEY_I )
	{
		Toggle();
		return;
	}

	BaseClass::OnKeyCodePressed( code );
}

void CInventoryPanel::OnKeyCodeTyped( vgui::KeyCode code )
{
	// Frame's default ESC path calls Close() (fade/destroy). Just hide.
	if ( code == KEY_ESCAPE || code == KEY_I )
	{
		Toggle();
		return;
	}

	BaseClass::OnKeyCodeTyped( code );
}

void CInventoryPanel::OnClose( void )
{
	SetVisible( false );
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

	// Original gates the panel on the suit, the player's health and the
	// inventory flag (hexrays sub_1012E6C0: client player offsets 3681 =
	// suit bool and 136 = health).
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
	if ( IsVisible() )
	{
		SetVisible( false );
	}
	else
	{
		ClearSlots();
		RefreshSlots();
		m_bNeedsRefresh = false;

		SetVisible( true );
		MoveToFront();
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
		m_pSlots[i]->SetSlotContents( info.pszSprite, info.pszTextToken );
	}
}

//-----------------------------------------------------------------------------
// vgui messages the original panel handled. Slot selection / drag still TODO.
//-----------------------------------------------------------------------------
void CInventoryPanel::OnNewSelection( void )
{
}

void CInventoryPanel::OnNewMouseReleased( void )
{
}
