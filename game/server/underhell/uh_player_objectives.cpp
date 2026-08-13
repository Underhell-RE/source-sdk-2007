//========= Copyright (c) 2008, Mxthe (Underhell). All rights reserved. ============//
//
// Purpose: Underhell objectives & map signaling — server-side CHL2_Player.
//
// Reconstructed 1:1 from the original server.dll:
//   * DispObj    — hexrays sub_101F11D0: find "Display_Objective" relay and trigger it
//   * GiveSign   — same func: find "GiveSignal" and trigger
//   * SkipScene  — same func: find "Relay_SkipScene" and trigger
//
// Original CLogicRelay trigger path (sub_10180EC0):
//   if (!m_bDisabled@848 && !m_bWaitForRefire@849)
//     m_OnTrigger@800.FireOutput(activator, this);
//     if (spawnflags & SF_REMOVE_ON_FIRE) UTIL_Remove(this);
//     else if (!(flags & SF_ALLOW_FAST_RETRIGGER))
//       m_bWaitForRefire=true; AddEvent("EnableRefire", maxDelay+0.001)
//
// The Display_Objective relay is placed in every chapter map and its
// OnTrigger outputs drive the current objective HUD (game_text / env_message
// titles like #UnderHell_Chapter1_X_Objective_Y). The actual objective
// strings are stored in CBasePlayer::m_UHObjectives @2676 (0x200 bytes =
// 8 players * 16 slots * 4 bytes, zeroed in sub_101F77C0, synced in
// sub_101386C0/sub_10138B60/sub_10139380 via HudText/HudMsg).
//
// This file is intentionally separate from uh_player_inventory.cpp for
// code quality / portability. Inventory stays pure inventory.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hl2_player.h"

//-----------------------------------------------------------------------------
// Purpose: Generic helper — find entity by targetname and fire its Trigger
// input with this player as activator/caller. Mirrors the exact pattern
// used for DispObj/GiveSign/SkipScene in sub_101F11D0.
//
// Original used:
//   sub_1012BF20(&gEntList, 0, name, 0,0,0,0)  -> FindEntityByName
//   sub_10180EC0(entity, player)               -> InputTrigger
// AcceptInput("Trigger") performs the same disabled / wait checks.
//-----------------------------------------------------------------------------
void CHL2_Player::UH_TriggerMapEntity( const char *pszTargetName )
{
	if ( !pszTargetName || !*pszTargetName )
		return;

	CBaseEntity *pEnt = gEntList.FindEntityByName( NULL, pszTargetName );
	if ( pEnt )
	{
		variant_t emptyVariant;
		// Original passed player in v53[0] as activator. Reproduce by using
		// this as both activator and caller.
		pEnt->AcceptInput( "Trigger", this, this, emptyVariant, 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: DispObj — display current objective (chapter task).
// 1:1 from sub_101F11D0:
//   v10 = gEntList.FindEntityByName(0, "Display_Objective")
//   if (v10) InputTrigger(v10, player)
//-----------------------------------------------------------------------------
void CHL2_Player::UH_DisplayObjective( void )
{
	UH_TriggerMapEntity( "Display_Objective" );
}

//-----------------------------------------------------------------------------
// Purpose: Dispatcher for objective / signaling client commands.
// Called from CHL2_Player::ClientCommand, exactly like the original engine
// forwarded unknown client commands to CBasePlayer::ClientCommand.
//-----------------------------------------------------------------------------
bool CHL2_Player::UH_HandleObjectiveCommand( const CCommand &args )
{
	const char *pszCommand = args[0];

	if ( !Q_stricmp( pszCommand, "DispObj" ) )
	{
		// Reconstructed 1:1 — see header comment above. This is the command
		// that caused "Unknown command: DispObj" before.
		UH_DisplayObjective();
		return true;
	}

	if ( !Q_stricmp( pszCommand, "GiveSign" ) )
	{
		// Original: GiveSign -> GiveSignal
		UH_TriggerMapEntity( "GiveSignal" );
		return true;
	}

	if ( !Q_stricmp( pszCommand, "SkipScene" ) )
	{
		// Original: SkipScene -> Relay_SkipScene
		UH_TriggerMapEntity( "Relay_SkipScene" );
		return true;
	}

	return false;
}
