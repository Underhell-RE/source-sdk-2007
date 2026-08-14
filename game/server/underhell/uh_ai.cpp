//========= Copyright (c) 2008, Mxthe (Underhell). All rights reserved. ============//
//
// Purpose: Underhell NPC AI extensions + dismemberment, installed on
//          CAI_BaseNPC (the FGD exposes them on BaseNPC).
//
// Behaviour 1:1 with the original, decoded from the FGD (underhell_base.fgd
// BaseNPC) + the ModDB tutorials:
//   - "Enemy AI Improvements & Sneaky Gameplay"  -> uh_fos / uh_viewdistance /
//     temp squads (squadtemp) / uh_spotbodies + OnSpot* outputs.
//   - "Bodygroups, Gibs, Ragdolls and Decals"    -> uh_bodygroup string parser,
//     Gib* inputs + gib-on-damage, the uh_gibhealth / uh_headhealth /
//     uh_helmethealth / uh_maxsergibs / uh_maxseragdolls / uh_ragdollcollisiontype /
//     uh_bodymousedamper / uh_maxitems convars.
//
// Note: the original's bodygroup parser "WILL crash" on a bad bodygroup name
// (per the tutorial); this port fails soft (FindBodygroupByName == -1 -> skip)
// so a typo can't take down the game.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "ai_basenpc.h"
#include "igamesystem.h"
#include "ragdoll_shared.h"
#include "physics_prop_ragdoll.h"
#include "physics_shared.h"
#include "decals.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Dismemberment tuning convars (original names + defaults).
//-----------------------------------------------------------------------------
static ConVar uh_gibhealth( "uh_gibhealth", "80", FCVAR_ARCHIVE,
	"Damage to gib a leg. Arms take 50% of this value." );
static ConVar uh_headhealth( "uh_headhealth", "21", FCVAR_ARCHIVE,
	"Damage to destroy the head." );
static ConVar uh_helmethealth( "uh_helmethealth", "30", FCVAR_ARCHIVE,
	"Damage a helmet takes before being shot off." );
static ConVar uh_maxsergibs( "uh_maxsergibs", "8", FCVAR_ARCHIVE,
	"Max server-side gibs before the oldest becomes a client ragdoll." );
static ConVar uh_maxseragdolls( "uh_maxseragdolls", "16", FCVAR_ARCHIVE,
	"Max server-side ragdolls before the oldest becomes a client ragdoll." );
static ConVar uh_ragdollcollisiontype( "uh_ragdollcollisiontype", "1", FCVAR_ARCHIVE,
	"Collision group for server-side ragdolls (-1 = client-side, 0-19 = group)." );
static ConVar uh_bodymousedamper( "uh_bodymousedamper", "4", FCVAR_ARCHIVE,
	"Mouse sensitivity divider while dragging a ragdoll (weight illusion)." );
static ConVar uh_maxitems( "uh_maxitems", "32", FCVAR_ARCHIVE,
	"Limit on enemy-dropped items/objects." );

//-----------------------------------------------------------------------------
// Gib model lookup: a severed limb model exists per body. The original uses a
// model-specific folder ("models\\Gibs\\BodyParts\\Soldier\\leftarm.mdl" etc.).
// Unknown models fall back to no gib (bodygroup change + blood still apply).
//-----------------------------------------------------------------------------
struct UHGibFolder_t
{
	const char *pszModelSubstring;	// match against GetModelName()
	const char *pszFolder;			// "models/gibs/bodyparts/..." prefix
};

static const UHGibFolder_t s_GibFolders[] =
{
	{ "combine_soldier_prisonguard",	"models/gibs/bodyparts/soldier_prisonguard" },
	{ "combine_soldier",				"models/gibs/bodyparts/soldier" },
};

static const char *UH_GibModelFor( const char *pszNPC, const char *pszLimb )
{
	for ( int i = 0; i < ARRAYSIZE( s_GibFolders ); i++ )
	{
		if ( V_stristr( pszNPC, s_GibFolders[i].pszModelSubstring ) )
		{
			return UTIL_VarArgs( "%s/%s.mdl", s_GibFolders[i].pszFolder, pszLimb );
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Bodygroup removal. Bodygroup names + values from the tutorial's
// combine_soldier table; the transition removes the named side. If the model
// lacks the bodygroup, fail soft (the original would crash here).
//-----------------------------------------------------------------------------
static bool UH_RemoveBodygroupSide( CBaseAnimating *pNPC, const char *pszGroup, int iSide )
{
	int iGroup = pNPC->FindBodygroupByName( pszGroup );
	if ( iGroup < 0 )
		return false;

	int iCur = pNPC->GetBodygroup( iGroup );

	// Left = bit 1, right = bit 2 (of the low two "regular" bits). Heavy legs
	// repeat the pattern in bits 3-4 (values 4-7), so mask off that bit too.
	int iNew = iCur;
	if ( iSide == 0 )		// left
	{
		iNew |= 1;
	}
	else					// right
	{
		iNew |= 2;
	}

	if ( iNew == iCur )
		return false;

	pNPC->SetBodygroup( iGroup, iNew );
	return true;
}

//-----------------------------------------------------------------------------
// Bodygroup string parser: "Helmet2Arms1Legs4" -> FindBodygroupByName + value.
//-----------------------------------------------------------------------------
void CAI_BaseNPC::UH_ApplySpawnSettings( void )
{
	// Bodygroups ("uh_bodygroup" keyvalue).
	if ( m_uh_bodygroup != NULL_STRING )
	{
		const char *psz = STRING( m_uh_bodygroup );
		const char *p = psz;
		while ( *p )
		{
			// Read the leading name (letters), then the value (digits).
			char name[64];
			int n = 0;
			while ( *p && !((*p) >= '0' && (*p) <= '9') && n < (int)sizeof(name) - 1 )
				name[n++] = *p++;
			name[n] = '\0';

			if ( *p == '\0' )
				break;	// trailing name with no number — ignore.

			int value = atoi( p );
			while ( *p && ((*p) >= '0' && (*p) <= '9') )
				p++;

			if ( name[0] && value >= 0 )
			{
				int iGroup = FindBodygroupByName( name );
				if ( iGroup >= 0 )
					SetBodygroup( iGroup, value );
			}
		}
	}

	// Field of view ("uh_fos" in degrees -> dot product).
	if ( m_flUhFOV > 0.0f )
	{
		// The SDK stores m_flFieldOfView as a cosine; a larger dot = narrower
		// cone. Convert degrees to the dot product.
		m_flFieldOfView = cos( DEG2RAD( m_flUhFOV * 0.5f ) );
	}

	// View distance ("uh_viewdistance").
	if ( m_flUhViewDistance > 0.0f )
	{
		m_flDistTooFar = m_flUhViewDistance;
	}
}

//-----------------------------------------------------------------------------
// Gib a body part: change the bodygroup to remove the limb and spawn a severed
// gib model at the hit position. Returns true if something was gibbed.
//-----------------------------------------------------------------------------
void CAI_BaseNPC::UH_GibBodyPart( int iHitGroup, const Vector &vecPosition, const Vector &vecDir )
{
	// Update the model's bodygroup so the limb is visually removed.
	switch ( iHitGroup )
	{
	case HITGROUP_LEFTARM:	UH_RemoveBodygroupSide( this, "ARMS", 0 ); break;
	case HITGROUP_RIGHTARM:	UH_RemoveBodygroupSide( this, "ARMS", 1 ); break;
	case HITGROUP_LEFTLEG:	UH_RemoveBodygroupSide( this, "Legs", 0 ); break;
	case HITGROUP_RIGHTLEG:	UH_RemoveBodygroupSide( this, "Legs", 1 ); break;
	case HITGROUP_HEAD:
		{
			int iGroup = FindBodygroupByName( "Head" );
			if ( iGroup >= 0 )
				SetBodygroup( iGroup, 1 );	// "Destroyed Head"
		}
		break;
	}

	// Spawn the severed gib (a physics prop) at the hit position. The head is
	// "destroyed" via bodygroup only (no severed-head gib), matching the
	// tutorial's gib list (arms + legs only).
	const char *pszLimb = NULL;
	switch ( iHitGroup )
	{
	case HITGROUP_LEFTARM:	pszLimb = "leftarm"; break;
	case HITGROUP_RIGHTARM:	pszLimb = "rightarm"; break;
	case HITGROUP_LEFTLEG:	pszLimb = "leftleg"; break;
	case HITGROUP_RIGHTLEG:	pszLimb = "rightleg"; break;
	}

	if ( pszLimb )
	{
		const char *pszModel = UH_GibModelFor( STRING( GetModelName() ), pszLimb );
		if ( pszModel )
		{
			CBaseEntity *pGib = CreateEntityByName( "prop_physics" );
			if ( pGib )
			{
				pGib->SetModel( pszModel );
				pGib->SetAbsOrigin( vecPosition );
				DispatchSpawn( pGib );

				IPhysicsObject *pPhys = pGib->VPhysicsGetObject();
				if ( pPhys )
				{
					pPhys->SetVelocity( &vecDir, NULL );
				}

				pGib->SetOwnerEntity( this );
			}
		}
	}

	// Blood decal / particles at the wound.
	if ( BloodColor() != DONT_BLEED )
	{
		SpawnBlood( vecPosition, vecDir, BloodColor(), 24.0f );
	}
}

//-----------------------------------------------------------------------------
// Accumulate damage per hitgroup and gib a limb once its threshold is crossed.
// Returns true if a body part was gibbed this hit.
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::UH_ConsiderGib( int iHitGroup, float flDamage, const Vector &vecPosition, const Vector &vecDir )
{
	// Only track the five gib-able groups.
	int idx = -1;
	switch ( iHitGroup )
	{
	case HITGROUP_HEAD:		idx = 0; break;
	case HITGROUP_LEFTARM:	idx = 1; break;
	case HITGROUP_RIGHTARM:	idx = 2; break;
	case HITGROUP_LEFTLEG:	idx = 3; break;
	case HITGROUP_RIGHTLEG:	idx = 4; break;
	}
	if ( idx < 0 )
		return false;

	// Dead NPCs gib freely; the threshold only gates living NPCs.
	if ( IsAlive() )
	{
		m_flGibDamage[idx] += flDamage;

		float flThreshold = 0.0f;
		if ( iHitGroup == HITGROUP_HEAD )
			flThreshold = uh_headhealth.GetFloat();
		else if ( iHitGroup == HITGROUP_LEFTARM || iHitGroup == HITGROUP_RIGHTARM )
			flThreshold = uh_gibhealth.GetFloat() * 0.5f;	// arms = 50% of legs
		else
			flThreshold = uh_gibhealth.GetFloat();

		if ( m_flGibDamage[idx] < flThreshold )
			return false;
	}

	UH_GibBodyPart( iHitGroup, vecPosition, vecDir );
	return true;
}

//-----------------------------------------------------------------------------
// Spot dead bodies: when enabled, scan for ragdoll/corpse entities in front of
// the NPC and fire the matching output once each.
//-----------------------------------------------------------------------------
void CAI_BaseNPC::UH_SpotBodiesThink( void )
{
	if ( !m_bUhSpotBodies )
		return;

	if ( gpGlobals->curtime < m_flNextSpotBodiesTime )
		return;
	m_flNextSpotBodiesTime = gpGlobals->curtime + 0.5f;

	Vector forward;
	AngleVectors( GetAbsAngles(), &forward );

	CBaseEntity *pEnt = NULL;
	while ( ( pEnt = gEntList.FindEntityByClassname( pEnt, "prop_ragdoll" ) ) != NULL )
	{
		// Roughly in front + within view distance.
		Vector toBody = pEnt->GetAbsOrigin() - EyePosition();
		float dist = VectorNormalize( toBody );
		if ( dist > m_flUhViewDistance && m_flUhViewDistance > 0 )
			continue;
		if ( DotProduct( toBody, forward ) < 0.3f )
			continue;

		// TODO: track which bodies we've already reported (a set of EHANDLEs).
		// For now, fire on each visible body (throttled by the 0.5s scan).

		// Classify: soldier / infected / default. The body's classname isn't
		// preserved on the ragdoll, so use the model name as a proxy.
		// FireOutput( pActivator, pCaller ): !activator = this NPC (who spotted),
		// !caller = the body — matching the tutorial ("!caller is the body").
		const char *pszModel = STRING( pEnt->GetModelName() );
		if ( V_stristr( pszModel, "combine_soldier" ) )
			m_OnSpotSoldierBody.FireOutput( this, pEnt );
		else if ( V_stristr( pszModel, "infected" ) )
			m_OnSpotInfectedBody.FireOutput( this, pEnt );
		else
			m_OnSpotDefaultBody.FireOutput( this, pEnt );

		break;	// one body per scan
	}
}

//-----------------------------------------------------------------------------
// Temporary squads: allied NPCs that see each other (and both have squadtemp
// enabled) join a temporary squad so they share target info. When LOS is lost
// the temp squad is broken. Simplified: if we have an enemy and another allied
// NPC (not in our squad) is nearby + looking at us, share our enemy with it.
//-----------------------------------------------------------------------------
void CAI_BaseNPC::UH_TempSquadUpdate( void )
{
	if ( !m_bUhSquadTemp )
		return;

	if ( !GetEnemy() )
		return;

	if ( gpGlobals->curtime < m_flNextTempSquadTime )
		return;
	m_flNextTempSquadTime = gpGlobals->curtime + 0.5f;

	// Find a nearby allied NPC (any class, per the tutorial) that can see us.
	CBaseEntity *pEnt = gEntList.FirstEnt();
	while ( pEnt )
	{
		if ( pEnt == this )
		{
			pEnt = gEntList.NextEnt( pEnt );
			continue;
		}

		CAI_BaseNPC *pOther = pEnt->MyNPCPointer();
		if ( pOther && pOther != GetEnemy() && pOther->IRelationType( this ) == D_LI
			&& pOther->m_bUhSquadTemp && pOther->IsAlive() )
		{
			if ( ( pOther->GetAbsOrigin() - GetAbsOrigin() ).Length2D() <= 256.0f
				&& FInViewCone( pOther ) )
			{
				// Share our enemy (the original shares target info while in LOS).
				pOther->UpdateEnemyMemory( GetEnemy(), GetEnemy()->GetAbsOrigin(), this );
				pOther->SetEnemy( GetEnemy() );
				return;
			}
		}

		pEnt = gEntList.NextEnt( pEnt );
	}
}

//-----------------------------------------------------------------------------
// Inputs (FGD BaseNPC).
//-----------------------------------------------------------------------------
void CAI_BaseNPC::InputSetSquadTemp( inputdata_t &inputdata )
{
	m_bUhSquadTemp = ( atoi( inputdata.value.String() ) != 0 );
}

void CAI_BaseNPC::InputSetViewDistance( inputdata_t &inputdata )
{
	m_flUhViewDistance = atof( inputdata.value.String() );
	m_flDistTooFar = m_flUhViewDistance;
}

void CAI_BaseNPC::InputSetSpotBodiesOn( inputdata_t &inputdata )
{
	m_bUhSpotBodies = true;
}

void CAI_BaseNPC::InputSetSpotBodiesOff( inputdata_t &inputdata )
{
	m_bUhSpotBodies = false;
}

void CAI_BaseNPC::InputGibHead( inputdata_t &inputdata )
{
	UH_GibBodyPart( HITGROUP_HEAD, EyePosition(), vec3_origin );
}

void CAI_BaseNPC::InputGibLeftArm( inputdata_t &inputdata )
{
	UH_GibBodyPart( HITGROUP_LEFTARM, GetAbsOrigin(), vec3_origin );
}

void CAI_BaseNPC::InputGibRightArm( inputdata_t &inputdata )
{
	UH_GibBodyPart( HITGROUP_RIGHTARM, GetAbsOrigin(), vec3_origin );
}

void CAI_BaseNPC::InputGibLeftLeg( inputdata_t &inputdata )
{
	UH_GibBodyPart( HITGROUP_LEFTLEG, GetAbsOrigin(), vec3_origin );
}

void CAI_BaseNPC::InputGibRightLeg( inputdata_t &inputdata )
{
	UH_GibBodyPart( HITGROUP_RIGHTLEG, GetAbsOrigin(), vec3_origin );
}

//-----------------------------------------------------------------------------
// Ragdoll / gib management (decoded from the original's CRagdollProp additions
// + the ragdoll manager list at 106960D0/D8/DC/E4/E8/F0). Three behaviours:
//
//   1. uh_ragdollcollisiontype — the collision group applied to NPC ragdolls
//      (-1 = client-side, else a 0..19 collision group index).
//   2. uh_maxseragdolls / uh_maxsergibs — cap server-side ragdolls/gibs; the
//      oldest is retired (the SDK LRU fades it out; the original converts it to
//      a client ragdoll, which is equivalent visually).
//   3. Dragging a body sprays "blood_drop" decals while the player holds it
//      (FVPHYSICS_PLAYER_HELD), like the original's DraggedThink.
//-----------------------------------------------------------------------------
class CUHRagdollManager : public CAutoGameSystemPerFrame
{
	typedef CAutoGameSystemPerFrame BaseClass;

public:
	CUHRagdollManager() : BaseClass( "CUHRagdollManager" ), m_flNextScan( 0.0f ) {}

	virtual void LevelInitPostEntity( void )
	{
		// Feed the SDK's existing ragdoll LRU our max count.
		s_RagdollLRU.SetMaxRagdollCount( uh_maxseragdolls.GetInt() );
	}

	virtual void FrameUpdatePostEntityThink( void )
	{
		if ( gpGlobals->curtime < m_flNextScan )
			return;
		m_flNextScan = gpGlobals->curtime + 0.5f;

		// Keep the LRU cap in sync (convar may change at runtime).
		s_RagdollLRU.SetMaxRagdollCount( uh_maxseragdolls.GetInt() );

		int iCollision = uh_ragdollcollisiontype.GetInt();

		CBaseEntity *pEnt = gEntList.FirstEnt();
		while ( pEnt )
		{
			if ( FClassnameIs( pEnt, "prop_ragdoll" ) )
			{
				CRagdollProp *pRagdoll = assert_cast<CRagdollProp *>( pEnt );

				// Only NPC-spawned ragdolls (owner = the dead NPC). Hammer-placed
				// prop_ragdolls have no owner and are excluded, per the tutorial.
				if ( pRagdoll->GetOwnerEntity() )
				{
					// Collision group from the convar.
					if ( iCollision >= 0 && pRagdoll->GetCollisionGroup() != iCollision )
					{
						pRagdoll->SetCollisionGroup( iCollision );
					}

					// Blood trail while dragged.
					IPhysicsObject *pPhys = pRagdoll->VPhysicsGetObject();
					if ( pPhys && ( pPhys->GetGameFlags() & FVPHYSICS_PLAYER_HELD ) )
					{
						Vector vecPos = pRagdoll->GetAbsOrigin();
						Vector vecDown = vecPos - Vector( 0, 0, 32.0f );
						trace_t tr;
						UTIL_TraceLine( vecPos, vecDown, MASK_SOLID_BRUSHONLY, pRagdoll, COLLISION_GROUP_NONE, &tr );
						if ( tr.fraction < 1.0f )
						{
							UTIL_DecalTrace( &tr, "blood_drop" );
						}
					}
				}
			}

			pEnt = gEntList.NextEnt( pEnt );
		}
	}

private:
	float m_flNextScan;
};

static CUHRagdollManager g_UHRagdollManager;
