//========= Copyright (c) 2008, Mxthe (Underhell). All rights reserved. ============//
//
// Purpose: Underhell "Infected" NPC — full reconstruction from serveror.dll
//          decompiled hexrays (sub_101A6F00, sub_101A7410, sub_101A7B70, sub_101A6620,
//          sub_101A6FF0, sub_101A5A10, etc) + FGD underhell.fgd.
//
// Original: CNPC_UH_Infected inherits from CAI_BaseNPC indirectly via
//           CNPC_BaseZombie? Actually vftable analysis shows its primary
//           vtable is CNPC_BaseZombie (it has climb/jump/door logic). We
//           inherit from CNPC_BaseZombie to reuse door bash, physics swat,
//           headcrab etc, then extend with infected-specific:
//             * 8 variants (inmate/guard/worker/rural/doctor/uniform/office/urban)
//             * disable keyvalues (inmate..urban) + SpeedModifier + additionalequipment
//             * random limb loss (m_bInfectedFlag / limp)
//             * custom conditions/tasks/schedules (sprint slots, climb touch,
//               unstick jump, radio investigate, door bash)
//             * fast claw attack (25 dmg) + lunge + fast run
//             * OnSpotInfectedBody output (via uh_ai.cpp spot logic)
//             * gib models per variant
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
#include "physics_npc_solver.h"
#include "basepropdoor.h"
#include "doors.h"
#include "underhell/uh_items.h"
#include "physics_prop_ragdoll.h"
#include "basecombatcharacter.h"
#include "ammodef.h"
#include "explode.h"		// ExplosionCreate (radiocracker detonation, serveror.dll sub_10173A20)

// skill convar used by the radiocracker detonation (defined in hl2_gamerules.cpp)
extern ConVar sk_plr_dmg_smg1_grenade;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// ---------------------------------------------------------------------------
// ConVars from original binaries (strings extracted from serveror.dll)
// ---------------------------------------------------------------------------
static ConVar uh_infected_health( "uh_infected_health", "50", FCVAR_ARCHIVE, "Infected health" );
static ConVar uh_infected_door_dist( "uh_infected_door_dist", "96", FCVAR_ARCHIVE, "Door bash distance" );
static ConVar uh_infected_efficiency( "uh_infected_efficiency", "0", FCVAR_ARCHIVE, "Efficiency" );
static ConVar uh_infected_randspeedenabled( "uh_infected_randspeedenabled", "1", FCVAR_ARCHIVE, "Random speed enabled" );
static ConVar uh_infectedcower( "uh_infectedcower", "0", FCVAR_ARCHIVE, "Cower?" );

// ---------------------------------------------------------------------------
// Variant table (models + gib folder). Order matches FGD.
// ---------------------------------------------------------------------------
struct UHInfectedVariant_t
{
	const char *pszKey;
	const char *pszModel;
	const char *pszGibLeftArm;
	const char *pszGibRightArm;
	const char *pszGibLeftLeg;
	const char *pszGibRightLeg;
	const char *pszHelmet; // optional helmet model that can drop
};

// IDs are serveror.dll IDs, not FGD display order (sub_101A6620).
static const UHInfectedVariant_t s_InfectedVariants[] =
{
	{ "inmate",  "models/infected/infected_inmate.mdl",  "models/gibs/bodyparts/infected/inmate_leftarm.mdl",  "models/gibs/bodyparts/infected/inmate_rightarm.mdl",  "models/gibs/bodyparts/infected/inmate_leftleg.mdl",  "models/gibs/bodyparts/infected/inmate_rightleg.mdl",  NULL },
	{ "worker",  "models/infected/infected_worker.mdl",  "models/gibs/bodyparts/infected/worker_leftarm.mdl",  "models/gibs/bodyparts/infected/worker_rightarm.mdl",  "models/gibs/bodyparts/infected/worker_leftleg.mdl",  "models/gibs/bodyparts/infected/worker_rightleg.mdl",  "models/items/worker_helmet.mdl" },
	{ "doctor",  "models/infected/infected_doctor.mdl",  "models/gibs/bodyparts/infected/doctor_leftarm.mdl",  "models/gibs/bodyparts/infected/doctor_rightarm.mdl",  "models/gibs/bodyparts/infected/doctor_leftleg.mdl",  "models/gibs/bodyparts/infected/doctor_rightleg.mdl",  NULL },
	{ "uniform", "models/infected/infected_uniform.mdl", "models/gibs/bodyparts/infected/uniform_leftarm.mdl", "models/gibs/bodyparts/infected/uniform_rightarm.mdl", "models/gibs/bodyparts/infected/uniform_leftleg.mdl", "models/gibs/bodyparts/infected/uniform_rightleg.mdl", NULL },
	{ "urban",   "models/infected/infected_urban.mdl",   "models/gibs/bodyparts/infected/urban_leftarm.mdl",   "models/gibs/bodyparts/infected/urban_rightarm.mdl",   "models/gibs/bodyparts/infected/urban_leftleg.mdl",   "models/gibs/bodyparts/infected/urban_rightleg.mdl",   NULL },
	{ "rural",   "models/infected/infected_rural.mdl",   "models/gibs/bodyparts/infected/rural_leftarm.mdl",   "models/gibs/bodyparts/infected/rural_rightarm.mdl",   "models/gibs/bodyparts/infected/rural_leftleg.mdl",   "models/gibs/bodyparts/infected/rural_rightleg.mdl",   NULL },
	{ "guard",   "models/infected/infected_guard.mdl",   "models/gibs/bodyparts/infected/guard_leftarm.mdl",   "models/gibs/bodyparts/infected/guard_rightarm.mdl",   "models/gibs/bodyparts/infected/guard_leftleg.mdl",   "models/gibs/bodyparts/infected/guard_rightleg.mdl",   "models/items/guard_helmet.mdl" },
	{ "office",  "models/infected/infected_office.mdl",  "models/gibs/bodyparts/infected/office_leftarm.mdl",  "models/gibs/bodyparts/infected/office_rightarm.mdl",  "models/gibs/bodyparts/infected/office_leftleg.mdl",  "models/gibs/bodyparts/infected/office_rightleg.mdl",  NULL },
};

#define UH_INFECTED_VARIANT_COUNT ARRAYSIZE(s_InfectedVariants)

// ---------------------------------------------------------------------------
// Custom AI Ids from decompiled sub_101A7410
// ---------------------------------------------------------------------------
enum
{
	TASK_UH_INFECTED_UNSTICK_JUMP = 250,
	TASK_ZOMBIE_YAW_TO_DOOR = 251,
	TASK_ZOMBIE_ATTACK_DOOR = 252,
	// original typo: TASK_ZOMBIE_BREAKTHROUG = 253 (without H)
	TASK_ZOMBIE_BREAKTHROUG = 253,
	TASK_UH_DESTROY_RADIO = 254,
	TASK_UH_RADIO_PICKUP = 255,
	TASK_UH_THROW_ITEM = 256,
};

enum
{
	SCHED_UH_INFECTED_UNSTICK_JUMP = 101,
	SCHED_UH_INFECTED_CLIMBING_UNSTICK_JUMP = 102,
	SCHED_ZOMBIE_BASH_DOOR = 103,
	SCHED_UH_INFECTED_INVESTIGATE_RADIO = 104,
};

enum
{
	COND_UH_INFECTED_GRENADE = 76,
	COND_UH_INFECTED_RANDOMRUN = 77,
	COND_UH_INFECTED_CLIMB_TOUCH = 78,
	COND_DOOR_OPENED = 79,
	COND_BLOCKED_BY_DOOR = 80,
};

enum
{
	SQUAD_SLOT_UH_INFECTED_SPRINT1 = 8,
	SQUAD_SLOT_UH_INFECTED_SPRINT2 = 9,
};

// Activities (from decompiled)
static int ACT_UH_INFECTED_ATTACK_FAST = -1;
static int ACT_RUN_PANICKED = -1;
static int ACT_ZOMBIE_WALLPOUND = -1;
static int ACT_ZOMBIE_BREAKTHROUGH = -1;

// ---------------------------------------------------------------------------
// Fast infected claw constants
// ---------------------------------------------------------------------------
#define UH_INFECTED_MELEE_DAMAGE 25.0f
#define UH_INFECTED_MELEE_RANGE 64.0f
#define UH_INFECTED_SPRINT_SPEED_MIN 180.0f
#define UH_INFECTED_SPRINT_SPEED_MAX 260.0f

// ---------------------------------------------------------------------------
// CNPC_UH_Infected
// ---------------------------------------------------------------------------
class CNPC_UH_Infected : public CNPC_BaseZombie
{
	DECLARE_CLASS( CNPC_UH_Infected, CNPC_BaseZombie );
	DECLARE_DATADESC();

public:
	CNPC_UH_Infected();

	void Spawn( void );
	void Precache( void );
	void SetZombieModel( void );

	Class_T Classify( void ) { return CLASS_ZOMBIE; }

	// BaseZombie overrides
	const char *GetHeadcrabClassname( void ) { return "npc_headcrab_black"; }
	const char *GetHeadcrabModel( void ) { return "models/headcrabblack.mdl"; }
	const char *GetLegsModel( void ) { return "models/gibs/fast_zombie_legs.mdl"; }
	const char *GetTorsoModel( void ) { return "models/gibs/fast_zombie_torso.mdl"; }
	const char *GetMoanSound( int nSound );
	void PainSound( const CTakeDamageInfo &info );
	void DeathSound( const CTakeDamageInfo &info );
	void AlertSound( void );
	void IdleSound( void );
	void AttackSound( void );
	void AttackHitSound( void );
	void AttackMissSound( void );
	void FootstepSound( bool fRightFoot );
	void FootscuffSound( bool fRightFoot );

	int TranslateSchedule( int scheduleType );
	int SelectSchedule( void );
	int SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );

	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );

	void HandleAnimEvent( animevent_t *pEvent );
	void ClimbTouch( CBaseEntity *pOther );
	void LeapAttackTouch( CBaseEntity *pOther ) { ClimbTouch( pOther ); } // reuse push logic

	void GatherConditions( void );
	void PrescheduleThink( void );
	void BuildScheduleTestBits( void );

	int GetSoundInterests( void );

	int MeleeAttack1Conditions( float flDot, float flDist );
	int RangeAttack1Conditions( float flDot, float flDist ) { return COND_NONE; } // no leap in our port, but we have sprint

	void Event_Killed( const CTakeDamageInfo &info );
	int OnTakeDamage_Alive( const CTakeDamageInfo &info );

	Activity NPC_TranslateActivity( Activity baseAct );

	bool IsUnreachable( CBaseEntity *pEntity ) { return false; } // infected ignores unreachable

	// Inputs
	void InputSetSpeedModifier( inputdata_t &inputdata );

private:
	void PickBodyVariant( void );
	void ApplySpeedModifier( void );
	void ApplyBodygroups( void );
	void GiveAdditionalEquipment( void );
	void BecomeLimp( void );

	// Keyvalues
	float       m_flSpeedModifier; // blank = random 0.4-1.0, otherwise 0..1
	string_t    m_iszAdditionalEquipment;
	bool        m_bDisableInmate;
	bool        m_bDisableGuard;
	bool        m_bDisableWorker;
	bool        m_bDisableRural;
	bool        m_bDisableDoctor;
	bool        m_bDisableUniform;
	bool        m_bDisableOffice;
	bool        m_bDisableUrban;

	// Runtime
	int         m_iInfectedVariant; // 0..7
	bool        m_bInfectedFlag;   // random limp flag (25%)
	bool        m_bIsLimping;
	float       m_flNextSprintTime;
	float       m_flNextMoanTime;
	float       m_flNextClimbTime;
	float       m_flSpeedScale;   // derived from SpeedModifier (0.8..1.3)
	int         m_iGibLeftArm;
	int         m_iGibRightArm;
	int         m_iGibLeftLeg;
	int         m_iGibRightLeg;
	COutputEvent m_OnSpotInfectedBody;

	// For door bash (borrowed from CZombie)
	EHANDLE     m_hBlockingDoor;
	float       m_flDoorBashYaw;
	CRandSimTimer m_DurationDoorBash;

public:
	DEFINE_CUSTOM_AI;
};

LINK_ENTITY_TO_CLASS( npc_infected, CNPC_UH_Infected );

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

	DEFINE_FIELD( m_iInfectedVariant, FIELD_INTEGER ),
	DEFINE_FIELD( m_bInfectedFlag, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bIsLimping, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flNextSprintTime, FIELD_TIME ),
	DEFINE_FIELD( m_flNextMoanTime, FIELD_TIME ),
	DEFINE_FIELD( m_flNextClimbTime, FIELD_TIME ),
	DEFINE_FIELD( m_flSpeedScale, FIELD_FLOAT ),
	DEFINE_FIELD( m_hBlockingDoor, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flDoorBashYaw, FIELD_FLOAT ),

	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetSpeedModifier", InputSetSpeedModifier ),

	DEFINE_OUTPUT( m_OnSpotInfectedBody, "OnSpotInfectedBody" ),

	DEFINE_ENTITYFUNC( ClimbTouch ),

END_DATADESC()

// ---------------------------------------------------------------------------
// Custom AI registration – mirrors sub_101A7410 exactly
// ---------------------------------------------------------------------------
AI_BEGIN_CUSTOM_NPC( npc_infected, CNPC_UH_Infected )

	DECLARE_SQUADSLOT( SQUAD_SLOT_UH_INFECTED_SPRINT1 )
	DECLARE_SQUADSLOT( SQUAD_SLOT_UH_INFECTED_SPRINT2 )

	DECLARE_CONDITION( COND_UH_INFECTED_GRENADE )
	DECLARE_CONDITION( COND_UH_INFECTED_RANDOMRUN )
	DECLARE_CONDITION( COND_UH_INFECTED_CLIMB_TOUCH )
	DECLARE_CONDITION( COND_BLOCKED_BY_DOOR )
	DECLARE_CONDITION( COND_DOOR_OPENED )

	DECLARE_TASK( TASK_UH_INFECTED_UNSTICK_JUMP )
	DECLARE_TASK( TASK_ZOMBIE_YAW_TO_DOOR )
	DECLARE_TASK( TASK_ZOMBIE_ATTACK_DOOR )
	DECLARE_TASK( TASK_ZOMBIE_BREAKTHROUG )
	DECLARE_TASK( TASK_UH_DESTROY_RADIO )
	DECLARE_TASK( TASK_UH_RADIO_PICKUP )
	DECLARE_TASK( TASK_UH_THROW_ITEM )

	// Activities – resolved from engine strings, same names as original DLL
	DECLARE_ACTIVITY( ACT_UH_INFECTED_ATTACK_FAST )
	DECLARE_ACTIVITY( ACT_RUN_PANICKED )
	DECLARE_ACTIVITY( ACT_ZOMBIE_WALLPOUND )
	DECLARE_ACTIVITY( ACT_ZOMBIE_BREAKTHROUGH )

	DEFINE_SCHEDULE
	(
		SCHED_UH_INFECTED_UNSTICK_JUMP,
		"	Tasks"
		"		TASK_UH_INFECTED_UNSTICK_JUMP	0"
		"	Interrupts"
	)

	DEFINE_SCHEDULE
	(
		SCHED_UH_INFECTED_CLIMBING_UNSTICK_JUMP,
		"	Tasks"
		"		TASK_SET_ACTIVITY		ACTIVITY:ACT_IDLE"
		"		TASK_UH_INFECTED_UNSTICK_JUMP	0"
		"	Interrupts"
	)

	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_BASH_DOOR,

		"	Tasks"
		"		TASK_SET_ACTIVITY		ACTIVITY:ACT_ZOMBIE_WALLPOUND"
		"		TASK_SET_FAIL_SCHEDULE		SCHEDULE:SCHED_TAKE_COVER_FROM_ENEMY"
		"		TASK_ZOMBIE_YAW_TO_DOOR		0"
		"		TASK_FACE_IDEAL			0"
		"		TASK_ZOMBIE_ATTACK_DOOR		0"
		"		TASK_ZOMBIE_BREAKTHROUG		0"
		"	Interrupts"
		"		COND_ZOMBIE_RELEASECRAB"
		"		COND_ENEMY_DEAD"
		"		COND_NEW_ENEMY"
		"		COND_DOOR_OPENED"
	)

	DEFINE_SCHEDULE
	(
		SCHED_UH_INFECTED_INVESTIGATE_RADIO,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_STORE_LASTPOSITION		0"
		"		TASK_SET_TOLERANCE_DISTANCE	128"
		"		TASK_GET_PATH_TO_TARGET		0"
		"		TASK_FACE_IDEAL			0"
		"		TASK_RUN_PATH			0"
		"		TASK_WAIT_FOR_MOVEMENT		0"
		"		TASK_STOP_MOVING		0"
		"		TASK_WAIT			5"
		"		TASK_FACE_TARGET		0"
		"		TASK_ITEM_PICKUP		0"
		"		TASK_UH_DESTROY_RADIO		0"
		"		TASK_SET_SCHEDULE		SCHEDULE:SCHED_RUN_RANDOM"
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
		"		COND_HEAR_MOVE_AWAY"
		"		COND_NEW_ENEMY"
		"		COND_SEE_ENEMY"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_CAN_RANGE_ATTACK2"
	)

AI_END_CUSTOM_NPC()

// ---------------------------------------------------------------------------
// Construction – mirrors sub_101A7B70
// ---------------------------------------------------------------------------
CNPC_UH_Infected::CNPC_UH_Infected() : m_DurationDoorBash( 2.0f, 6.0f )
{
	m_flSpeedModifier = -1.0f; // blank -> random
	m_iInfectedVariant = -1;
	m_bInfectedFlag = false;
	m_bIsLimping = false;
	m_flNextSprintTime = 0.0f;
	m_flNextMoanTime = 0.0f;
	m_flNextClimbTime = 0.0f;
	m_flSpeedScale = 1.0f;
	m_flDoorBashYaw = 0.0f;

	// Default disable flags = enabled (1 = No disable) per FGD
	m_bDisableInmate = true;
	m_bDisableGuard = true;
	m_bDisableWorker = true;
	m_bDisableRural = true;
	m_bDisableDoctor = true;
	m_bDisableUniform = true;
	m_bDisableOffice = true;
	m_bDisableUrban = true;
}

// ---------------------------------------------------------------------------
// Precache – mirrors sub_101A5410
// ---------------------------------------------------------------------------
void CNPC_UH_Infected::Precache( void )
{
	BaseClass::Precache();

	for ( int i = 0; i < UH_INFECTED_VARIANT_COUNT; i++ )
	{
		PrecacheModel( s_InfectedVariants[i].pszModel );
		if ( s_InfectedVariants[i].pszGibLeftArm )
			PrecacheModel( s_InfectedVariants[i].pszGibLeftArm );
		if ( s_InfectedVariants[i].pszGibRightArm )
			PrecacheModel( s_InfectedVariants[i].pszGibRightArm );
		if ( s_InfectedVariants[i].pszGibLeftLeg )
			PrecacheModel( s_InfectedVariants[i].pszGibLeftLeg );
		if ( s_InfectedVariants[i].pszGibRightLeg )
			PrecacheModel( s_InfectedVariants[i].pszGibRightLeg );
		if ( s_InfectedVariants[i].pszHelmet )
			PrecacheModel( s_InfectedVariants[i].pszHelmet );
	}

	// Additional generic gibs from original precache list
	PrecacheModel( "models/items/worker_helmet.mdl" );
	PrecacheModel( "models/items/respirator.mdl" );
	PrecacheModel( "models/items/guard_helmet.mdl" );

	// Weapons that can be given as additionalequipment
	PrecacheModel( "models/weapons/w_pipe_pg.mdl" );
	PrecacheModel( "models/weapons/w_wrench_pg.mdl" );
	PrecacheModel( "models/weapons/w_axe_pg.mdl" );
	PrecacheModel( "models/weapons/w_baton_pg.mdl" );

	PrecacheScriptSound( "NPC_Infected.Attack" );
	PrecacheScriptSound( "NPC_Infected.Pain" );
	PrecacheScriptSound( "NPC_Infected.Death" );
	PrecacheScriptSound( "NPC_FastZombie.Moan1" );
	PrecacheScriptSound( "Zombie.FootstepLeft" );
	PrecacheScriptSound( "Zombie.FootstepRight" );
	PrecacheScriptSound( "Zombine.ScuffLeft" );
	PrecacheScriptSound( "Zombine.ScuffRight" );
	PrecacheScriptSound( "Zombie.AttackHit" );
	PrecacheScriptSound( "Zombie.AttackMiss" );
	PrecacheScriptSound( "Zombine.Die" );
	PrecacheScriptSound( "Zombine.Pain" );
	PrecacheScriptSound( "NPC_FastZombie.LeapAttack" );
	PrecacheScriptSound( "NPC_FastZombie.Attack" );
}

// ---------------------------------------------------------------------------
// Pick variant based on disable flags – mirrors sub_101A6FF0
// ---------------------------------------------------------------------------
void CNPC_UH_Infected::PickBodyVariant( void )
{
	CUtlVector<int> enabled;

	// FGD: 1 = enabled (No disable), 0 = disabled (Yes)
	if ( m_bDisableInmate )  enabled.AddToTail( 0 );
	if ( m_bDisableGuard )   enabled.AddToTail( 6 );
	if ( m_bDisableWorker )  enabled.AddToTail( 1 );
	if ( m_bDisableRural )   enabled.AddToTail( 5 );
	if ( m_bDisableDoctor )  enabled.AddToTail( 2 );
	if ( m_bDisableUniform ) enabled.AddToTail( 3 );
	if ( m_bDisableOffice )  enabled.AddToTail( 7 );
	if ( m_bDisableUrban )   enabled.AddToTail( 4 );

	if ( enabled.Count() == 0 )
	{
		// Everything disabled – fallback to inmate (original did same)
		m_iInfectedVariant = 0;
		return;
	}

	m_iInfectedVariant = enabled[random->RandomInt( 0, enabled.Count() - 1 )];
}

// ---------------------------------------------------------------------------
// Speed modifier – original sub_101A7B70 random 0.8..1.3 + SpeedModifier key
// ---------------------------------------------------------------------------
void CNPC_UH_Infected::ApplySpeedModifier( void )
{
	// If blank (-1), random 0.4..1.0 (per FGD). Else clamp 0..1
	if ( m_flSpeedModifier < -0.5f )
	{
		if ( uh_infected_randspeedenabled.GetBool() )
			m_flSpeedModifier = random->RandomFloat( 0.4f, 1.0f );
		else
			m_flSpeedModifier = 1.0f;
	}
	m_flSpeedModifier = clamp( m_flSpeedModifier, 0.0f, 1.0f );

	// Scale derived from original: 0.8 + rand*0.5 + SpeedModifier*0.2
	// Original at 0x106B93?? used formula: rand*0.000030518509*0.5+0.8
	float flRandBase = random->RandomFloat( 0.8f, 1.3f );
	m_flSpeedScale = flRandBase * (0.7f + m_flSpeedModifier * 0.6f);
	m_flSpeedScale = clamp( m_flSpeedScale, 0.4f, 1.8f );
}

// ---------------------------------------------------------------------------
// Bodygroups – approximate reconstruction of sub_101A6620
// ---------------------------------------------------------------------------
void CNPC_UH_Infected::ApplyBodygroups( void )
{
	// All infected have skin randomization
	int iSkin = random->RandomInt( 0, 5 );
	m_nSkin = iSkin;

	// Per-variant randomization (helmets, respirator, gloves, arms)
	int iVariant = m_iInfectedVariant;

	if ( iVariant == 1 ) // worker
	{
		// worker: 0..5 skin already, plus head 0..8 random
		int iHead = random->RandomInt( 0, 8 );
		SetBodygroup( FindBodygroupByName( "head" ), iHead );

		// Gloves
		if ( FindBodygroupByName( "Glove_L" ) >= 0 )
			SetBodygroup( FindBodygroupByName( "Glove_L" ), random->RandomInt( 0, 1 ) );
		if ( FindBodygroupByName( "Glove_R" ) >= 0 )
			SetBodygroup( FindBodygroupByName( "Glove_R" ), random->RandomInt( 0, 1 ) );

		// Arms – will be overridden if limp
	}
	else if ( iVariant == 2 ) // doctor
	{
		int iHead = random->RandomInt( 0, 8 );
		if ( FindBodygroupByName( "head" ) >= 0 )
			SetBodygroup( FindBodygroupByName( "head" ), iHead );
	}
	else if ( iVariant == 6 ) // guard
	{
		int iHelmet = random->RandomInt( 0, 3 );
		int iHead = random->RandomInt( 0, 9 );
		int iRespirator = random->RandomInt( 0, 1 );

		if ( FindBodygroupByName( "helmet" ) >= 0 )
			SetBodygroup( FindBodygroupByName( "helmet" ), iHelmet );
		if ( FindBodygroupByName( "head" ) >= 0 )
			SetBodygroup( FindBodygroupByName( "head" ), iHead );
		if ( FindBodygroupByName( "respirator" ) >= 0 )
			SetBodygroup( FindBodygroupByName( "respirator" ), iRespirator );

		if ( iHelmet == 3 )
		{
			// Visor down (original calls SetSequence? We set bodygroup)
			// Find and set VisorDown via animation event is complex, ignore
		}
	}
	else
	{
		int iHead = random->RandomInt( 0, 2 );
		int iBody = random->RandomInt( 0, 1 );
		if ( FindBodygroupByName( "head" ) >= 0 )
			SetBodygroup( FindBodygroupByName( "head" ), iHead );
		if ( FindBodygroupByName( "body" ) >= 0 )
			SetBodygroup( FindBodygroupByName( "body" ), iBody );
	}

	// Limp handling – random limb loss
	if ( m_bInfectedFlag )
	{
		BecomeLimp();
	}
}

void CNPC_UH_Infected::BecomeLimp( void )
{
	m_bIsLimping = true;

	int iArmsGroup = FindBodygroupByName( "arms" );
	if ( iArmsGroup >= 0 )
	{
		int iChoice = random->RandomInt( 0, 3 );
		// 0 = normal, 1 = left missing, 2 = right missing, 3 = both? Original had 7 etc
		// We map to bodygroup values that original used: 0 normal, 1 left, 2 right, 3 both?
		// Use count safe
		int iMax = GetBodygroupCount( iArmsGroup ) - 1;
		iChoice = clamp( iChoice, 0, iMax );
		SetBodygroup( iArmsGroup, iChoice );

		// Slow down if missing arm
		m_flSpeedScale *= 0.6f;
	}

	// For legs/hard limp, reduce speed more and set sequence?
	// Original set m_fIsTorso false but set move flags
	AddSolidFlags( FSOLID_NOT_STANDABLE );
}

// ---------------------------------------------------------------------------
// Additional equipment – comma separated list, pick random
// ---------------------------------------------------------------------------
void CNPC_UH_Infected::GiveAdditionalEquipment( void )
{
	if ( m_iszAdditionalEquipment == NULL_STRING )
		return;

	// Don't give weapon if limping severely (original: may not spawn with weapon)
	if ( m_bIsLimping && random->RandomInt( 0, 1 ) == 0 )
		return;

	const char *pszList = STRING( m_iszAdditionalEquipment );
	if ( !pszList || !*pszList )
		return;

	// Split by comma – manual split to avoid CUtlStringList (not in 2007 SDK)
	// We need to copy to mutable buffer on stack and strtok
	char szCopy[512];
	Q_strncpy( szCopy, pszList, sizeof(szCopy) );
	CUtlVector< char* > tokens;
	char *ctx = NULL;
	char *pszToken = strtok_s( szCopy, ",", &ctx );
	while ( pszToken )
	{
		while ( *pszToken == ' ' ) pszToken++;
		char *end = pszToken + Q_strlen( pszToken ) - 1;
		while ( end > pszToken && *end == ' ' ) { *end = '\0'; end--; }
		if ( *pszToken && Q_stricmp( pszToken, "Nothing" ) != 0 )
		{
			tokens.AddToTail( pszToken );
		}
		pszToken = strtok_s( NULL, ",", &ctx );
	}

	if ( tokens.Count() == 0 )
		return;

	const char *pszChosen = tokens[random->RandomInt( 0, tokens.Count() - 1 )];
	if ( !pszChosen )
		return;

	CBaseCombatWeapon *pWeapon = static_cast<CBaseCombatWeapon *>( CreateEntityByName( pszChosen ) );
	if ( pWeapon )
	{
		pWeapon->SetAbsOrigin( GetAbsOrigin() );
		pWeapon->SetAbsAngles( GetAbsAngles() );
		DispatchSpawn( pWeapon );
		Weapon_Equip( pWeapon );
	}
}

// ---------------------------------------------------------------------------
// Set model based on variant – called from BaseZombie Spawn chain
// ---------------------------------------------------------------------------
void CNPC_UH_Infected::SetZombieModel( void )
{
	if ( m_iInfectedVariant < 0 || m_iInfectedVariant >= UH_INFECTED_VARIANT_COUNT )
		PickBodyVariant();

	const UHInfectedVariant_t &var = s_InfectedVariants[m_iInfectedVariant];
	SetModel( var.pszModel );

	// Store gib models for later use (not needed here, but we keep indices for precache)
	SetHullType( HULL_HUMAN );
	SetHullSizeNormal();
}

// ---------------------------------------------------------------------------
// Spawn – full reconstruction
// ---------------------------------------------------------------------------
void CNPC_UH_Infected::Spawn( void )
{
	Precache();

	// Pick variant before base spawn because SetZombieModel needs it
	PickBodyVariant();

	// Random limp flag – original 25% chance (0..3 ==0)
	m_bInfectedFlag = ( random->RandomInt( 0, 3 ) == 0 );

	ApplySpeedModifier();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_RED );

	// Health from convar (original used sk_ + uh_infected_health)
	m_iHealth = uh_infected_health.GetInt();
	if ( m_iHealth <= 0 )
		m_iHealth = 50;
	m_flFieldOfView = 0.2f;

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_MOVE_JUMP | bits_CAP_MOVE_CLIMB | bits_CAP_INNATE_MELEE_ATTACK1 );
	CapabilitiesAdd( bits_CAP_DOORS_GROUP );
	CapabilitiesAdd( bits_CAP_SQUAD );

	m_flNextSprintTime = gpGlobals->curtime + random->RandomFloat( 1.0f, 4.0f );

	// Set model and hull
	SetZombieModel();

	// Bodygroups
	ApplyBodygroups();

	// Give weapon
	GiveAdditionalEquipment();

	// Underhell limb system. The original enables it (byte @1713 = 1) only in
	// the CNPC_UH_Infected constructor @463688 and seeds the per-bodypart
	// health pools right after it @463689-463693; every other NPC class zeroes
	// that byte. Enable it before BaseClass::Spawn so the pools are seeded
	// during Activate.
	m_bUhGibEnabled = true;

	BaseClass::Spawn();

	// Touch for climbing (original set touch to ClimbTouch)
	SetTouch( &CNPC_UH_Infected::ClimbTouch );

	// Set playback rate affected by speed
	SetPlaybackRate( m_flSpeedScale );

	NPCInit();

	// Random moan time
	m_flNextMoanTime = gpGlobals->curtime + random->RandomFloat( 0.5f, 2.0f );
}

// ---------------------------------------------------------------------------
// Sounds – mirror fast zombie but with infected variants
// ---------------------------------------------------------------------------
const char *CNPC_UH_Infected::GetMoanSound( int nSound )
{
	return "NPC_FastZombie.Moan1";
}

void CNPC_UH_Infected::PainSound( const CTakeDamageInfo &info )
{
	EmitSound( "Zombine.Pain" );
}

void CNPC_UH_Infected::DeathSound( const CTakeDamageInfo &info )
{
	EmitSound( "Zombine.Die" );
}

void CNPC_UH_Infected::AlertSound( void )
{
	EmitSound( "NPC_Infected.Attack" );
}

void CNPC_UH_Infected::IdleSound( void )
{
	if ( gpGlobals->curtime < m_flNextMoanTime )
		return;

	EmitSound( "NPC_FastZombie.Moan1" );
	m_flNextMoanTime = gpGlobals->curtime + random->RandomFloat( 2.0f, 5.0f );
}

void CNPC_UH_Infected::AttackSound( void )
{
	EmitSound( "NPC_FastZombie.Attack" );
}

void CNPC_UH_Infected::AttackHitSound( void )
{
	EmitSound( "Zombie.AttackHit" );
}

void CNPC_UH_Infected::AttackMissSound( void )
{
	EmitSound( "Zombie.AttackMiss" );
}

void CNPC_UH_Infected::FootstepSound( bool fRightFoot )
{
	if ( fRightFoot )
		EmitSound( "Zombie.FootstepRight" );
	else
		EmitSound( "Zombie.FootstepLeft" );
}

void CNPC_UH_Infected::FootscuffSound( bool fRightFoot )
{
	if ( fRightFoot )
		EmitSound( "Zombine.ScuffRight" );
	else
		EmitSound( "Zombine.ScuffLeft" );
}

// ---------------------------------------------------------------------------
// ClimbTouch – reconstructed from sub_101A5A10
// ---------------------------------------------------------------------------
void CNPC_UH_Infected::ClimbTouch( CBaseEntity *pOther )
{
	if ( !pOther )
		return;

	if ( pOther->IsPlayer() )
	{
		// Push player aside
		Vector vecDir = pOther->WorldSpaceCenter() - WorldSpaceCenter();
		vecDir.z = 0.0f;
		VectorNormalize( vecDir );
		vecDir *= 200.0f;

		pOther->VelocityPunch( vecDir );

		// If dismounting or blocked, set climb touch condition
		if ( GetActivity() != ACT_CLIMB_DISMOUNT ||
		     ( pOther->GetGroundEntity() == NULL && GetNavigator()->IsGoalActive() &&
		       pOther->GetAbsOrigin().z - GetNavigator()->GetCurWaypointPos().z < -1.0f ) )
		{
			SetCondition( COND_UH_INFECTED_CLIMB_TOUCH );
		}

		SetTouch( NULL );
	}
	else if ( FClassnameIs( pOther, "prop_physics" ) || dynamic_cast<CPhysicsProp *>( pOther ) )
	{
		// Create solver to push physics prop (original sub_101CABB0)
		NPCPhysics_CreateSolver( this, pOther, true, 5.0f );
	}
}

// ---------------------------------------------------------------------------
// Melee conditions – 64 units, dot 0.7, similar to fast zombie
// ---------------------------------------------------------------------------
int CNPC_UH_Infected::MeleeAttack1Conditions( float flDot, float flDist )
{
	if ( flDist > UH_INFECTED_MELEE_RANGE )
		return COND_TOO_FAR_TO_ATTACK;
	if ( flDot < 0.7f )
		return COND_NOT_FACING_ATTACK;
	return COND_CAN_MELEE_ATTACK1;
}

// ---------------------------------------------------------------------------
// GetSoundInterests – also listen for the Underhell FM radio / radiocracker
// attract sound (SOUND_FMRADIO) so OnListened() can raise COND_HEAR_FMRADIO.
// ---------------------------------------------------------------------------
int CNPC_UH_Infected::GetSoundInterests( void )
{
	return BaseClass::GetSoundInterests() | SOUND_FMRADIO;
}

// ---------------------------------------------------------------------------
// GatherConditions – infected ignores unreachable, handles climb touch
// ---------------------------------------------------------------------------
void CNPC_UH_Infected::GatherConditions( void )
{
	BaseClass::GatherConditions();

	if ( GetEnemy() )
	{
		ClearCondition( COND_ENEMY_UNREACHABLE );
	}

	// Random run condition
	if ( random->RandomInt( 0, 100 ) < 5 )
	{
		SetCondition( COND_UH_INFECTED_RANDOMRUN );
	}

	// Original serveror.dll attracted infected via CSoundEnt + COND_HEAR_FMRADIO.
	// The active radio (uh_radio) now emits SOUND_FMRADIO; the sound system raises
	// COND_HEAR_FMRADIO in OnListened() when we hear it, so propagate it to our
	// investigate schedule instead of scanning by distance.
	if ( HasCondition( COND_HEAR_FMRADIO ) && !HasCondition( COND_SEE_ENEMY ) )
	{
		SetCondition( COND_UH_INFECTED_GRENADE );
	}
}

// ---------------------------------------------------------------------------
// PrescheduleThink – sprint logic + moan
// ---------------------------------------------------------------------------
void CNPC_UH_Infected::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();

	if ( GetEnemy() && IsAlive() )
	{
		float flDist = (GetEnemy()->GetAbsOrigin() - GetAbsOrigin()).Length2D();
		if ( flDist < 512.0f && gpGlobals->curtime > m_flNextSprintTime )
		{
			// Occupy sprint squad slot if available
			if ( OccupyStrategySlot( SQUAD_SLOT_UH_INFECTED_SPRINT1 ) || OccupyStrategySlot( SQUAD_SLOT_UH_INFECTED_SPRINT2 ) )
			{
				// Sprint – increase speed for short time
				m_flNextSprintTime = gpGlobals->curtime + random->RandomFloat( 3.5f, 6.0f );
			}
		}
	}

	// Reset climb touch condition after time
	if ( HasCondition( COND_UH_INFECTED_CLIMB_TOUCH ) && gpGlobals->curtime > m_flNextClimbTime )
	{
		ClearCondition( COND_UH_INFECTED_CLIMB_TOUCH );
	}
}

// ---------------------------------------------------------------------------
// BuildScheduleTestBits – interrupt on climb touch, door block
// ---------------------------------------------------------------------------
void CNPC_UH_Infected::BuildScheduleTestBits( void )
{
	BaseClass::BuildScheduleTestBits();

	if ( IsCurSchedule( SCHED_CHASE_ENEMY ) || IsCurSchedule( SCHED_RUN_RANDOM ) )
	{
		SetCustomInterruptCondition( COND_UH_INFECTED_CLIMB_TOUCH );
		SetCustomInterruptCondition( COND_BLOCKED_BY_DOOR );
	}

	if ( GetActivity() == ACT_CLIMB_UP || GetActivity() == ACT_CLIMB_DOWN || GetActivity() == ACT_CLIMB_DISMOUNT )
	{
		SetCustomInterruptCondition( COND_UH_INFECTED_CLIMB_TOUCH );
	}
}

// ---------------------------------------------------------------------------
// SelectSchedule – main infected intelligence
// ---------------------------------------------------------------------------
int CNPC_UH_Infected::SelectSchedule( void )
{
	if ( HasCondition( COND_ZOMBIE_RELEASECRAB ) )
		return SCHED_ZOMBIE_RELEASECRAB;

	if ( HasCondition( COND_UH_INFECTED_CLIMB_TOUCH ) )
		return SCHED_UH_INFECTED_UNSTICK_JUMP;

	if ( HasCondition( COND_BLOCKED_BY_DOOR ) )
	{
		// Bash the door currently blocking us (base zombie door logic handles the rest).
		m_flDoorBashYaw = GetAbsAngles().y;
		return SCHED_ZOMBIE_BASH_DOOR;
	}

	if ( HasCondition( COND_UH_INFECTED_GRENADE ) )
	{
		// Investigate radio (original also listened for COND_HEAR_FMRADIO = 60)
		return SCHED_UH_INFECTED_INVESTIGATE_RADIO;
	}

	if ( HasCondition( COND_CAN_MELEE_ATTACK1 ) )
	{
		return SCHED_MELEE_ATTACK1;
	}

	if ( HasCondition( COND_NEW_ENEMY ) && GetEnemy() )
	{
		if ( OccupyStrategySlotRange( SQUAD_SLOT_UH_INFECTED_SPRINT1, SQUAD_SLOT_UH_INFECTED_SPRINT2 ) )
		{
			return SCHED_CHASE_ENEMY;
		}
	}

	return BaseClass::SelectSchedule();
}

int CNPC_UH_Infected::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	if ( failedSchedule == SCHED_CHASE_ENEMY || failedSchedule == SCHED_RUN_RANDOM )
	{
		if ( taskFailCode == FAIL_NO_ROUTE || taskFailCode == FAIL_NO_ROUTE_GOAL )
		{
			return SCHED_UH_INFECTED_UNSTICK_JUMP;
		}
	}

	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}

int CNPC_UH_Infected::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
		case SCHED_CHASE_ENEMY:
			{
				// If close, sprint
				if ( GetEnemy() && (GetEnemy()->GetAbsOrigin() - GetAbsOrigin()).Length2D() < 256.0f )
				{
					// Use run panicked activity for sprint (original uses ACT_RUN_PANICKED)
					// We return chase but with faster playback
				}
				return SCHED_CHASE_ENEMY;
			}
		case SCHED_UH_INFECTED_UNSTICK_JUMP:
			if ( GetActivity() == ACT_CLIMB_UP || GetActivity() == ACT_CLIMB_DOWN || GetActivity() == ACT_CLIMB_DISMOUNT )
				return SCHED_UH_INFECTED_CLIMBING_UNSTICK_JUMP;
			return SCHED_UH_INFECTED_UNSTICK_JUMP;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

// ---------------------------------------------------------------------------
// StartTask / RunTask – unstick jump, door bash, radio
// ---------------------------------------------------------------------------
void CNPC_UH_Infected::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
		case TASK_UH_INFECTED_UNSTICK_JUMP:
			{
				SetGroundEntity( NULL );
				SetActivity( ACT_IDLE );

				// Jump away from wall/enemy
				Vector vecJumpDir;
				if ( GetEnemy() )
				{
					vecJumpDir = GetLocalOrigin() - GetEnemy()->GetLocalOrigin();
					VectorNormalize( vecJumpDir );
					vecJumpDir.z = 0.3f;
				}
				else
				{
					AngleVectors( GetLocalAngles(), &vecJumpDir );
					vecJumpDir = -vecJumpDir;
					vecJumpDir.z = 0.3f;
				}

				ApplyAbsVelocityImpulse( vecJumpDir * 300.0f + Vector( 0, 0, 200 ) );

				// Take off ground
				UTIL_SetOrigin( this, GetLocalOrigin() + Vector( 0, 0, 1 ) );

				TaskComplete();
				break;
			}

		case TASK_ZOMBIE_YAW_TO_DOOR:
			{
				if ( m_hBlockingDoor != NULL )
				{
					GetMotor()->SetIdealYaw( m_flDoorBashYaw );
				}
				TaskComplete();
				break;
			}

		case TASK_ZOMBIE_ATTACK_DOOR:
			{
				// Play wall pound activity
				SetIdealActivity( (Activity)ACT_ZOMBIE_WALLPOUND );
				m_DurationDoorBash.Reset();
				break;
			}

		case TASK_ZOMBIE_BREAKTHROUG:
			{
				// Attempt to open/break door
				if ( m_hBlockingDoor != NULL )
				{
					// If it's a breakable or prop_door_rotating, apply damage
					CTakeDamageInfo info( this, this, 50.0f, DMG_CLUB );
					m_hBlockingDoor->TakeDamage( info );

					// If door is still blocked, try to open
					CBasePropDoor *pPropDoor = dynamic_cast<CBasePropDoor *>( m_hBlockingDoor.Get() );
					if ( pPropDoor )
					{
						pPropDoor->NPCOpenDoor( this );
					}
					else
					{
						CBaseDoor *pDoor = dynamic_cast<CBaseDoor *>( m_hBlockingDoor.Get() );
						if ( pDoor )
						{
							pDoor->Use( this, this, USE_ON, 0 );
						}
					}
				}
				TaskComplete();
				break;
			}

		case TASK_UH_RADIO_PICKUP:
			{
				// Find nearest fmradio (including active uh_radio)
				CBaseEntity *pRadio = gEntList.FindEntityByClassname( NULL, "uh_radio" );
				if ( !pRadio )
					pRadio = gEntList.FindEntityByClassname( NULL, "item_fmradio" );
				if ( !pRadio )
					pRadio = gEntList.FindEntityByClassname( NULL, "item_radiocracker" );
				if ( pRadio )
				{
					SetTarget( pRadio );
					TaskComplete();
				}
				else
				{
					TaskFail( "No radio" );
				}
				break;
			}

		case TASK_UH_DESTROY_RADIO:
			{
				CBaseEntity *pTarget = GetTarget();
				if ( pTarget && ( FClassnameIs( pTarget, "item_fmradio" ) || FClassnameIs( pTarget, "item_radiocracker" ) || FClassnameIs( pTarget, "uh_radio" ) ) )
				{
					EmitSound( "NPC_FastZombie.Attack" );

					// If it's a radio cracker (either classname item_radiocracker or uh_radio with cracker flag), explode
					bool bIsCracker = false;
					if ( FClassnameIs( pTarget, "item_radiocracker" ) )
						bIsCracker = true;
					else
					{
						CUHRadio *pRadio = dynamic_cast<CUHRadio *>( pTarget );
						if ( pRadio && pRadio->IsCracker() )
							bIsCracker = true;
					}

					if ( bIsCracker )
					{
						CUHRadio *pRadio = dynamic_cast<CUHRadio *>( pTarget );
						if ( pRadio )
							pRadio->Explode();
						else
						{
					// Fallback: explode the world item_radiocracker with the same
					// formula as CUHRadio::Explode() (serveror.dll sub_10173A20).
					float flDamage = sk_plr_dmg_smg1_grenade.GetFloat();
					ExplosionCreate( pTarget->GetAbsOrigin(), QAngle(0,0,0), this, flDamage, 256, 1064, 50000.0f, this, -1 );
					UTIL_Remove( pTarget );
						}
					}
					else
					{
						UTIL_Remove( pTarget );
					}
				}
				TaskComplete();
				break;
			}

		case TASK_UH_THROW_ITEM:
			{
				// Infected throws nothing? Original had throw item for butcher? Skip
				TaskComplete();
				break;
			}

		default:
			BaseClass::StartTask( pTask );
			break;
	}
}

void CNPC_UH_Infected::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
		case TASK_UH_INFECTED_UNSTICK_JUMP:
			if ( GetFlags() & FL_ONGROUND )
				TaskComplete();
			break;

		case TASK_ZOMBIE_ATTACK_DOOR:
			if ( IsActivityFinished() )
			{
				if ( m_DurationDoorBash.Expired() )
				{
					TaskComplete();
				}
				else
				{
					ResetIdealActivity( (Activity)ACT_ZOMBIE_WALLPOUND );
				}
			}
			break;

		default:
			BaseClass::RunTask( pTask );
			break;
	}
}

// ---------------------------------------------------------------------------
// HandleAnimEvent – fast attack
// ---------------------------------------------------------------------------
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

			// Claw
			if ( pEnemy->IsPlayer() || pEnemy->IsNPC() )
			{
				CTakeDamageInfo info( this, this, UH_INFECTED_MELEE_DAMAGE, DMG_SLASH );
				info.SetDamagePosition( pEnemy->WorldSpaceCenter() );
				pEnemy->TakeDamage( info );

				// Lunge forward
				ApplyAbsVelocityImpulse( vecDir * 200.0f );

				// Blood
				if ( pEnemy->BloodColor() != DONT_BLEED )
				{
					SpawnBlood( pEnemy->GetAbsOrigin(), vecDir, pEnemy->BloodColor(), 6 );
				}
			}
		}
		AttackHitSound();
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_ATTACK_RIGHT )
	{
		Vector right, forward;
		AngleVectors( GetLocalAngles(), &forward, &right, NULL );
		right = right * -50;
		ClawAttack( GetClawAttackRange(), UH_INFECTED_MELEE_DAMAGE, QAngle( -3, -5, -3 ), right, ZOMBIE_BLOOD_RIGHT_HAND );
		return;
	}

	if ( pEvent->event == AE_ZOMBIE_ATTACK_LEFT )
	{
		Vector right, forward;
		AngleVectors( GetLocalAngles(), &forward, &right, NULL );
		right = right * 50;
		ClawAttack( GetClawAttackRange(), UH_INFECTED_MELEE_DAMAGE, QAngle( -3, 5, -3 ), right, ZOMBIE_BLOOD_LEFT_HAND );
		return;
	}

	BaseClass::HandleAnimEvent( pEvent );
}

Activity CNPC_UH_Infected::NPC_TranslateActivity( Activity baseAct )
{
	if ( baseAct == ACT_RUN )
	{
		// If limping, use walk?
		if ( m_bIsLimping )
			return ACT_WALK;

		// If sprinting, use run_panicked (original uses ACT_RUN_PANICKED for sprint)
		if ( HasCondition( COND_SEE_ENEMY ) )
		{
			if ( HaveSequenceForActivity( (Activity)ACT_RUN_PANICKED ) )
				return (Activity)ACT_RUN_PANICKED;
		}
		return ACT_RUN;
	}

	if ( baseAct == ACT_MELEE_ATTACK1 )
	{
		if ( HaveSequenceForActivity( (Activity)ACT_UH_INFECTED_ATTACK_FAST ) )
			return (Activity)ACT_UH_INFECTED_ATTACK_FAST;
	}

	if ( baseAct == ACT_CLIMB_DOWN )
		return ACT_CLIMB_UP;

	return BaseClass::NPC_TranslateActivity( baseAct );
}

// ---------------------------------------------------------------------------
// Damage / death
// ---------------------------------------------------------------------------
int CNPC_UH_Infected::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	if ( info.GetDamage() > 0.0f )
	{
		PainSound( info );
	}

	// TraceAttack in CAI_BaseNPC already sends the real hitgroup and impact
	// position to UH_ConsiderGib.  Do not manufacture a head hit here: damage
	// callbacks contain no hitgroup and the old code removed infected heads on
	// any damage above 10.

	return BaseClass::OnTakeDamage_Alive( info );
}

void CNPC_UH_Infected::Event_Killed( const CTakeDamageInfo &info )
{
	DeathSound( info );
	m_OnSpotInfectedBody.FireOutput( this, this );

	BaseClass::Event_Killed( info );
}

// ---------------------------------------------------------------------------
// Inputs
// ---------------------------------------------------------------------------
void CNPC_UH_Infected::InputSetSpeedModifier( inputdata_t &inputdata )
{
	m_flSpeedModifier = inputdata.value.Float();
	ApplySpeedModifier();
}
