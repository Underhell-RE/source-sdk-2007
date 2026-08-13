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
// NOTE: "switch", "dropitem" and "useitem" are intentionally NOT registered
// as ConCommands — like the original they arrive through the engine's
// client-command route (unknown commands are forwarded to the server and
// dispatched by CHL2_Player::ClientCommand). "emit" stays in the vanilla
// base handler. Only "UpdateInventory" is a real ConCommand.
//-----------------------------------------------------------------------------
static CHL2_Player *UH_GetCommandPlayer( void )
{
	return dynamic_cast<CHL2_Player *>( UTIL_GetCommandClient() );
}

static void UH_CC_UpdateInventory( const CCommand &args )
{
	CHL2_Player *pPlayer = UH_GetCommandPlayer();
	if ( pPlayer )
		pPlayer->UH_UpdateInventory();
}

// TODO: verify the original registration flags (sub_102DDDE0's ConCommand
// initializer was not recovered from the binary).
static ConCommand uh_cc_update_inventory( "UpdateInventory", UH_CC_UpdateInventory, "Updates the inventory", FCVAR_CLIENTCMD_CAN_EXECUTE );

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
// Purpose: Add an item to the first free inventory slot.
// TODO: reconstruct the original add semantics exactly (stacking behaviour,
// full-inventory handling) from the decompiled pickup code.
//-----------------------------------------------------------------------------
void CHL2_Player::UH_AddInventoryItem( int iItem )
{
	if ( !UH_IsValidInventoryItem( iItem ) )
		return;

	for ( int i = 0; i < UH_INVENTORY_SLOTS; ++i )
	{
		if ( m_iInventory[i] == UH_ITEM_NONE )
		{
			m_iInventory.Set( i, iItem );
			return;
		}
	}

	// Inventory full.
	// TODO: original behaviour (deny sound? item dropped back to world?).
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
	int iItem = m_iInventory[iSlot];
	if ( !iItem )
	{
		Warning( "No Entity\n" );
		return false;
	}

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

	// Original reads the eye position and drops/uses the item right there.
	// TODO: confirm the yaw source field (original: float at player + 708).
	Vector vecOrigin = EyePosition();
	QAngle angItem( 0, EyeAngles().y - 90.0f, 0 );

	pItem->SetAbsOrigin( vecOrigin );
	pItem->SetAbsAngles( angItem );
	pItem->Spawn();

	if ( bUse )
	{
		bool bUsed = ( pItem->Use( this, this, USE_ON, (float)iItem ) != 0 );
		if ( !bUsed )
		{
			EmitSound( "HL2Player.UseDeny" );
			UTIL_Remove( pItem );
		}

		if ( UH_IsGlowstick( iItem ) )
		{
			// Using an unlit glowstick lights it in place of dropping it.
			m_iInventory.Set( iSlot, UH_GetLitGlowstickItem( iItem ) );
			engine->ClientCommand( edict(), "UpdateInventory" );
			return bUsed;
		}

		if ( !bUsed )
		{
			engine->ClientCommand( edict(), "UpdateInventory" );
			return false;
		}
	}
	else
	{
		// Drop path — style the spawned entity per id.
		switch ( iItem )
		{
		case UH_ITEM_FLARE_PACK:
			// Flare packs drop a lit flare prop instead of the pack model.
			UTIL_Remove( pItem );
			pItem = CreateEntityByName( "prop_physics" );
			pItem->SetModel( "models/PG_props/pg_obj/pg_flare.mdl" );
			pItem->SetAbsOrigin( vecOrigin );
			pItem->SetAbsAngles( angItem );
			pItem->Spawn();
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
			pItem->SetBodygroup( 0, UH_GetGlowstickBodyGroup( iItem ) );
			pItem->SetAbsOrigin( vecOrigin );
			pItem->SetAbsAngles( angItem );
			pItem->Spawn();
			break;

		default:
			{
				// TODO: original wrote the body group on a raw field (entity
				// offset +212) — verify the exact member (m_nBody? m_nSkin?).
				int iBodyGroup = UH_GetDropBodyGroup( iItem );
				if ( iBodyGroup >= 0 )
				{
					pItem->SetBodygroup( 0, iBodyGroup );
				}
			}
			break;
		}
	}

	UH_RemoveInventoryItem( iSlot );
	engine->ClientCommand( edict(), "UpdateInventory" );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Inventory command dispatcher. Runs from ClientCommand — the engine
// forwards unknown client commands here, exactly like the original (hexrays
// sub_102DDBF0). Returns true when the command was consumed.
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
