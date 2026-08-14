//========= Copyright (c) 2008, Mxthe (Underhell). All rights reserved. ============//
//
// Purpose: Underhell ironsight — client-side.
//
// "ironsight_toggle" is a client command: it flips the local viewmodel's
// m_bExpSighted (so CalcViewModelView slides the gun to the eye by the
// weapon's ExpOffset) and forwards the command to the server, which toggles
// the networked m_bIronSighted (accuracy + FOV zoom) and plays the sound.
// Model: VDC "Adding Ironsights" (jorg40/Cin) + the note about propagating
// the state to the server.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "c_basehlplayer.h"
#include "baseviewmodel_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CON_COMMAND( ironsight_toggle, "Toggles ironsight for the current weapon." )
{
	C_BaseHLPlayer *pPlayer = dynamic_cast<C_BaseHLPlayer *>( C_BasePlayer::GetLocalPlayer() );
	if ( !pPlayer )
		return;

	// Toggle the local viewmodel slide immediately (server FOV/accuracy
	// follows via the forwarded command below).
	C_BaseViewModel *pViewModel = pPlayer->GetViewModel();
	if ( pViewModel )
	{
		pViewModel->m_bExpSighted = !pViewModel->m_bExpSighted;
	}

	// Let the server toggle m_bIronSighted + FOV zoom + sound.
	engine->ClientCmd( "ironsight_toggle" );
}
