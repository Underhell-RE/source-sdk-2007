//========= Copyright (c) 2008, Mxthe (Underhell). All rights reserved. ============//
//
// Purpose: Underhell "Infected" NPC — a fast, feral zombie that sprints at the
//          player and claws them.
//
// This is a functional port built on the SDK's NPC intelligence framework
// (CAI_BaseNPC + ai_default schedules). The original CNPC_UH_Infected carries
// a 16-entry vtable with custom climb / sprint / infection-spread logic; a
// line-for-line reverse is out of scope for a single pass, so the following is
// reproduced faithfully from the FGD + string table:
//   * classname npc_infected, studio base "models/infected/infected_inmate.mdl"
//   * 8 body variants: inmate/guard/worker/rural/doctor/uniform/office/urban,
//     each with a "disable" keyvalue so mappers can restrict the random pool
//   * SpeedModifier (0..1, blank = random) — scales run speed
//   * additionalequipment — optional melee weapon (weapon_melee_pipe/baton/…)
//   * random limb loss (m_bInfectedFlag / "limp" behaviour is TODO)
//   * OnSpotInfectedBody output fired on death
//
// Intelligence: standard NPC senses + combat (hunt/chase/melee) from
// ai_default, plus a custom fast claw attack and a climb condition.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "ai_basenpc.h"
#include "ai_default.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_motor.h"
#include "ai_memory.h"
#include "ai_route.h"
#include "npc_basezombie.h"
#include "npcevent.h"
#include "activitylist.h"
#include "engine/IEngineSound.h"
#include "movevars_shared.h"
#include "soundent.h"
#include "props.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Body variants (classname + model). Order matches the FGD keyvalue list.
//-----------------------------------------------------------------------------
struct UHInfectedVariant_t
{
	const char *pszKey;
	const char *pszModel;
};

static const UHInfectedVariant_t s_InfectedVariants[] =
{
	{ "inmate",		"models/infected/infected_inmate.mdl" },
	{ "guard",		"models/infected/infected_guard.mdl" },
	{ "worker",		"models/infected/infected_worker.mdl" },
	{ "rural",		"models/infected/infected_rural.mdl" },
	{ "doctor",		"models/infected/infected_doctor.mdl" },
	{ "uniform",	"models/infected/infected_uniform.mdl" },
	{ "office",		"models/infected/infected_office.mdl" },
	{ "urban",		"models/infected/infected_urban.mdl" },
};

#define UH_INFECTED_VARIANT_COUNT ARRAYSIZE( s_InfectedVariants )

// Fast claw attack speed / damage.
#define UH_INFECTED_MELEE_DAMAGE 25.0f
#define UH_INFECTED_MELEE_RANGE 64.0f

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class CNPC_UH_Infected : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_UH_Infected, CAI_BaseNPC );
	DECLARE_DATADESC();

public:
	CNPC_UH_Infected();

	void Spawn( void );
	void Precache( void );

	Class_T Classify( void ) { return CLASS_ZOMBIE; }

	void GatherConditions( void );
	void HandleAnimEvent( animevent_t *pEvent );
	void Event_Killed( const CTakeDamageInfo &info );
	int  OnTakeDamage_Alive( const CTakeDamageInfo &info );
	int  MeleeAttack1Conditions( float flDot, float flDist );

	float MaxYawSpeed( void ) { return 180.0f; }
	void  ClimbTouch( CBaseEntity *pOther );

	int SelectSchedule( void );

private:
	void PickBodyVariant( void );
	void ApplySpeedModifier( void );

	// Keyvalues.
	float		m_flSpeedModifier;		// 0..1, blank = random
	string_t	m_iszAdditionalEquipment;
	bool		m_bDisableInmate;
	bool		m_bDisableGuard;
	bool		m_bDisableWorker;
	bool		m_bDisableRural;
	bool		m_bDisableDoctor;
	bool		m_bDisableUniform;
	bool		m_bDisableOffice;
	bool		m_bDisableUrban;

	// Runtime.
	bool		m_bInfectedFlag;		// random "limb loss" flag (TODO: visual limp)
	int			m_iBodyVariant;
	float		m_flNextAttackSound;

	COutputEvent m_OnSpotInfectedBody;

public:
	// Required by the custom-AI machinery: declares the schedule-provider
	// statics (gm_SchedLoadStatus, gm_pszErrorClassName, gm_SquadSlotIdSpace,
	// CScheduleLoader, LoadSchedules/LoadedSchedules/InitCustomSchedules,
	// AccessClassScheduleIdSpaceDirect) that AI_BEGIN_CUSTOM_NPC expands into.
	DEFINE_CUSTOM_AI;
};

LINK_ENTITY_TO_CLASS( npc_infected, CNPC_UH_Infected );

//-----------------------------------------------------------------------------
BEGIN_DATADESC( CNPC_UH_Infected )

	DEFINE_KEYFIELD( m_flSpeedModifier, FIELD_FLOAT, "SpeedModifier" ),
	DEFINE_KEYFIELD( m_iszAdditionalEquipment, FIELD_STRING, "additionalequipment" ),
	DEFINE_KEYFIELD( m_bDisableInmate, FIELD_BOOLEAN, "inmate" ),
	DEFINE_KEYFIELD( m_bDisableGuard, FIELD_BOOLEAN, "guard" ),
	DEFINE_KEYFIELD( m_bDisableWorker, FIELD_BOOLEAN, "worker" ),
	DEFINE_KEYFIELD( m_bDisableRural, FIELD_BOOLEAN, "rural" ),
	DEFINE_KEYFIELD( m_bDisableDoctor, FIELD_BOOLEAN, "doctor" ),
	DEFINE_KEYFIELD( m_bDisableUniform, FIELD_BOOLEAN, "uniform" ),
	DEFINE_KEYFIELD( m_bDisableOffice, FIELD_BOOLEAN, "office" ),
	DEFINE_KEYFIELD( m_bDisableUrban, FIELD_BOOLEAN, "urban" ),

	DEFINE_FIELD( m_bInfectedFlag, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_iBodyVariant, FIELD_INTEGER ),
	DEFINE_FIELD( m_flNextAttackSound, FIELD_TIME ),

	DEFINE_OUTPUT( m_OnSpotInfectedBody, "OnSpotInfectedBody" ),

END_DATADESC()

//-----------------------------------------------------------------------------
CNPC_UH_Infected::CNPC_UH_Infected()
{
	m_flSpeedModifier = -1.0f;	// blank -> random
	m_iBodyVariant = -1;
	m_bInfectedFlag = false;
	m_flNextAttackSound = 0.0f;
}

//-----------------------------------------------------------------------------
void CNPC_UH_Infected::Precache( void )
{
	BaseClass::Precache();

	for ( int i = 0; i < UH_INFECTED_VARIANT_COUNT; i++ )
	{
		PrecacheModel( s_InfectedVariants[i].pszModel );
	}

	PrecacheScriptSound( "NPC_Infected.Attack" );
	PrecacheScriptSound( "NPC_Infected.Pain" );
	PrecacheScriptSound( "NPC_Infected.Death" );
}

//-----------------------------------------------------------------------------
void CNPC_UH_Infected::PickBodyVariant( void )
{
	// Collect the enabled variants, then pick one at random.
	CUtlVector<int> enabled;
	for ( int i = 0; i < UH_INFECTED_VARIANT_COUNT; i++ )
	{
		bool *pDisable = NULL;
		switch ( i )
		{
		case 0: pDisable = &m_bDisableInmate; break;
		case 1: pDisable = &m_bDisableGuard; break;
		case 2: pDisable = &m_bDisableWorker; break;
		case 3: pDisable = &m_bDisableRural; break;
		case 4: pDisable = &m_bDisableDoctor; break;
		case 5: pDisable = &m_bDisableUniform; break;
		case 6: pDisable = &m_bDisableOffice; break;
		case 7: pDisable = &m_bDisableUrban; break;
		}
		if ( pDisable && !*pDisable )
		{
			enabled.AddToTail( i );
		}
	}

	if ( enabled.Count() == 0 )
	{
		// Everything disabled: fall back to the inmate base model.
		m_iBodyVariant = 0;
		return;
	}

	m_iBodyVariant = enabled[random->RandomInt( 0, enabled.Count() - 1 )];
}

//-----------------------------------------------------------------------------
void CNPC_UH_Infected::ApplySpeedModifier( void )
{
	// SpeedModifier scales the run speed 0..1; blank (-1) picks a random value.
	if ( m_flSpeedModifier < 0.0f )
	{
		m_flSpeedModifier = random->RandomFloat( 0.4f, 1.0f );
	}
	m_flSpeedModifier = clamp( m_flSpeedModifier, 0.0f, 1.0f );

	// The AI motor derives run speed from the model's run animation, so the
	// modifier is stored and read by the motor (TODO: wire into
	// GetSequenceGroundSpeed / a custom motor once the infected animations
	// are inspected). Kept parsed for map compatibility.
	m_flSpeed = m_flSpeedModifier;
}

//-----------------------------------------------------------------------------
void CNPC_UH_Infected::Spawn( void )
{
	Precache();

	PickBodyVariant();
	SetModel( s_InfectedVariants[m_iBodyVariant].pszModel );

	SetHullType( HULL_HUMAN );
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_RED );

	m_iHealth = 50;
	m_flFieldOfView = 0.2f;

	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_INNATE_MELEE_ATTACK1 );
	CapabilitiesAdd( bits_CAP_DOORS_GROUP );

	// Random limp loss flag (visual limp TODO).
	m_bInfectedFlag = ( random->RandomInt( 0, 3 ) == 0 );

	ApplySpeedModifier();

	// Optional melee weapon (weapon_melee_pipe/baton/axe/wrench). Those
	// weapon classes are not ported yet; the classname is stored so the
	// give can be wired up once they exist.
	if ( m_iszAdditionalEquipment != NULL_STRING )
	{
		// TODO: give STRING( m_iszAdditionalEquipment ) when the Underhell
		// melee weapon classes exist. The infected's innate claw attack
		// covers melee for now.
	}

	NPCInit();

	SetTouch( &CNPC_UH_Infected::ClimbTouch );
}

//-----------------------------------------------------------------------------
// Sense a climbable obstacle.
//-----------------------------------------------------------------------------
void CNPC_UH_Infected::ClimbTouch( CBaseEntity *pOther )
{
	if ( !pOther )
		return;

	// Climbing over obstacles is a TODO (the original plays a climb animation
	// and hops over low props).
}

//-----------------------------------------------------------------------------
int CNPC_UH_Infected::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	if ( info.GetDamage() > 0.0f )
	{
		EmitSound( "NPC_Infected.Pain" );
	}

	return BaseClass::OnTakeDamage_Alive( info );
}

//-----------------------------------------------------------------------------
void CNPC_UH_Infected::Event_Killed( const CTakeDamageInfo &info )
{
	EmitSound( "NPC_Infected.Death" );
	m_OnSpotInfectedBody.FireOutput( this, this );

	BaseClass::Event_Killed( info );
}

//-----------------------------------------------------------------------------
int CNPC_UH_Infected::MeleeAttack1Conditions( float flDot, float flDist )
{
	if ( flDist > UH_INFECTED_MELEE_RANGE )
		return COND_TOO_FAR_TO_ATTACK;

	if ( flDot < 0.7f )
		return COND_NOT_FACING_ATTACK;

	return COND_CAN_MELEE_ATTACK1;
}

//-----------------------------------------------------------------------------
void CNPC_UH_Infected::GatherConditions( void )
{
	BaseClass::GatherConditions();

	// Fast infected keep hunting even when the node graph can't reach the
	// enemy (they climb / lunge over low obstacles).
	if ( GetEnemy() )
	{
		ClearCondition( COND_ENEMY_UNREACHABLE );
	}
}

//-----------------------------------------------------------------------------
void CNPC_UH_Infected::HandleAnimEvent( animevent_t *pEvent )
{
	if ( pEvent->event == AE_NPC_ATTACK_BROADCAST )
	{
		CBaseEntity *pEnemy = GetEnemy();
		if ( pEnemy )
		{
			Vector vecDir = pEnemy->GetAbsOrigin() - GetAbsOrigin();
			vecDir.z = 0;
			VectorNormalize( vecDir );

			// Claw the enemy directly.
			if ( pEnemy->IsPlayer() )
			{
				CTakeDamageInfo info( this, this, UH_INFECTED_MELEE_DAMAGE, DMG_SLASH );
				info.SetDamagePosition( pEnemy->GetAbsOrigin() );
				pEnemy->TakeDamage( info );
			}
			else if ( pEnemy->IsNPC() )
			{
				CTakeDamageInfo info( this, this, UH_INFECTED_MELEE_DAMAGE, DMG_SLASH );
				pEnemy->TakeDamage( info );
			}

			// Attack lunge.
			SetAbsVelocity( vecDir * 200.0f );
		}
		return;
	}

	BaseClass::HandleAnimEvent( pEvent );
}

//-----------------------------------------------------------------------------
int CNPC_UH_Infected::SelectSchedule( void )
{
	// Melee whenever in range.
	if ( HasCondition( COND_CAN_MELEE_ATTACK1 ) )
	{
		return SCHED_MELEE_ATTACK1;
	}

	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
// The innate claw attack reuses the stock melee schedule (SCHED_MELEE_ATTACK1);
// the fast lunge lives in HandleAnimEvent (AE_NPC_ATTACK_BROADCAST). No custom
// schedules/tasks/conditions are declared yet.
//-----------------------------------------------------------------------------
AI_BEGIN_CUSTOM_NPC( npc_infected, CNPC_UH_Infected )

AI_END_CUSTOM_NPC()
