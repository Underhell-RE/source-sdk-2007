//========= Copyright (c) 2008, Mxthe (Underhell). All rights reserved. ============//
//
// Purpose: Underhell ironsight — client side.
//
// There is intentionally NO client "ironsight_toggle" command here. The
// original does not register one either (the string only appears in a
// server-resync path, sub_100D8E90): the keybinding forwards "ironsight_toggle"
// to the server through the engine's client-command route (exactly like
// dropitem/useitem/switch), the server toggles the networked m_bIronSighted,
// and the client viewmodel slides by reading C_BaseHLPlayer::IsIronSighted()
// every frame in CalcViewModelView. A single source of truth avoids the
// client/server desync (and any double-execution) a locally-toggled flag
// would cause.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
