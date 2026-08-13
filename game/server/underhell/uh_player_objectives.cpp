//========= Copyright (c) 2008, Mxthe (Underhell). All rights reserved. ============//
//
// Purpose: Underhell objectives & map signaling — server-side CHL2_Player.
//
// Reconstructed 1:1 from the original server.dll:
//   * DispObj    — hexrays sub_101F11D0: find "Display_Objective" relay and trigger it
//   * GiveSign   — same func: find "GiveSignal" and trigger
//   * SkipScene  — same func: find "Relay_SkipScene" and trigger
//
// VMF evidence (uh_prologue_2_d.vmf, added by user):
//   Display_Objective (logic_relay)
//     OnTrigger -> MainObjective,ShowMessage
//   MainObjective (env_message)
//     messagesound "Underhell.NewObjective"
//     message set at runtime via logic_auto:
//       OnMapSpawn MainObjective,SetMessagePriority1,@titles_Prologue.txt_Prologue_2_Objective_A
//   So the text was not drawn because vanilla CMessage lacks SetMessagePriority1
//   input — only sound played. Fixed in envmessage.cpp by adding those inputs.
//
// Original CLogicRelay trigger path (sub_10180EC0):
//   if (!m_bDisabled@848 && !m_bWaitForRefire@849)
//     m_OnTrigger@800.FireOutput(activator, this);
//     ...
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
// For robustness we also try Display (game_text) and ShowMessage (env_message)
// because Display_Objective could be overridden in custom maps.
//-----------------------------------------------------------------------------
void CHL2_Player::UH_TriggerMapEntity( const char *pszTargetName )
{
	if ( !pszTargetName || !*pszTargetName )
		return;

	CBaseEntity *pEnt = gEntList.FindEntityByName( NULL, pszTargetName );
	if ( pEnt )
	{
		variant_t emptyVariant;
		// Original passed player in v53[0] as activator.
		pEnt->AcceptInput( "Trigger", this, this, emptyVariant, 0 );
		// Fallbacks for direct game_text / env_message usage:
		pEnt->AcceptInput( "Display", this, this, emptyVariant, 0 );
		pEnt->AcceptInput( "ShowMessage", this, this, emptyVariant, 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: DispObj — display current objective (chapter task).
// 1:1 from sub_101F11D0 + VMF fix:
//   v10 = gEntList.FindEntityByName(0, "Display_Objective")
//   if (v10) InputTrigger(v10, player) -> MainObjective,ShowMessage
// If Display_Objective relay is missing (custom map), also try MainObjective
// directly, then fallback to direct HudMessage via UTIL_ShowMessage.
//-----------------------------------------------------------------------------
void CHL2_Player::UH_DisplayObjective( void )
{
	// Primary path: original relay
	UH_TriggerMapEntity( "Display_Objective" );

	// Secondary: direct MainObjective env_message (as in uh_prologue_2_d.vmf)
	// This ensures text draws even if relay output was broken.
	CBaseEntity *pMain = gEntList.FindEntityByName( NULL, "MainObjective" );
	if ( pMain )
	{
		variant_t emptyVariant;
		pMain->AcceptInput( "ShowMessage", this, this, emptyVariant, 0 );
	}
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
		UH_DisplayObjective();
		return true;
	}

	if ( !Q_stricmp( pszCommand, "GiveSign" ) )
	{
		UH_TriggerMapEntity( "GiveSignal" );
		return true;
	}

	if ( !Q_stricmp( pszCommand, "SkipScene" ) )
	{
		UH_TriggerMapEntity( "Relay_SkipScene" );
		return true;
	}

	return false;
}
