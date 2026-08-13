//========= Copyright (c) 2008, Mxthe (Underhell). All rights reserved. ============//
//
// Purpose: Underhell inventory UI panel.
//
// Original: CInventoryPanel, a vgui::Frame loaded from
// "resource/UI/InventoryPanel.res" (1024x512 at 368,84), holding 28 slot
// panels — a 4x6 grid plus a row of 4 — refreshed from the local player's
// replicated m_iInventory.
//
// $NoKeywords: $
//=============================================================================//

#ifndef UH_C_INVENTORY_PANEL_H
#define UH_C_INVENTORY_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/Frame.h"
#include "vgui_controls/Label.h"

#include "underhell/uh_inventory.h"

//-----------------------------------------------------------------------------
// Interface exposed by the original panel (client-side inventory UI).
//-----------------------------------------------------------------------------
class IInventoryPanel
{
public:
	virtual ~IInventoryPanel() {}
};

//-----------------------------------------------------------------------------
// One inventory slot: 28x28 icon painted straight from the mod's HUD sprite
// material + a localized label. Drawing bypasses the scheme image lookup —
// the sprites live under "Sprites/Hud/Items/", not in vgui/.
//-----------------------------------------------------------------------------
class CInventorySlotPanel : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CInventorySlotPanel, vgui::Panel );

public:
	CInventorySlotPanel( vgui::Panel *pParent, const char *pszName );
	virtual ~CInventorySlotPanel();

	virtual void PerformLayout( void );
	virtual void PaintBackground( void );

	void SetSlotContents( const char *pszSprite, const char *pszTextToken );
	void Clear( void );
	void SetSelected( bool bSelected ) { m_bSelected = bSelected; }

private:
	const char		*m_pszSprite;	// static table pointer, NULL = nothing to draw
	vgui::Label		*m_pLabel;
	int				m_iTextureId;
	bool			m_bSelected;
};

//-----------------------------------------------------------------------------
// The inventory frame. Created lazily by GetInventoryPanel().
//-----------------------------------------------------------------------------
class CInventoryPanel : public vgui::Frame, public IInventoryPanel
{
	DECLARE_CLASS_SIMPLE( CInventoryPanel, vgui::Frame );

public:
	CInventoryPanel( vgui::VPANEL parent );
	virtual ~CInventoryPanel( void );

	virtual void PerformLayout( void );
	virtual void PaintBackground( void );
	virtual void OnThink( void );
	virtual void OnKeyCodePressed( vgui::KeyCode code );

	// cl_inventoryToggle — flips visibility and asks the server to resync.
	void Toggle( void );

	// Client "UpdateInventory" command — full refresh.
	void RequestRefresh( void )	{ m_bNeedsRefresh = true; }

private:
	void ClearSlots( void );
	void RefreshSlots( void );
	void SetSlot( int iSlot, const char *pszSprite, const char *pszTextToken );

	// Messages the original panel registered (hexrays sub_1012EC80/sub_1012ED20).
	MESSAGE_FUNC( OnNewSelection, "NewSelection" );
	MESSAGE_FUNC( OnNewMouseReleased, "NewMouseReleased" );

	CInventorySlotPanel		*m_pSlots[UH_INVENTORY_SLOTS];
	int						m_iBackgroundTextureId;
	bool					m_bNeedsRefresh;
};

// Singleton accessor — creates the panel on first use.
CInventoryPanel *GetInventoryPanel( void );

#endif // UH_C_INVENTORY_PANEL_H
