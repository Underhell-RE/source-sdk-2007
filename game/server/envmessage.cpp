//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements visual effects entities: sprites, beams, bubbles, etc.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "EnvMessage.h"
#include "engine/IEngineSound.h"
#include "KeyValues.h"
#include "filesystem.h"
#include "Color.h"
#include "GameStats.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( env_message, CMessage );

BEGIN_DATADESC( CMessage )

	DEFINE_KEYFIELD( m_iszMessage, FIELD_STRING, "message" ),
	DEFINE_KEYFIELD( m_sNoise, FIELD_SOUNDNAME, "messagesound" ),
	DEFINE_KEYFIELD( m_MessageAttenuation, FIELD_INTEGER, "messageattenuation" ),
	DEFINE_KEYFIELD( m_MessageVolume, FIELD_FLOAT, "messagevolume" ),

	DEFINE_FIELD( m_Radius, FIELD_FLOAT ),

	DEFINE_INPUTFUNC( FIELD_VOID, "ShowMessage", InputShowMessage ),
	DEFINE_INPUTFUNC( FIELD_STRING, "InputMessage", InputMessage ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetMessage", InputSetMessage ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetMessagePriority1", InputSetMessagePriority1 ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetMessagePriority2", InputSetMessagePriority2 ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetMessagePriority3", InputSetMessagePriority3 ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetMessagePriority4", InputSetMessagePriority4 ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetMessagePriority5", InputSetMessagePriority5 ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetMessagePriority6", InputSetMessagePriority6 ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetMessagePriority7", InputSetMessagePriority7 ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetMessagePriority8", InputSetMessagePriority8 ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetMessagePriority9", InputSetMessagePriority9 ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetMessagePriority10", InputSetMessagePriority10 ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetMessagePriority11", InputSetMessagePriority11 ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetMessagePriority12", InputSetMessagePriority12 ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetMessagePriority13", InputSetMessagePriority13 ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetMessagePriority14", InputSetMessagePriority14 ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetMessagePriority15", InputSetMessagePriority15 ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetMessagePriority16", InputSetMessagePriority16 ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RemoveMessagePriority", InputRemoveMessagePriority ),

	DEFINE_ARRAY( m_iszMessagesPriority, FIELD_STRING, 16 ),

	DEFINE_OUTPUT(m_OnShowMessage, "OnShowMessage"),

END_DATADESC()



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMessage::Spawn( void )
{
	Precache();

	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );

	switch( m_MessageAttenuation )
	{
	case 1: // Medium radius
		m_Radius = ATTN_STATIC;
		break;
	
	case 2:	// Large radius
		m_Radius = ATTN_NORM;
		break;

	case 3:	//EVERYWHERE
		m_Radius = ATTN_NONE;
		break;
	
	default:
	case 0: // Small radius
		m_Radius = SNDLVL_IDLE;
		break;
	}
	m_MessageAttenuation = 0;

	// Remap volume from [0,10] to [0,1].
	m_MessageVolume *= 0.1;

	// No volume, use normal
	if ( m_MessageVolume <= 0 )
	{
		m_MessageVolume = 1.0;
	}
}


void CMessage::Precache( void )
{
	if ( m_sNoise != NULL_STRING )
	{
		PrecacheScriptSound( STRING(m_sNoise) );
	}
}

//-----------------------------------------------------------------------------
// Underhell: helper to set message from string param.
// The original had 16 priority slots, VMF uses SetMessagePriority1 with
// "@titles_*.txt_*" syntax like "@titles_Prologue.txt_Prologue_2_Objective_A"
// which means: load titles_Prologue.txt and show entry Prologue_2_Objective_A.
// That entry's Message is "#UnderHell_Prologue_2_Objective_A" which is
// localized via Underhell_english.txt. We parse the @ reference to get the
// actual titles entry name so UTIL_ShowMessage can find it.
//-----------------------------------------------------------------------------
const char *CMessage::ParseTitlesReference( const char *pszInput, char *outBuf, int outBufSize )
{
	if ( !pszInput || !*pszInput )
		return pszInput;

	// Strip leading @ or #? Keep # for localization, but handle @titles case.
	if ( pszInput[0] == '@' )
	{
		// Look for ".txt_" or ".txt " separator
		const char *pszAfterTxt = Q_stristr( pszInput, ".txt_" );
		if ( pszAfterTxt )
		{
			pszAfterTxt += 5; // skip ".txt_"
			Q_strncpy( outBuf, pszAfterTxt, outBufSize );
			return outBuf;
		}
		pszAfterTxt = Q_stristr( pszInput, ".txt " );
		if ( pszAfterTxt )
		{
			pszAfterTxt += 5;
			Q_strncpy( outBuf, pszAfterTxt, outBufSize );
			return outBuf;
		}
		// Fallback: after last '_' maybe? For "@titles_Prologue.txt_Prologue_2_Objective_A"
		// we already handled .txt_ case. If format is different, take after last '/':
		const char *pszLastSlash = strrchr( pszInput, '/' );
		const char *pszStart = pszLastSlash ? pszLastSlash + 1 : pszInput + 1; // skip @
		// If still contains path, try to extract entry name
		Q_strncpy( outBuf, pszStart, outBufSize );
		// If outBuf contains ".txt_", extract after
		char *p = (char*)Q_stristr( outBuf, ".txt_" );
		if ( p )
		{
			Q_strncpy( outBuf, p + 5, outBufSize );
			return outBuf;
		}
		return outBuf;
	}
	// If starts with #, keep as is for localization, but also allow.
	return pszInput;
}

void CMessage::SetMessageFromString( const char *pszMessage )
{
	if ( !pszMessage || !*pszMessage )
		return;

	char szClean[512];
	const char *pszParsed = ParseTitlesReference( pszMessage, szClean, sizeof(szClean) );
	if ( !pszParsed )
		pszParsed = pszMessage;

	m_iszMessage = AllocPooledString( pszParsed );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for showing the message and/or playing the sound.
//-----------------------------------------------------------------------------
void CMessage::InputShowMessage( inputdata_t &inputdata )
{
	// Underhell extension: if priority messages are set, show highest priority
	// that is still valid. The VMF uses SetMessagePriority1 to set current
	// objective, so we check slots in order.
	const char *pszToShow = NULL;

	for ( int i = 0; i < 16; ++i )
	{
		if ( m_iszMessagesPriority[i] != NULL_STRING && STRING(m_iszMessagesPriority[i])[0] )
		{
			pszToShow = STRING(m_iszMessagesPriority[i]);
			break;
		}
	}

	if ( !pszToShow || !*pszToShow )
	{
		if ( m_iszMessage != NULL_STRING )
			pszToShow = STRING(m_iszMessage);
	}

	CBaseEntity *pPlayer = NULL;
	bool bHasPlayer = false;

	if ( m_spawnflags & SF_MESSAGE_ALL )
	{
		if ( pszToShow && *pszToShow )
			UTIL_ShowMessageAll( pszToShow );
	}
	else
	{
		if ( inputdata.pActivator && inputdata.pActivator->IsPlayer() )
		{
			pPlayer = inputdata.pActivator;
			bHasPlayer = true;
		}
		else
		{
			pPlayer = (gpGlobals->maxClients > 1) ? NULL : UTIL_GetLocalPlayer();
			bHasPlayer = (pPlayer != NULL);
		}

		if ( pPlayer && pPlayer->IsPlayer() )
		{
			if ( pszToShow && *pszToShow )
				UTIL_ShowMessage( pszToShow, ToBasePlayer( pPlayer ) );
		}
		else if ( !pPlayer )
		{
			if ( pszToShow && *pszToShow )
			{
				CBasePlayer *pLocal = UTIL_GetLocalPlayer();
				if ( pLocal )
					UTIL_ShowMessage( pszToShow, pLocal );
				else
					UTIL_ShowMessageAll( pszToShow );
			}
		}
	}

	// Fallback for Underhell: if TextMessageGet fails (titles_*.txt not loaded),
	// try to show localization token directly via center print. The titles entry
	// Prologue_2_Objective_A has Message "#UnderHell_Prologue_2_Objective_A" which
	// is in Underhell_english.txt. We construct "#UnderHell_<entry>" and send as
	// TextMsg so client localizes it. This ensures text renders even without
	// titles file loading, matching original behavior where sound played but
	// text didn't (user report).
	if ( pszToShow && *pszToShow )
	{
		// If pszToShow is already a localization token starting with #, keep it
		// Otherwise try to build Underhell token
		char szFallback[512];
		if ( pszToShow[0] == '#' )
		{
			Q_strncpy( szFallback, pszToShow, sizeof(szFallback) );
		}
		else
		{
			// Try #UnderHell_<entry> and also #<entry>
			// First try exact entry as localization? Actually most objectives are
			// UnderHell_<entry>
			Q_snprintf( szFallback, sizeof(szFallback), "#UnderHell_%s", pszToShow );
		}

		// Send as center print and as HudMessage fallback — both will be localized
		// by client if token exists in Underhell_english.txt
		if ( bHasPlayer && pPlayer )
		{
			ClientPrint( ToBasePlayer(pPlayer), HUD_PRINTCENTER, szFallback );
		}
		else
		{
			UTIL_ClientPrintAll( HUD_PRINTCENTER, szFallback );
		}

		// Also try direct HUD message with the fallback token using game_text style
		// In case TextMsg center print is not enough, also show via HudMessage with default parms
		// This mirrors original sub_10139380 which used HudText (sub_1025F270) and sound.
		{
			hudtextparms_t hparms;
			hparms.channel = 1;
			hparms.x = 0.1f;
			hparms.y = 0.1f;
			hparms.effect = 2;
			hparms.r1 = 165; hparms.g1 = 155; hparms.b1 = 30; hparms.a1 = 0;
			hparms.r2 = 255; hparms.g2 = 245; hparms.b2 = 115; hparms.a2 = 0;
			hparms.fadeinTime = 0.007f;
			hparms.fadeoutTime = 0.5f;
			hparms.holdTime = 3.5f;
			hparms.fxTime = 0.25f;

			// Try to get localized string directly via TextMessageGet for fallback token?
			// If not found, UTIL_HudMessage will still show raw token which vgui will localize.
			if ( pPlayer && pPlayer->IsPlayer() )
				UTIL_HudMessage( ToBasePlayer(pPlayer), hparms, szFallback );
			else
				UTIL_HudMessageAll( hparms, szFallback );
		}
	}

	if ( m_sNoise != NULL_STRING )
	{
		CPASAttenuationFilter filter( this );
		
		EmitSound_t ep;
		ep.m_nChannel = CHAN_BODY;
		ep.m_pSoundName = (char*)STRING(m_sNoise);
		ep.m_flVolume = m_MessageVolume;
		ep.m_SoundLevel = ATTN_TO_SNDLVL( m_Radius );

		EmitSound( filter, entindex(), ep );
	}

	if ( m_spawnflags & SF_MESSAGE_ONCE )
	{
		UTIL_Remove( this );
	}

	m_OnShowMessage.FireOutput( inputdata.pActivator, this );
}

//-----------------------------------------------------------------------------
// Purpose: SetMessage and priority variants — Underhell extension.
// Original binary had 16 separate inputs (InputSetMessagePriority1..16) plus
// SetMessage and RemoveMessagePriority. We implement them all as setting
// the main message and corresponding priority slot.
//-----------------------------------------------------------------------------
void CMessage::InputMessage( inputdata_t &inputdata )
{
	// Underhell house map: OnPressed Message,InputMessage,@titles_House.txt_House_Comment_Chest
	// Should set and show immediately
	InputSetMessage( inputdata );
	InputShowMessage( inputdata );
}

void CMessage::InputSetMessage( inputdata_t &inputdata )
{
	const char *psz = inputdata.value.String();
	if ( psz && *psz )
	{
		char szClean[512];
		const char *pszParsed = ParseTitlesReference( psz, szClean, sizeof(szClean) );
		m_iszMessage = AllocPooledString( pszParsed ? pszParsed : psz );
		m_iszMessagesPriority[0] = m_iszMessage;
	}
}

void CMessage::InputSetMessagePriority1( inputdata_t &inputdata ) { const char *psz = inputdata.value.String(); if ( psz && *psz ) { char szClean[512]; const char *p = ParseTitlesReference( psz, szClean, sizeof(szClean) ); m_iszMessagesPriority[0] = AllocPooledString( p ? p : psz ); m_iszMessage = m_iszMessagesPriority[0]; } }
void CMessage::InputSetMessagePriority2( inputdata_t &inputdata ) { const char *psz = inputdata.value.String(); if ( psz && *psz ) { char szClean[512]; const char *p = ParseTitlesReference( psz, szClean, sizeof(szClean) ); m_iszMessagesPriority[1] = AllocPooledString( p ? p : psz ); } }
void CMessage::InputSetMessagePriority3( inputdata_t &inputdata ) { const char *psz = inputdata.value.String(); if ( psz && *psz ) { char szClean[512]; const char *p = ParseTitlesReference( psz, szClean, sizeof(szClean) ); m_iszMessagesPriority[2] = AllocPooledString( p ? p : psz ); } }
void CMessage::InputSetMessagePriority4( inputdata_t &inputdata ) { const char *psz = inputdata.value.String(); if ( psz && *psz ) { char szClean[512]; const char *p = ParseTitlesReference( psz, szClean, sizeof(szClean) ); m_iszMessagesPriority[3] = AllocPooledString( p ? p : psz ); } }
void CMessage::InputSetMessagePriority5( inputdata_t &inputdata ) { const char *psz = inputdata.value.String(); if ( psz && *psz ) { char szClean[512]; const char *p = ParseTitlesReference( psz, szClean, sizeof(szClean) ); m_iszMessagesPriority[4] = AllocPooledString( p ? p : psz ); } }
void CMessage::InputSetMessagePriority6( inputdata_t &inputdata ) { const char *psz = inputdata.value.String(); if ( psz && *psz ) { char szClean[512]; const char *p = ParseTitlesReference( psz, szClean, sizeof(szClean) ); m_iszMessagesPriority[5] = AllocPooledString( p ? p : psz ); } }
void CMessage::InputSetMessagePriority7( inputdata_t &inputdata ) { const char *psz = inputdata.value.String(); if ( psz && *psz ) { char szClean[512]; const char *p = ParseTitlesReference( psz, szClean, sizeof(szClean) ); m_iszMessagesPriority[6] = AllocPooledString( p ? p : psz ); } }
void CMessage::InputSetMessagePriority8( inputdata_t &inputdata ) { const char *psz = inputdata.value.String(); if ( psz && *psz ) { char szClean[512]; const char *p = ParseTitlesReference( psz, szClean, sizeof(szClean) ); m_iszMessagesPriority[7] = AllocPooledString( p ? p : psz ); } }
void CMessage::InputSetMessagePriority9( inputdata_t &inputdata ) { const char *psz = inputdata.value.String(); if ( psz && *psz ) { char szClean[512]; const char *p = ParseTitlesReference( psz, szClean, sizeof(szClean) ); m_iszMessagesPriority[8] = AllocPooledString( p ? p : psz ); } }
void CMessage::InputSetMessagePriority10( inputdata_t &inputdata ) { const char *psz = inputdata.value.String(); if ( psz && *psz ) { char szClean[512]; const char *p = ParseTitlesReference( psz, szClean, sizeof(szClean) ); m_iszMessagesPriority[9] = AllocPooledString( p ? p : psz ); } }
void CMessage::InputSetMessagePriority11( inputdata_t &inputdata ) { const char *psz = inputdata.value.String(); if ( psz && *psz ) { char szClean[512]; const char *p = ParseTitlesReference( psz, szClean, sizeof(szClean) ); m_iszMessagesPriority[10] = AllocPooledString( p ? p : psz ); } }
void CMessage::InputSetMessagePriority12( inputdata_t &inputdata ) { const char *psz = inputdata.value.String(); if ( psz && *psz ) { char szClean[512]; const char *p = ParseTitlesReference( psz, szClean, sizeof(szClean) ); m_iszMessagesPriority[11] = AllocPooledString( p ? p : psz ); } }
void CMessage::InputSetMessagePriority13( inputdata_t &inputdata ) { const char *psz = inputdata.value.String(); if ( psz && *psz ) { char szClean[512]; const char *p = ParseTitlesReference( psz, szClean, sizeof(szClean) ); m_iszMessagesPriority[12] = AllocPooledString( p ? p : psz ); } }
void CMessage::InputSetMessagePriority14( inputdata_t &inputdata ) { const char *psz = inputdata.value.String(); if ( psz && *psz ) { char szClean[512]; const char *p = ParseTitlesReference( psz, szClean, sizeof(szClean) ); m_iszMessagesPriority[13] = AllocPooledString( p ? p : psz ); } }
void CMessage::InputSetMessagePriority15( inputdata_t &inputdata ) { const char *psz = inputdata.value.String(); if ( psz && *psz ) { char szClean[512]; const char *p = ParseTitlesReference( psz, szClean, sizeof(szClean) ); m_iszMessagesPriority[14] = AllocPooledString( p ? p : psz ); } }
void CMessage::InputSetMessagePriority16( inputdata_t &inputdata ) { const char *psz = inputdata.value.String(); if ( psz && *psz ) { char szClean[512]; const char *p = ParseTitlesReference( psz, szClean, sizeof(szClean) ); m_iszMessagesPriority[15] = AllocPooledString( p ? p : psz ); } }

void CMessage::InputRemoveMessagePriority( inputdata_t &inputdata )
{
	// If no param, clear all. If param is priority number, clear that slot.
	// Original removed by priority index, but for safety clear main and all.
	const char *psz = inputdata.value.String();
	if ( psz && *psz )
	{
		int iPrio = atoi( psz );
		if ( iPrio >= 1 && iPrio <= 16 )
		{
			m_iszMessagesPriority[iPrio-1] = NULL_STRING;
			if ( iPrio == 1 )
				m_iszMessage = NULL_STRING;
			return;
		}
	}
	// No valid index: clear all priorities
	for ( int i = 0; i < 16; ++i )
		m_iszMessagesPriority[i] = NULL_STRING;
	m_iszMessage = NULL_STRING;
}


void CMessage::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	inputdata_t inputdata;

	inputdata.pActivator	= NULL;
	inputdata.pCaller		= NULL;

	InputShowMessage( inputdata );
}


class CCredits : public CPointEntity
{
public:
	DECLARE_CLASS( CMessage, CPointEntity );
	DECLARE_DATADESC();

	void	Spawn( void );
	void	InputRollCredits( inputdata_t &inputdata );
	void	InputRollOutroCredits( inputdata_t &inputdata );
	void	InputShowLogo( inputdata_t &inputdata );
	void	InputSetLogoLength( inputdata_t &inputdata );

	COutputEvent m_OnCreditsDone;

	virtual void OnRestore();
private:

	void		RollOutroCredits();

	bool		m_bRolledOutroCredits;
	float		m_flLogoLength;
};

LINK_ENTITY_TO_CLASS( env_credits, CCredits );

BEGIN_DATADESC( CCredits )
	DEFINE_INPUTFUNC( FIELD_VOID, "RollCredits", InputRollCredits ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RollOutroCredits", InputRollOutroCredits ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ShowLogo", InputShowLogo ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetLogoLength", InputSetLogoLength ),
	DEFINE_OUTPUT( m_OnCreditsDone, "OnCreditsDone"),

	DEFINE_FIELD( m_bRolledOutroCredits, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flLogoLength, FIELD_FLOAT )
END_DATADESC()

void CCredits::Spawn( void )
{
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );
}

static void CreditsDone_f( void )
{
	CCredits *pCredits = (CCredits*)gEntList.FindEntityByClassname( NULL, "env_credits" );

	if ( pCredits )
	{
		pCredits->m_OnCreditsDone.FireOutput( pCredits, pCredits );
	}
}

static ConCommand creditsdone("creditsdone", CreditsDone_f );

extern ConVar sv_unlockedchapters;

void CCredits::OnRestore()
{
	BaseClass::OnRestore();

	if ( m_bRolledOutroCredits )
	{
		// Roll them again so that the client .dll will send the "creditsdone" message and we'll
		//  actually get back to the main menu
		RollOutroCredits();
	}
}

void CCredits::RollOutroCredits()
{
	sv_unlockedchapters.SetValue( "15" );
	
	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

	CSingleUserRecipientFilter user( pPlayer );
	user.MakeReliable();

	UserMessageBegin( user, "CreditsMsg" );
		WRITE_BYTE( 3 );
	MessageEnd();
}

void CCredits::InputRollOutroCredits( inputdata_t &inputdata )
{
	RollOutroCredits();

	// In case we save restore
	m_bRolledOutroCredits = true;

	gamestats->Event_Credits();
}

void CCredits::InputShowLogo( inputdata_t &inputdata )
{
	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

	CSingleUserRecipientFilter user( pPlayer );
	user.MakeReliable();

	if ( m_flLogoLength )
	{
		UserMessageBegin( user, "LogoTimeMsg" );
			WRITE_FLOAT( m_flLogoLength );
		MessageEnd();
	}
	else
	{
		UserMessageBegin( user, "CreditsMsg" );
			WRITE_BYTE( 1 );
		MessageEnd();
	}
}

void CCredits::InputSetLogoLength( inputdata_t &inputdata )
{
	m_flLogoLength = inputdata.value.Float();
}

void CCredits::InputRollCredits( inputdata_t &inputdata )
{
	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();

	CSingleUserRecipientFilter user( pPlayer );
	user.MakeReliable();

	UserMessageBegin( user, "CreditsMsg" );
		WRITE_BYTE( 2 );
	MessageEnd();
}
