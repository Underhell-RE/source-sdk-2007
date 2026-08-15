//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements visual effects entities: sprites, beams, bubbles, etc.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "engine/IEngineSound.h"
#include "baseentity.h"
#include "entityoutput.h"
#include "recipientfilter.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CEnvHudHint : public CPointEntity
{
public:
	DECLARE_CLASS( CEnvHudHint, CPointEntity );

	void	Spawn( void );
	void	Precache( void );

private:
	void InputShowHudHint( inputdata_t &inputdata );
	void InputHideHudHint( inputdata_t &inputdata );
	// Underhell: show the hint text passed in the parameter.
	void InputHint( inputdata_t &inputdata );
	void InputHintThroughParameter( inputdata_t &inputdata );
	// Shared KeyHintText user-message helper.
	void ShowHintText( const char *pszHint );
	string_t m_iszMessage;
	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( env_hudhint, CEnvHudHint );

BEGIN_DATADESC( CEnvHudHint )

	DEFINE_KEYFIELD( m_iszMessage, FIELD_STRING, "message" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ShowHudHint", InputShowHudHint ),
	DEFINE_INPUTFUNC( FIELD_VOID, "HideHudHint", InputHideHudHint ),
	DEFINE_INPUTFUNC( FIELD_STRING, "InputHint", InputHint ),
	DEFINE_INPUTFUNC( FIELD_STRING, "InputHintThroughParameter", InputHintThroughParameter ),

END_DATADESC()



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvHudHint::Spawn( void )
{
	Precache();

	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvHudHint::Precache( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Send the KeyHintText user message with the given hint text.
//-----------------------------------------------------------------------------
void CEnvHudHint::ShowHintText( const char *pszHint )
{
	CBaseEntity *pPlayer = UTIL_GetLocalPlayer();
	if ( !pPlayer || !pPlayer->IsNetClient() )
		return;

	CSingleUserRecipientFilter user( (CBasePlayer *)pPlayer );
	user.MakeReliable();
	UserMessageBegin( user, "KeyHintText" );
		WRITE_BYTE( 1 );	// one message
		WRITE_STRING( pszHint );
	MessageEnd();
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for showing the message and/or playing the sound.
//-----------------------------------------------------------------------------
void CEnvHudHint::InputShowHudHint( inputdata_t &inputdata )
{
	ShowHintText( STRING(m_iszMessage) );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CEnvHudHint::InputHideHudHint( inputdata_t &inputdata )
{
	ShowHintText( "" );
}

//-----------------------------------------------------------------------------
// Purpose: Underhell extension — show the hint text passed in the parameter
// (independent of the entity's "message" keyvalue).
//-----------------------------------------------------------------------------
void CEnvHudHint::InputHint( inputdata_t &inputdata )
{
	ShowHintText( inputdata.value.String() );
}

//-----------------------------------------------------------------------------
// Purpose: Underhell — like InputHint, but the parameter is a localization
// token resolved through the message/titles system. TODO: verify the exact
// original resolution (currently the raw parameter is shown, which the client
// localizes via its '#' prefix).
//-----------------------------------------------------------------------------
void CEnvHudHint::InputHintThroughParameter( inputdata_t &inputdata )
{
	ShowHintText( inputdata.value.String() );
}
