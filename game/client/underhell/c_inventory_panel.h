//========= Copyright (c) 2008, Mxthe (Underhell). All rights reserved. ============//
//
// Purpose: Underhell inventory UI panel.
//
// Original: CInventoryPanel : vgui::Frame, 28 vgui::DragnDropSlot children
// (ImageButton : ImagePanel). Layout from hexrays sub_1012E360.
//
// $NoKeywords: $
//=============================================================================//

#ifndef UH_C_INVENTORY_PANEL_H
#define UH_C_INVENTORY_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/Frame.h"
#include "vgui_controls/ImagePanel.h"

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
// One inventory slot. Original: vgui::DragnDropSlot : ImageButton : ImagePanel.
// We stand in with ImagePanel (OB SDK has no ImageButton / DragnDropSlot) —
// same 28x28 scaled sprite, same tooltip text. Drag-drop still TODO.
//-----------------------------------------------------------------------------
class CInventorySlotPanel : public vgui::ImagePanel
{
	DECLARE_CLASS_SIMPLE( CInventorySlotPanel, vgui::ImagePanel );

public:
	CInventorySlotPanel( vgui::Panel *pParent, const char *pszName );

	void SetSlotContents( const char *pszSprite, const char *pszTextToken );
	void Clear( void );
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
	virtual void OnKeyCodeTyped( vgui::KeyCode code );
	virtual void OnClose( void );

	// cl_inventoryToggle — flips visibility and asks the server to resync.
	void Toggle( void );

	// Client "UpdateInventory" command — full refresh.
	void RequestRefresh( void )	{ m_bNeedsRefresh = true; }

private:
	void LayoutSlots( void );
	void ClearSlots( void );
	void RefreshSlots( void );

	// Messages the original panel registered (hexrays sub_1012EC80/sub_1012ED20).
	MESSAGE_FUNC( OnNewSelection, "NewSelection" );
	MESSAGE_FUNC( OnNewMouseReleased, "NewMouseReleased" );

	CInventorySlotPanel		*m_pSlots[UH_INVENTORY_SLOTS];
	vgui::ImagePanel		*m_pBackground;
	bool					m_bNeedsRefresh;
};

// Singleton accessor — creates the panel on first use.
CInventoryPanel *GetInventoryPanel( void );

#endif // UH_C_INVENTORY_PANEL_H
