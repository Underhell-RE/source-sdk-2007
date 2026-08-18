#include "cbase.h"
#include "npc_basezombie.h"
#include "ai_schedule.h"
#include "ai_motor.h"
#include "soundent.h"
#include "explode.h"
#include "basepropdoor.h"

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
	CNPC_UH_Butcher() : m_bChargeEnabled( true ), m_bCower( false ), m_bCharging( false ), m_flNextCharge( 0.0f ), m_flChargeEndTime( 0.0f ) {}
	void Spawn(); void Precache(); void PrescheduleThink();
	const char *GetHeadcrabClassname() { return "npc_headcrab_black"; }
	const char *GetHeadcrabModel() { return "models/headcrabblack.mdl"; }
	const char *GetLegsModel() { return "models/gibs/fast_zombie_legs.mdl"; }
	const char *GetTorsoModel() { return "models/gibs/fast_zombie_torso.mdl"; }
	const char *GetMoanSound( int ) { return "NPC_Butcher.Idle"; }
	void PainSound( const CTakeDamageInfo & ) { EmitSound( "NPC_Butcher.Alert" ); }
	void AlertSound() { EmitSound( "NPC_Butcher.Alert" ); }
	void IdleSound() { EmitSound( "NPC_Butcher.Idle" ); }
	void DeathSound( const CTakeDamageInfo & ) { EmitSound( "NPC_Butcher.Die" ); }
	void AttackSound() { EmitSound( "NPC_Butcher.Melee" ); }
	void AttackHitSound() { EmitSound( "NPC_Butcher.Melee" ); }
	void AttackMissSound() { EmitSound( "Zombie.AttackMiss" ); }
	void FootstepSound( bool bRightFoot ) { EmitSound( bRightFoot ? "NPC_Butcher.FootstepRight" : "NPC_Butcher.FootstepLeft" ); }
	void FootscuffSound( bool bRightFoot ) { FootstepSound( bRightFoot ); }
	void InputEnableCharge( inputdata_t & ) { m_bChargeEnabled = true; }
	void InputDisableCharge( inputdata_t & ) { m_bChargeEnabled = false; m_bCharging = false; SetAbsVelocity( vec3_origin ); }
	void InputSetCowerOn( inputdata_t & ) { m_bCower = true; }
	void InputSetCowerOff( inputdata_t & ) { m_bCower = false; }
	void InputChargeEntity( inputdata_t &inputdata );
private:
	void DoCharge( CBaseEntity *pTarget );
	void ChargeImpact( CBaseEntity *pHit, const Vector &dir );
	bool m_bChargeEnabled, m_bCower, m_bCharging;
	float m_flNextCharge, m_flChargeEndTime;
	EHANDLE m_hChargeTarget, m_hChargeStart;
};
LINK_ENTITY_TO_CLASS( npc_butcher, CNPC_UH_Butcher );
BEGIN_DATADESC( CNPC_UH_Butcher )
	DEFINE_FIELD( m_bChargeEnabled, FIELD_BOOLEAN ), DEFINE_FIELD( m_bCower, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bCharging, FIELD_BOOLEAN ), DEFINE_FIELD( m_flNextCharge, FIELD_TIME ),
	DEFINE_FIELD( m_flChargeEndTime, FIELD_TIME ), DEFINE_FIELD( m_hChargeTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hChargeStart, FIELD_EHANDLE ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableCharge", InputEnableCharge ), DEFINE_INPUTFUNC( FIELD_VOID, "DisableCharge", InputDisableCharge ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetCowerOn", InputSetCowerOn ), DEFINE_INPUTFUNC( FIELD_VOID, "SetCowerOff", InputSetCowerOff ),
	DEFINE_INPUTFUNC( FIELD_STRING, "ChargeEntity", InputChargeEntity ),
END_DATADESC()
void CNPC_UH_Butcher::Precache() { PrecacheModel( "models/butcher.mdl" ); PrecacheModel( "models/headcrabblack.mdl" ); PrecacheModel( "models/gibs/fast_zombie_legs.mdl" ); PrecacheModel( "models/gibs/fast_zombie_torso.mdl" ); PrecacheScriptSound( "NPC_Butcher.FootstepRight" ); PrecacheScriptSound( "NPC_Butcher.FootstepLeft" ); PrecacheScriptSound( "NPC_Butcher.Alert" ); PrecacheScriptSound( "NPC_Butcher.Idle" ); PrecacheScriptSound( "NPC_Butcher.Die" ); PrecacheScriptSound( "NPC_Butcher.Charge" ); PrecacheScriptSound( "NPC_Butcher.ChargeHit" ); PrecacheScriptSound( "NPC_Butcher.Melee" ); PrecacheScriptSound( "Zombie.AttackMiss" ); BaseClass::Precache(); }
void CNPC_UH_Butcher::Spawn() { Precache(); SetModel( "models/butcher.mdl" ); BaseClass::Spawn(); SetHealth( uh_butcher_health.GetInt() ); SetMaxHealth( GetHealth() ); SetPlaybackRate( uh_butcher_speed.GetFloat() ); }
void CNPC_UH_Butcher::DoCharge( CBaseEntity *pTarget )
{
	if ( !pTarget || !m_bChargeEnabled || m_bCower || m_bCharging || gpGlobals->curtime < m_flNextCharge )
		return;

	Vector dir = pTarget->WorldSpaceCenter() - WorldSpaceCenter();
	dir.z = 0.0f;
	if ( VectorNormalize( dir ) == 0.0f )
		return;

	m_bCharging = true;
	m_flChargeEndTime = gpGlobals->curtime + 2.0f;
	m_flNextCharge = gpGlobals->curtime + uh_butcher_charge_cooldown.GetFloat();
	GetMotor()->SetIdealYawAndUpdate( UTIL_VecToYaw( dir ), AI_KEEP_YAW_SPEED );
	SetAbsVelocity( dir * 512.0f );
	EmitSound( "NPC_Butcher.Charge" );
}

void CNPC_UH_Butcher::ChargeImpact( CBaseEntity *pHit, const Vector &dir )
{
	if ( !pHit || pHit == this )
		return;

	EmitSound( "NPC_Butcher.ChargeHit" );
	CTakeDamageInfo info( this, this, sk_butcher_dmg_charge.GetFloat(), DMG_CLUB );
	CalculateMeleeDamageForce( &info, dir, pHit->WorldSpaceCenter(), 5.0f );
	pHit->TakeDamage( info );

	CBasePropDoor *pDoor = dynamic_cast<CBasePropDoor *>( pHit );
	if ( pDoor && !pDoor->IsDoorLocked() )
		pDoor->UHBreachDoor( this, this, true, pHit->WorldSpaceCenter() );
}

void CNPC_UH_Butcher::PrescheduleThink()
{
	BaseClass::PrescheduleThink();
	if ( m_bCower )
	{
		m_bCharging = false;
		SetAbsVelocity( vec3_origin );
		return;
	}

	CBaseEntity *pTarget = m_hChargeTarget.Get() ? m_hChargeTarget.Get() : GetEnemy();
	if ( m_bCharging )
	{
		Vector dir = GetAbsVelocity();
		dir.z = 0.0f;
		VectorNormalize( dir );
		trace_t tr;
		UTIL_TraceHull( WorldSpaceCenter(), WorldSpaceCenter() + dir * 72.0f,
			Vector( -24, -24, -36 ), Vector( 24, 24, 48 ), MASK_SHOT_HULL,
			this, COLLISION_GROUP_NONE, &tr );
		if ( tr.m_pEnt && tr.m_pEnt != this )
		{
			ChargeImpact( tr.m_pEnt, dir );
			m_bCharging = false;
			SetAbsVelocity( vec3_origin );
		}
		else if ( gpGlobals->curtime >= m_flChargeEndTime )
		{
			m_bCharging = false;
			SetAbsVelocity( vec3_origin );
		}
		return;
	}

	if ( m_bChargeEnabled && pTarget &&
		 ( pTarget->WorldSpaceCenter() - WorldSpaceCenter() ).LengthSqr() <= 4000000.0f )
		DoCharge( pTarget );
}

void CNPC_UH_Butcher::InputChargeEntity( inputdata_t &inputdata )
{
	char names[256];
	Q_strncpy( names, inputdata.value.String(), sizeof( names ) );
	char *pTargetName = strtok( names, " " );
	char *pStartName = strtok( NULL, " " );
	m_hChargeTarget = pTargetName ? gEntList.FindEntityByName( NULL, pTargetName, this ) : NULL;
	m_hChargeStart = pStartName ? gEntList.FindEntityByName( NULL, pStartName, this ) : NULL;
	if ( m_hChargeStart.Get() )
		SetAbsOrigin( m_hChargeStart->GetAbsOrigin() );
	DoCharge( m_hChargeTarget.Get() );
}
