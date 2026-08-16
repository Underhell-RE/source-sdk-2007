#include "cbase.h"
#include "npc_basezombie.h"
#include "ai_schedule.h"
#include "soundent.h"
#include "explode.h"

#include "tier0/memdbgon.h"

ConVar uh_butcher_health( "uh_butcher_health", "99999999", FCVAR_ARCHIVE );
ConVar sk_butcher_dmg_charge( "sk_butcher_dmg_charge", "20", FCVAR_ARCHIVE );
ConVar uh_butcher_speed( "uh_butcher_speed", "1.25", FCVAR_ARCHIVE );
ConVar uh_butcher_charge_cooldown( "uh_butcher_charge_cooldown", "5.0", FCVAR_ARCHIVE );

class CNPC_UH_Butcher : public CNPC_BaseZombie
{
	DECLARE_CLASS( CNPC_UH_Butcher, CNPC_BaseZombie );
	DECLARE_DATADESC();
public:
	CNPC_UH_Butcher() : m_bChargeEnabled( true ), m_bCower( false ), m_flNextCharge( 0.0f ) {}
	void Spawn(); void Precache(); void PrescheduleThink();
	void AlertSound() { EmitSound( "NPC_Butcher.Alert" ); }
	void IdleSound() { EmitSound( "NPC_Butcher.Idle" ); }
	void DeathSound( const CTakeDamageInfo &info ) { EmitSound( "NPC_Butcher.Die" ); }
	void FootstepSound( bool bRightFoot ) { EmitSound( bRightFoot ? "NPC_Butcher.FootstepRight" : "NPC_Butcher.FootstepLeft" ); }
	void InputEnableCharge( inputdata_t & ) { m_bChargeEnabled = true; }
	void InputDisableCharge( inputdata_t & ) { m_bChargeEnabled = false; }
	void InputSetCowerOn( inputdata_t & ) { m_bCower = true; }
	void InputSetCowerOff( inputdata_t & ) { m_bCower = false; }
	void InputChargeEntity( inputdata_t &inputdata );
private:
	void DoCharge( CBaseEntity *pTarget );
	bool m_bChargeEnabled, m_bCower; float m_flNextCharge; EHANDLE m_hChargeTarget;
};
LINK_ENTITY_TO_CLASS( npc_butcher, CNPC_UH_Butcher );
BEGIN_DATADESC( CNPC_UH_Butcher )
	DEFINE_FIELD( m_bChargeEnabled, FIELD_BOOLEAN ), DEFINE_FIELD( m_bCower, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flNextCharge, FIELD_TIME ), DEFINE_FIELD( m_hChargeTarget, FIELD_EHANDLE ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableCharge", InputEnableCharge ), DEFINE_INPUTFUNC( FIELD_VOID, "DisableCharge", InputDisableCharge ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetCowerOn", InputSetCowerOn ), DEFINE_INPUTFUNC( FIELD_VOID, "SetCowerOff", InputSetCowerOff ),
	DEFINE_INPUTFUNC( FIELD_STRING, "ChargeEntity", InputChargeEntity ),
END_DATADESC()
void CNPC_UH_Butcher::Precache() { PrecacheModel( "models/butcher.mdl" ); PrecacheScriptSound( "NPC_Butcher.FootstepRight" ); PrecacheScriptSound( "NPC_Butcher.FootstepLeft" ); PrecacheScriptSound( "NPC_Butcher.Alert" ); PrecacheScriptSound( "NPC_Butcher.Idle" ); PrecacheScriptSound( "NPC_Butcher.Die" ); PrecacheScriptSound( "NPC_Butcher.Charge" ); PrecacheScriptSound( "NPC_Butcher.ChargeHit" ); PrecacheScriptSound( "NPC_Butcher.Melee" ); BaseClass::Precache(); }
void CNPC_UH_Butcher::Spawn() { Precache(); SetModel( "models/butcher.mdl" ); BaseClass::Spawn(); SetHealth( uh_butcher_health.GetInt() ); SetMaxHealth( GetHealth() ); SetPlaybackRate( uh_butcher_speed.GetFloat() ); }
void CNPC_UH_Butcher::DoCharge( CBaseEntity *pTarget ) { if ( !pTarget || gpGlobals->curtime < m_flNextCharge ) return; Vector dir = pTarget->WorldSpaceCenter() - WorldSpaceCenter(); VectorNormalize( dir ); ApplyAbsVelocityImpulse( dir * 512.0f ); EmitSound( "NPC_Butcher.Charge" ); CTakeDamageInfo info( this, this, sk_butcher_dmg_charge.GetFloat(), DMG_CLUB ); CalculateMeleeDamageForce( &info, dir, WorldSpaceCenter() ); pTarget->TakeDamage( info ); EmitSound( "NPC_Butcher.ChargeHit" ); m_flNextCharge = gpGlobals->curtime + uh_butcher_charge_cooldown.GetFloat(); }
void CNPC_UH_Butcher::PrescheduleThink() { BaseClass::PrescheduleThink(); if ( m_bCower ) return; CBaseEntity *pTarget = m_hChargeTarget.Get() ? m_hChargeTarget.Get() : GetEnemy(); if ( m_bChargeEnabled && pTarget && ( pTarget->WorldSpaceCenter() - WorldSpaceCenter() ).LengthSqr() <= 4000000.0f ) DoCharge( pTarget ); }
void CNPC_UH_Butcher::InputChargeEntity( inputdata_t &inputdata ) { m_hChargeTarget = gEntList.FindEntityByName( NULL, inputdata.value.String(), this ); DoCharge( m_hChargeTarget.Get() ); }
