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

// Tries to find a titles entry in scripts/titles_*.txt files.
// Returns true if found and fills outParms and outMessage (which is the
// localization token like "#UnderHell_House_Comment_Chest" or the raw message).
// This replicates original Underhell's handling of @titles_*.txt_* references
// and ensures correct color/position (middle bottom for chest comments vs
// middle for objectives). Uses filesystem and KeyValues parser.
bool CMessage::GetTitlesEntry( const char *pszEntryName, hudtextparms_t &outParms, char *outMessage, int outMessageSize )
{
	if ( !pszEntryName || !*pszEntryName )
		return false;

	// List of known titles files from Underhell (from scripts/ folder)
	const char *pszFiles[] =
	{
		"titles.txt",
		"titles_Prologue.txt",
		"titles_House.txt",
		"titles_Chapter1.txt",
		"titles_Chapter1.txt", // duplicate for safety, engine also loads mod-specific
		NULL
	};

	// Default parms (if not found in file, use objective style)
	Q_memset( &outParms, 0, sizeof(outParms) );
	outParms.channel = 1;
	outParms.x = -1;
	outParms.y = -1;
	outParms.effect = 0;
	outParms.r1 = 255; outParms.g1 = 255; outParms.b1 = 255; outParms.a1 = 255;
	outParms.r2 = 255; outParms.g2 = 255; outParms.b2 = 255; outParms.a2 = 255;
	outParms.fadeinTime = 0;
	outParms.fadeoutTime = 0;
	outParms.holdTime = 5.0f;
	outParms.fxTime = 0;

	for ( int i = 0; pszFiles[i]; ++i )
	{
		char szPath[256];
		Q_snprintf( szPath, sizeof(szPath), "scripts/%s", pszFiles[i] );

		// Try to load via filesystem (MOD then GAME)
		KeyValues *pKV = new KeyValues( pszFiles[i] );
		bool bLoaded = pKV->LoadFromFile( filesystem, szPath, "MOD" );
		if ( !bLoaded )
			bLoaded = pKV->LoadFromFile( filesystem, szPath, "GAME" );

		if ( !bLoaded )
		{
			pKV->deleteThis();
			continue;
		}

		// Titles files have entries as subkeys at root level
		// Each entry like "House_Comment_Chest" { "positionx" "0.1" ... "Message" "#UnderHell_..." }
		KeyValues *pEntry = pKV->FindKey( pszEntryName );
		if ( !pEntry )
		{
			// Some files have an extra top-level wrapper (file name as root)?
			// Try to find case-insensitive?
			for ( KeyValues *pSub = pKV->GetFirstSubKey(); pSub; pSub = pSub->GetNextKey() )
			{
				if ( !Q_stricmp( pSub->GetName(), pszEntryName ) )
				{
					pEntry = pSub;
					break;
				}
			}
		}

		if ( pEntry )
		{
			// Fill parms from entry
			outParms.x = pEntry->GetFloat( "positionx", -1.0f );
			outParms.y = pEntry->GetFloat( "positiony", -1.0f );
			outParms.effect = pEntry->GetInt( "effect", 0 );
			outParms.fadeinTime = pEntry->GetFloat( "fadein", 0.0f );
			outParms.fadeoutTime = pEntry->GetFloat( "fadeout", 0.0f );
			outParms.holdTime = pEntry->GetFloat( "holdtime", 3.0f );
			outParms.fxTime = pEntry->GetFloat( "fxtime", 0.25f );
			outParms.r1 = pEntry->GetInt( "r1", 255 );
			outParms.g1 = pEntry->GetInt( "g1", 255 );
			outParms.b1 = pEntry->GetInt( "b1", 255 );
			outParms.a1 = pEntry->GetInt( "a1", 255 );
			outParms.r2 = pEntry->GetInt( "r2", 255 );
			outParms.g2 = pEntry->GetInt( "g2", 255 );
			outParms.b2 = pEntry->GetInt( "b2", 255 );
			outParms.a2 = pEntry->GetInt( "a2", 255 );
			outParms.channel = pEntry->GetInt( "channel", 1 );

			const char *pszMsg = pEntry->GetString( "Message", "" );
			if ( pszMsg && *pszMsg )
			{
				Q_strncpy( outMessage, pszMsg, outMessageSize );
			}
			else
			{
				// No Message field — use entry name as fallback
				Q_strncpy( outMessage, pszEntryName, outMessageSize );
			}

			pKV->deleteThis();
			return true;
		}

		pKV->deleteThis();
	}

	return false;
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

	// Try to resolve titles file entry for correct color/position (middle bottom etc.)
	// This is the original Underhell behavior: env_message message is a titles entry
	// like "House_Comment_Chest" which lives in scripts/titles_House.txt with
	// positionx/y, color r1/g1/b1, effect, etc., and Message "#UnderHell_House_Comment_Chest"
	// We load that entry and use its parms for HUD display, to avoid duplicate middle text
	// and to get the correct color (chest comments are different from objectives).
	if ( pszToShow && *pszToShow )
	{
		hudtextparms_t hparms;
		char szMessageFromTitles[512];
		if ( GetTitlesEntry( pszToShow, hparms, szMessageFromTitles, sizeof(szMessageFromTitles) ) )
		{
			// Found in titles_*.txt — use its parms and its Message (localization token)
			// This matches original: text rendered with different color in middle bottom
			CBaseEntity *pPlayer = NULL;
			if ( inputdata.pActivator && inputdata.pActivator->IsPlayer() )
				pPlayer = inputdata.pActivator;
			else
				pPlayer = (gpGlobals->maxClients > 1) ? NULL : UTIL_GetLocalPlayer();

			if ( pPlayer && pPlayer->IsPlayer() )
				UTIL_HudMessage( ToBasePlayer(pPlayer), hparms, szMessageFromTitles );
			else
				UTIL_HudMessageAll( hparms, szMessageFromTitles );

			// Also play sound and fire output, but don't do extra UTIL_ShowMessage to avoid duplicate
			goto play_sound;
		}
	}

	// Fallback: no titles entry found — try vanilla path and localization token fallback
	{
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

		// If UTIL_ShowMessage didn't find anything (titles file not loaded), try localization token directly
		// This ensures at least something shows, matching original where only sound played before fix
		if ( pszToShow && *pszToShow && pszToShow[0] != '#' )
		{
			char szFallback[512];
			Q_snprintf( szFallback, sizeof(szFallback), "#UnderHell_%s", pszToShow );

			if ( bHasPlayer && pPlayer )
				ClientPrint( ToBasePlayer(pPlayer), HUD_PRINTCENTER, szFallback );
			else
				UTIL_ClientPrintAll( HUD_PRINTCENTER, szFallback );
		}
	}

play_sound:
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
