//========= Copyright (c) 2008, Mxthe (Underhell). All rights reserved. ============//
//
// Purpose: Underhell Ace -- cloaked elite combine assault NPC.
//
//=============================================================================//

#include "cbase.h"
#include "npc_combines.h"
#include "hl2_player.h"

#include "tier0/memdbgon.h"

ConVar sk_ace_health( "sk_ace_health", "200", FCVAR_ARCHIVE );
ConVar sk_ace_kick( "sk_ace_kick", "20", FCVAR_ARCHIVE );
ConVar sk_ace_land_magnitude( "sk_ace_land_magnitude", "128", FCVAR_ARCHIVE );
ConVar sk_ace_land_radius( "sk_ace_land_radius", "20", FCVAR_ARCHIVE );
ConVar sk_ace_land_force( "sk_ace_land_force", "512", FCVAR_ARCHIVE );
ConVar sk_ace_speedmult( "sk_ace_speedmult", "1.5", FCVAR_ARCHIVE );
ConVar sk_ace_cloak_timer( "sk_ace_cloak_timer", "3.0", FCVAR_ARCHIVE );
ConVar ace_spawn_health( "ace_spawn_health", "0", FCVAR_ARCHIVE );

class CNPC_Ace : public CNPC_CombineS
{
	DECLARE_CLASS( CNPC_Ace, CNPC_CombineS );
	DECLARE_DATADESC();
public:
	CNPC_Ace();
	void Spawn( void );
	void Precache( void );
	void PrescheduleThink( void );
	void PainSound( const CTakeDamageInfo &info );
	void Event_Killed( const CTakeDamageInfo &info );
	void InputCloakNow( inputdata_t &inputdata );
	void InputUnCloakNow( inputdata_t &inputdata );
	void InputDisableCloak( inputdata_t &inputdata );
	void InputEnableCloak( inputdata_t &inputdata );
private:
	void SetCloaked( bool bCloaked );
	bool m_bCloakEnabled;
	bool m_bCloaked;
	float m_flNextCloakTime;
};

LINK_ENTITY_TO_CLASS( npc_ace, CNPC_Ace );

BEGIN_DATADESC( CNPC_Ace )
	DEFINE_FIELD( m_bCloakEnabled, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bCloaked, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flNextCloakTime, FIELD_TIME ),
	DEFINE_INPUTFUNC( FIELD_VOID, "CloakNow", InputCloakNow ),
	DEFINE_INPUTFUNC( FIELD_VOID, "UnCloakNow", InputUnCloakNow ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableCloak", InputDisableCloak ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableCloak", InputEnableCloak ),
END_DATADESC()

CNPC_Ace::CNPC_Ace()
{
	m_bCloakEnabled = true;
	m_bCloaked = false;
	m_flNextCloakTime = 0.0f;
}

void CNPC_Ace::Precache()
{
	if ( GetModelName() == NULL_STRING )
		SetModelName( MAKE_STRING( "models/combine_soldier_assassin.mdl" ) );
	PrecacheModel( STRING( GetModelName() ) );
	BaseClass::Precache();
}

void CNPC_Ace::Spawn()
{
	Precache();
	BaseClass::Spawn();
	SetHealth( ace_spawn_health.GetInt() > 0 ? ace_spawn_health.GetInt() : sk_ace_health.GetInt() );
	SetMaxHealth( GetHealth() );
	SetKickDamage( sk_ace_kick.GetFloat() );
	SetPlaybackRate( sk_ace_speedmult.GetFloat() );
}

void CNPC_Ace::SetCloaked( bool bCloaked )
{
	if ( m_bCloaked == bCloaked )
		return;
	m_bCloaked = bCloaked;
	SetRenderMode( kRenderTransColor );
	SetRenderColor( 255, 255, 255, bCloaked ? 24 : 255 );
	if ( bCloaked )
		AddEffects( EF_NOSHADOW );
	else
		RemoveEffects( EF_NOSHADOW );
}

void CNPC_Ace::PrescheduleThink()
{
	BaseClass::PrescheduleThink();
	if ( !m_bCloakEnabled || gpGlobals->curtime < m_flNextCloakTime )
		return;

	if ( GetEnemy() && !HasCondition( COND_SEE_ENEMY ) )
	{
		SetCloaked( true );
		m_flNextCloakTime = gpGlobals->curtime + sk_ace_cloak_timer.GetFloat();
	}
	else if ( m_bCloaked && HasCondition( COND_SEE_ENEMY ) )
	{
		SetCloaked( false );
		m_flNextCloakTime = gpGlobals->curtime + sk_ace_cloak_timer.GetFloat();
	}
}

void CNPC_Ace::PainSound( const CTakeDamageInfo &info )
{
	if ( gpGlobals->curtime >= m_flNextCloakTime )
	{
		GetSentences()->Speak( "ACE_PAIN", SENTENCE_PRIORITY_INVALID, SENTENCE_CRITERIA_ALWAYS );
		m_flNextCloakTime = gpGlobals->curtime + 1.0f;
	}
}

void CNPC_Ace::Event_Killed( const CTakeDamageInfo &info )
{
	SetCloaked( false );
	if ( GetEnemy() && GetEnemy()->IsPlayer() )
		GetSentences()->Speak( "ACE_PLAYER_DEAD", SENTENCE_PRIORITY_INVALID, SENTENCE_CRITERIA_ALWAYS );
	else
		GetSentences()->Speak( "ACE_MAN_DOWN", SENTENCE_PRIORITY_INVALID, SENTENCE_CRITERIA_ALWAYS );
	BaseClass::Event_Killed( info );
}

void CNPC_Ace::InputCloakNow( inputdata_t &inputdata ) { if ( m_bCloakEnabled ) SetCloaked( true ); }
void CNPC_Ace::InputUnCloakNow( inputdata_t &inputdata ) { SetCloaked( false ); }
void CNPC_Ace::InputDisableCloak( inputdata_t &inputdata ) { m_bCloakEnabled = false; SetCloaked( false ); }
void CNPC_Ace::InputEnableCloak( inputdata_t &inputdata ) { m_bCloakEnabled = true; }
