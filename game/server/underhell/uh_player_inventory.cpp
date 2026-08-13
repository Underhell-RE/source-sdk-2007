//========= Copyright (c) 2008, Mxthe (Underhell). All rights reserved. ============//
//
// Purpose: Underhell inventory — server-side CHL2_Player implementation.
//
// Reconstructed 1:1 from the original server.dll:
//   * UH_ItemAction             — hexrays sub_102E05F0 (vtable index 411)
//   * UH_UpdateInventory        — hexrays sub_102DDDE0
//   * UH_HandleInventoryCommand — hexrays sub_102DDBF0 (switch/dropitem/useitem)
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hl2_player.h"

#include "underhell/uh_inventory.h"

//-----------------------------------------------------------------------------
// Console commands.
//
// NOTE: "switch", "dropitem", "useitem" and "UpdateInventory" are
// intentionally NOT registered as ConCommands — exactly like the original,
// they arrive through the engine's client-command route (unknown commands are
// forwarded to the server and dispatched by CHL2_Player::ClientCommand).
// "emit" stays in the vanilla base handler.
//-----------------------------------------------------------------------------
static CHL2_Player *UH_GetCommandPlayer( void )
{
	return dynamic_cast<CHL2_Player *>( UTIL_GetCommandClient() );
}

//-----------------------------------------------------------------------------
// Dev/testing helper — not part of the original. Lets the inventory UI and
// use/drop flows be exercised without the (still stubbed) pickup logic.
//-----------------------------------------------------------------------------
CON_COMMAND_F( uh_give_item, "Adds an item to your inventory (dev/testing).", FCVAR_CHEAT )
{
	CHL2_Player *pPlayer = UH_GetCommandPlayer();
	if ( !pPlayer )
		return;

	if ( args.ArgC() < 2 )
	{
		Msg( "usage: uh_give_item <item id 1..%d>\n", UH_ITEM_MAX - 1 );
		return;
	}

	int iItem = atoi( args[1] );
	if ( !UH_IsValidInventoryItem( iItem ) )
	{
		Msg( "uh_give_item: invalid item id %d\n", iItem );
		return;
	}

	pPlayer->UH_AddInventoryItem( iItem );
	engine->ClientCommand( pPlayer->edict(), "UpdateInventory" );
}

//-----------------------------------------------------------------------------
// Item ids that get a body group when dropped into the world as an item model.
// Original mapping (hexrays sub_102E05F0 switch): apple/soda colours.
//-----------------------------------------------------------------------------
static int UH_GetDropBodyGroup( int iItem )
{
	switch ( iItem )
	{
	case UH_ITEM_APPLE_RED:			// Apple (Red)
	case UH_ITEM_SODA_FIRST:		// Soda flavour 1
		return 0;
	case UH_ITEM_APPLE_GREEN:		// Apple (Green)
	case UH_ITEM_SODA_FIRST + 1:	// Soda flavour 2
		return 1;
	case UH_ITEM_SODA_FIRST + 2:
		return 2;
	case UH_ITEM_SODA_FIRST + 3:
		return 3;
	case UH_ITEM_SODA_FIRST + 4:
		return 4;
	case UH_ITEM_SODA_FIRST + 5:
		return 5;
	default:
		return -1;	// no body group change
	}
}

//-----------------------------------------------------------------------------
// Purpose: Initial inventory state. The original constructor zeroes the array
// and forces m_bInventoryEnabled to 1 (hexrays sub_102E0350).
//-----------------------------------------------------------------------------
void CHL2_Player::UH_InitializeInventory( void )
{
	for ( int i = 0; i < UH_INVENTORY_SLOTS; ++i )
	{
		m_iInventory.Set( i, UH_ITEM_NONE );
	}

	m_bShoulderFlashlight = false;
	m_iUHBatteryCount = 0;
	m_iUHHermitCardsCount = 0;
	m_iUHHermitCurrentQuestCount = 0;
	m_iUHHermitTotalQuestCount = 0;
	m_bDisplayHermitCard = false;
	m_bFlashlightOn = false;
	m_bInventoryEnabled = true;
}

//-----------------------------------------------------------------------------
// Purpose: Spawn the world entity for an inventory item in front of the
// player (eye position + forward*56 + up*64, angled to (0, eye yaw - 90, 0)),
// styled per id. Shared by UH_GiveItem (vtable [410], drop-on-full) and
// UH_ItemAction (vtable [411], dropitem) — the original has the same
// per-id switch in both.
//-----------------------------------------------------------------------------
void CHL2_Player::UH_SpawnItemInWorld( int iItem )
{
	const char *pszClass = UH_GetInventoryItemClass( iItem );
	CBaseEntity *pItem = CreateEntityByName( pszClass );
	if ( !pItem )
	{
		Warning( "Unable to create entity?!\n" );
		return;
	}

	// Original: origin = eye + forward*56 + up*64 (sub_102DE310).
	Vector vecForward;
	EyeVectors( &vecForward );

	Vector vecOrigin = EyePosition() + vecForward * 56.0f + Vector( 0, 0, 64.0f );
	QAngle angItem( 0, EyeAngles().y - 90.0f, 0 );

	// Per-id styling (original switch, sub_102E05F0 / sub_102DE310).
	switch ( iItem )
	{
	case UH_ITEM_FLARE_PACK:
		// Flare packs drop a lit flare prop instead of the pack model.
		UTIL_Remove( pItem );
		pItem = CreateEntityByName( "prop_physics" );
		pItem->SetModel( "models/PG_props/pg_obj/pg_flare.mdl" );
		break;

	case UH_ITEM_GLOWSTICK_FIRST:
	case UH_ITEM_GLOWSTICK_YELLOW:
	case UH_ITEM_GLOWSTICK_GREEN:
	case UH_ITEM_GLOWSTICK_BLUE:
	case UH_ITEM_GLOWSTICK_LAST:
		// Glowsticks drop as coloured lit props.
		UTIL_Remove( pItem );
		pItem = CreateEntityByName( "prop_physics" );
		pItem->SetModel( "models/PG_props/pg_obj/pg_glow_stick.mdl" );
		static_cast<CBaseAnimating *>( pItem )->SetBodygroup( 0, UH_GetGlowstickBodyGroup( iItem ) );
		break;

	default:
		{
			// TODO: original wrote the body group on a raw field (entity
			// offset +212) — verify the exact member (m_nBody? m_nSkin?).
			int iBodyGroup = UH_GetDropBodyGroup( iItem );
			if ( iBodyGroup >= 0 )
			{
				static_cast<CBaseAnimating *>( pItem )->SetBodygroup( 0, iBodyGroup );
			}
		}
		break;
	}

	pItem->SetAbsOrigin( vecOrigin );
	pItem->SetAbsAngles( angItem );
	pItem->Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Add an item to the inventory (original: CHL2_Player vtable [410],
// sub_102DE310). When every slot is taken the item is spawned back into the
// world in front of the player instead of vanishing.
//-----------------------------------------------------------------------------
void CHL2_Player::UH_GiveItem( int iItem )
{
	if ( !UH_IsValidInventoryItem( iItem ) )
		return;

	int iFreeSlot = UH_FindFreeSlot();
	if ( iFreeSlot < 0 )
	{
		// Inventory full — the original drops the item to the world.
		UH_SpawnItemInWorld( iItem );
		return;
	}

	m_iInventory.Set( iFreeSlot, iItem );
	engine->ClientCommand( edict(), "UpdateInventory" );
}

//-----------------------------------------------------------------------------
// Purpose: First free inventory slot (0-based), or -1 when full.
// Original helper sub_10171D30 returned a 1-based index with 28 as the
// "full" sentinel; the clean equivalent is used internally here.
//-----------------------------------------------------------------------------
int CHL2_Player::UH_FindFreeSlot( void ) const
{
	for ( int i = 0; i < UH_INVENTORY_SLOTS; ++i )
	{
		if ( m_iInventory[i] == UH_ITEM_NONE )
			return i;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Add an item to the first free inventory slot.
//-----------------------------------------------------------------------------
void CHL2_Player::UH_AddInventoryItem( int iItem )
{
	UH_GiveItem( iItem );
}

void CHL2_Player::UH_RemoveInventoryItem( int iSlot )
{
	if ( iSlot >= 0 && iSlot < UH_INVENTORY_SLOTS )
	{
		m_iInventory.Set( iSlot, UH_ITEM_NONE );
	}
}

int CHL2_Player::UH_FindInventoryItem( int iItem ) const
{
	for ( int i = 0; i < UH_INVENTORY_SLOTS; ++i )
	{
		if ( m_iInventory[i] == iItem )
			return i;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Use or drop the item in a slot (original: vtable index 411,
// called with (slot, 0) from "dropitem" and (slot, 1) from "useitem").
//
// Behaviour per id, decoded from hexrays sub_102E05F0:
//   * spawns the world entity named by the item's classname at the eye
//     position, angled to (0, eye yaw - 90, 0), then either Use()s it
//     (bUse) or styles it as a dropped prop (body groups / flare / glowstick)
//   * lit glowsticks (19..23) have no world entity — the slot is consumed
//     and the lit prop attached to the player is removed
//   * using an unlit glowstick (14..18) lights it: slot becomes id + 5
//   * the slot is cleared and the client is told to resync afterwards
//-----------------------------------------------------------------------------
bool CHL2_Player::UH_ItemAction( int iSlot, bool bUse )
{
	// ClientCommand arguments are untrusted. Validate before indexing the
	// network array so malformed `useitem`/`dropitem` commands cannot read
	// outside m_iInventory.
	if ( iSlot < 0 || iSlot >= UH_INVENTORY_SLOTS )
	{
		Warning( "Invalid inventory slot %d\n", iSlot );
		return false;
	}

	int iItem = m_iInventory[iSlot];
	if ( !iItem )
	{
		Warning( "No Entity\n" );
		return false;
	}

	if ( !bUse )
	{
		// Drop path — spawn the styled world item in front of the player
		// (shared with UH_GiveItem, like the original's duplicated switch).
		UH_SpawnItemInWorld( iItem );

		UH_RemoveInventoryItem( iSlot );
		engine->ClientCommand( edict(), "UpdateInventory" );
		return true;
	}

	// Use path (original sub_102E05F0): create the item entity, place it at
	// eye position + forward*56 + up*64, angled to (0, eye yaw - 90, 0),
	// then Use() it with the item id as the value.
	const char *pszClass = UH_GetInventoryItemClass( iItem );
	CBaseEntity *pItem = CreateEntityByName( pszClass );

	if ( !pItem )
	{
		if ( UH_IsLitGlowstick( iItem ) )
		{
			UH_RemoveInventoryItem( iSlot );

			// TODO: find the lit glowstick attached to the player and remove
			// it (original warns if none found — keep the exact wording).
			Warning( "Attempted to remove Lit glowstick, but did not find one on player! \n" );
		}
		else
		{
			Warning( "Unable to create entity?!\n" );
		}

		engine->ClientCommand( edict(), "UpdateInventory" );
		return false;
	}

	Vector vecForward;
	EyeVectors( &vecForward );

	Vector vecOrigin = EyePosition() + vecForward * 56.0f + Vector( 0, 0, 64.0f );
	QAngle angItem( 0, EyeAngles().y - 90.0f, 0 );

	pItem->SetAbsOrigin( vecOrigin );
	pItem->SetAbsAngles( angItem );
	pItem->Spawn();

	// NOTE: the SDK's CItem::Use() returns void, but the original binary
	// returned a success bool from vtable index 64 (played
	// "HL2Player.UseDeny" + removed the item on failure). Assume success
	// until the per-item use logic is implemented.
	// TODO: recreate the original success/failure semantics.
	pItem->Use( this, this, USE_ON, (float)iItem );

	if ( UH_IsGlowstick( iItem ) )
	{
		// Using an unlit glowstick lights it in place of dropping it.
		m_iInventory.Set( iSlot, UH_GetLitGlowstickItem( iItem ) );
		engine->ClientCommand( edict(), "UpdateInventory" );
		return true;
	}

	UH_RemoveInventoryItem( iSlot );
	engine->ClientCommand( edict(), "UpdateInventory" );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Pure inventory command dispatcher. Only inventory commands here —
// objective / signaling commands live in uh_player_objectives.cpp for
// code quality / portability (see CHL2_Player::UH_HandleObjectiveCommand).
// Original: hexrays sub_102DDBF0 for switch/dropitem/useitem.
//-----------------------------------------------------------------------------
bool CHL2_Player::UH_HandleInventoryCommand( const CCommand &args )
{
	const char *pszCommand = args[0];

	if ( !Q_stricmp( pszCommand, "switch" ) )
	{
		if ( args.ArgC() > 2 )
		{
			int iSlotA = atoi( args[1] );
			int iSlotB = atoi( args[2] );

			if ( iSlotA >= 0 && iSlotA < UH_INVENTORY_SLOTS &&
				 iSlotB >= 0 && iSlotB < UH_INVENTORY_SLOTS )
			{
				int iTemp = m_iInventory[iSlotA];
				m_iInventory.Set( iSlotA, m_iInventory[iSlotB] );
				m_iInventory.Set( iSlotB, iTemp );

				engine->ClientCommand( edict(), "UpdateInventory" );
			}
		}
		return true;
	}

	if ( !Q_stricmp( pszCommand, "dropitem" ) )
	{
		if ( args.ArgC() > 1 )
		{
			UH_ItemAction( atoi( args[1] ), false );
		}
		return true;
	}

	if ( !Q_stricmp( pszCommand, "useitem" ) )
	{
		if ( args.ArgC() > 1 )
		{
			UH_ItemAction( atoi( args[1] ), true );
		}
		return true;
	}

	if ( !Q_stricmp( pszCommand, "UpdateInventory" ) )
	{
		UH_UpdateInventory();
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: "UpdateInventory" server command (FCVAR_CLIENTCMD_CAN_EXECUTE).
// Original (hexrays sub_102DDDE0):
//   * marks every non-empty slot as network-changed so the owning client
//     receives the current contents,
//   * removes the item the player is currently holding in hands,
//   * tells the client to run "UpdateInventory" locally (refreshes the UI).
//-----------------------------------------------------------------------------
void CHL2_Player::UH_UpdateInventory( void )
{
	for ( int i = 0; i < UH_INVENTORY_SLOTS; ++i )
	{
		if ( m_iInventory[i] != UH_ITEM_NONE )
		{
			// GetForModify() forces a network state change for the element
			// (original called StateChanged( 4928 + 4*i ) directly).
			m_iInventory.GetForModify( i );
		}
	}

	// TODO: remove the held item entity (original stored an EHANDLE at
	// player offset 2164, called a virtual on it, then removed the entity
	// and reset the handle). Needs the CBasePlayer held-item member.

	engine->ClientCommand( edict(), "UpdateInventory" );
}
