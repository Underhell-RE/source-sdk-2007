//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ENVMESSAGE_H
#define ENVMESSAGE_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"
#include "entityoutput.h"


#define SF_MESSAGE_ONCE			0x0001		// Fade in, not out
#define SF_MESSAGE_ALL			0x0002		// Send to all clients

class CMessage : public CPointEntity
{
public:
	DECLARE_CLASS( CMessage, CPointEntity );

	CMessage();

	void	Spawn( void );
	void	Precache( void );

	inline void SetMessage( string_t iszMessage ) { m_iszMessage = iszMessage; }

	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

private:

	void InputShowMessage( inputdata_t &inputdata );
	void InputMessage( inputdata_t &inputdata );
	void InputSetMessage( inputdata_t &inputdata );
	void InputSetMessagePriority1( inputdata_t &inputdata );
	void InputSetMessagePriority2( inputdata_t &inputdata );
	void InputSetMessagePriority3( inputdata_t &inputdata );
	void InputSetMessagePriority4( inputdata_t &inputdata );
	void InputSetMessagePriority5( inputdata_t &inputdata );
	void InputSetMessagePriority6( inputdata_t &inputdata );
	void InputSetMessagePriority7( inputdata_t &inputdata );
	void InputSetMessagePriority8( inputdata_t &inputdata );
	void InputSetMessagePriority9( inputdata_t &inputdata );
	void InputSetMessagePriority10( inputdata_t &inputdata );
	void InputSetMessagePriority11( inputdata_t &inputdata );
	void InputSetMessagePriority12( inputdata_t &inputdata );
	void InputSetMessagePriority13( inputdata_t &inputdata );
	void InputSetMessagePriority14( inputdata_t &inputdata );
	void InputSetMessagePriority15( inputdata_t &inputdata );
	void InputSetMessagePriority16( inputdata_t &inputdata );
	void InputRemoveMessagePriority( inputdata_t &inputdata );

	void SetMessageFromString( const char *pszMessage );
	const char *ParseTitlesReference( const char *pszInput, char *outBuf, int outBufSize );
	bool GetTitlesEntry( const char *pszEntryName, hudtextparms_t &outParms, char *outMessage, int outMessageSize );

	// Resolve a message string (handles @titles_*.txt_* references) and show it
	// on the target player's HUD. Shared by ShowMessage / InputMessage.
	void ShowMessageText( const char *psz );
	// Return the currently-active message (highest stored priority, else the
	// "message" keyvalue), or NULL when nothing is stored.
	const char *GetActiveMessage( void );
	// Store a message into one of the 16 priority slots (prio 1..16) and, when
	// it is now the highest-priority slot, promote it to the active message.
	void SetPriorityMessage( int prio, const char *psz );
	// Persist / restore the active priority through the global state system
	// when m_iGlobalEnvMessageIndex is set (0-7).
	void UH_SyncGlobalMessagePriority( void );
	void UH_RestoreGlobalMessagePriority( void );

	string_t m_iszMessage;		// Message to display (keyvalue "message").
	string_t m_iszSetMessage;	// Underhell: message set via "SetMessage" (highest priority).
	float m_MessageVolume;
	int m_MessageAttenuation;
	float m_Radius;

	DECLARE_DATADESC();

	string_t m_sNoise;
	COutputEvent m_OnShowMessage;

	// Underhell: 16 priority slots (index 0 == priority 1 .. index 15 == priority 16).
	// Higher priority number wins. "SetMessage" (m_iszSetMessage) beats all of them.
	string_t m_iszMessagesPriority[16];

	// Underhell: which priority slot (1..16, 17 == SetMessage, 0 == none) is active.
	int m_iCurrentPriority;

	// Underhell: GlobalEnvMessageIndex (0-7), -1 == disabled. Shares the active
	// message priority between maps via the global state system.
	int m_iGlobalEnvMessageIndex;
};

#endif // ENVMESSAGE_H
