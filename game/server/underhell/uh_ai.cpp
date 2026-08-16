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
#include "ai_senses.h"
#include "igamesystem.h"
#include "ragdoll_shared.h"
#include "physics_prop_ragdoll.h"
#include "physics_shared.h"
#include "decals.h"
#include "particle_parse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Dismemberment tuning convars (original names + defaults).
//-----------------------------------------------------------------------------
ConVar uh_gibhealth( "uh_gibhealth", "80", FCVAR_ARCHIVE,
	"Bodypart health pool: legs get half this value, arms a quarter (original @463690)." );
ConVar uh_headhealth( "uh_headhealth", "21", FCVAR_ARCHIVE,
	"Head health pool: the head gets half this value (original @463689)." );
ConVar uh_helmethealth( "uh_helmethealth", "30", FCVAR_ARCHIVE,
	"Helmet health before it is shot off (original @463685, no divisor)." );
static ConVar uh_maxsergibs( "uh_maxsergibs", "8", FCVAR_ARCHIVE,
	"Max server-side gibs before the oldest becomes a client ragdoll." );
static ConVar uh_maxseragdolls( "uh_maxseragdolls", "16", FCVAR_ARCHIVE,
	"Max server-side ragdolls before the oldest becomes a client ragdoll." );
static ConVar uh_ragdollcollisiontype( "uh_ragdollcollisiontype", "1", FCVAR_ARCHIVE,
	"Collision group for server-side ragdolls (-1 = client-side, 0-19 = group)." );
ConVar uh_bodymousedamper( "uh_bodymousedamper", "4", FCVAR_ARCHIVE,
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
	const char *pszLimbPrefix;		// "" or a prefix like "pmc_" / "inmate_"
};

static int UH_FindBodygroup( CBaseAnimating *pBody, const char *pszName );

static const UHGibFolder_t s_GibFolders[] =
{
	{ "combine_soldier_prisonguard",	"models/gibs/bodyparts/soldier_prisonguard",	"" },
	{ "combine_soldier",				"models/gibs/bodyparts/soldier",				"" },
	{ "infected_inmate",				"models/gibs/bodyparts/infected",				"inmate_" },
	{ "infected_guard",					"models/gibs/bodyparts/infected",				"guard_" },
	{ "infected_worker",				"models/gibs/bodyparts/infected",				"worker_" },
	{ "infected_rural",					"models/gibs/bodyparts/infected",				"rural_" },
	{ "infected_doctor",				"models/gibs/bodyparts/infected",				"doctor_" },
	{ "infected_uniform",				"models/gibs/bodyparts/infected",				"uniform_" },
	{ "infected_office",				"models/gibs/bodyparts/infected",				"office_" },
	{ "infected_urban",					"models/gibs/bodyparts/infected",				"urban_" },
	{ "pmc",							"models/gibs/bodyparts/pmc",					"pmc_" },
};

static const char *UH_GibModelFor( CBaseAnimating *pBody, const char *pszLimb )
{
	const char *pszNPC = STRING( pBody->GetModelName() );
	for ( int i = 0; i < ARRAYSIZE( s_GibFolders ); i++ )
	{
		if ( V_stristr( pszNPC, s_GibFolders[i].pszModelSubstring ) )
		{
			// CNPC_CombineS selects the second leg meshes whenever its authored
			// Legs bodygroup is the heavy variant (value 4).  They are soldier
			// meshes even for the heavy prison model.
			if ( !V_stricmp( pszLimb, "leftleg" ) || !V_stricmp( pszLimb, "rightleg" ) )
			{
				int iLegs = UH_FindBodygroup( pBody, "legs" );
				if ( iLegs >= 0 && pBody->GetBodygroup( iLegs ) >= 4 &&
					( V_stristr( pszNPC, "combine_soldier" ) || V_stristr( pszNPC, "prisonguard" ) ) )
				{
					return UTIL_VarArgs( "models/gibs/bodyparts/soldier/%s2.mdl", pszLimb );
				}
			}
			return UTIL_VarArgs( "%s/%s%s.mdl", s_GibFolders[i].pszFolder, s_GibFolders[i].pszLimbPrefix, pszLimb );
		}
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Bodygroup removal. Bodygroup names + values from the tutorial's
// combine_soldier table; the transition removes the named side. If the model
// lacks the bodygroup, fail soft (the original would crash here).
//-----------------------------------------------------------------------------
// sub_10031BF0 does not use a generic left/right bitfield.  The authored
// combine models encode arm-loss states differently: regular soldiers use
// 0/2/4/6 while prison/worker/PMC variants use 0/1/2/3.  Treating them as
// bits 1/2 (the old implementation) selected intact or wrong geometry.
static int UH_FindBodygroup( CBaseAnimating *pBody, const char *pszName )
{
	int i = pBody->FindBodygroupByName( pszName );
	if ( i >= 0 )
		return i;

	char alternate[64];
	V_strncpy( alternate, pszName, sizeof( alternate ) );
	if ( alternate[0] >= 'a' && alternate[0] <= 'z' )
		alternate[0] -= 'a' - 'A';
	else if ( alternate[0] >= 'A' && alternate[0] <= 'Z' )
		alternate[0] += 'a' - 'A';
	return pBody->FindBodygroupByName( alternate );
}

static bool UH_RemoveBodygroupSide( CBaseAnimating *pBody, const char *pszGroup, int iSide )
{
	int iGroup = UH_FindBodygroup( pBody, pszGroup );
	if ( iGroup < 0 )
		return false;

	const int iCurrent = pBody->GetBodygroup( iGroup );
	const int iCount = pBody->GetBodygroupCount( iGroup );
	const char *pszModel = STRING( pBody->GetModelName() );
	int iNew = iCurrent;

	if ( !V_stricmp( pszGroup, "arms" ) )
	{
		// From sub_10031BF0 cases 4/5.  Soldier arm bodygroups reserve bit 0
		// for the intact variant, therefore left/right loss is +2/+4.  The
		// 4-state families use +1/+2.
		const bool bSixStateSoldier =
			V_stristr( pszModel, "combine_soldier" ) &&
			!V_stristr( pszModel, "prisonguard" ) && iCount > 4;
		const int iMask = bSixStateSoldier ? ( iSide == 0 ? 2 : 4 ) : ( iSide == 0 ? 1 : 2 );
		iNew = iCurrent | iMask;
	}
	else if ( !V_stricmp( pszGroup, "legs" ) )
	{
		// The leg bodygroup has ordinary and heavy sets.  Preserve the heavy
		// base (bit 2) and mark the severed side in its low bits.
		const int iMask = iSide == 0 ? 1 : 2;
		iNew = iCurrent | iMask;
	}
	else
	{
		iNew = iCurrent | ( iSide == 0 ? 1 : 2 );
	}

	if ( iNew == iCurrent || iNew >= iCount )
		return false;
	pBody->SetBodygroup( iGroup, iNew );
	pBody->ResetSequenceInfo();
	return true;
}

// sub_10031BF0 uses model-family-specific destroyed-head variants. In
// particular, value 1 is valid for the prison guard but leaves a normal
// combine soldier with an intact/incorrect head; soldiers use the high
// destroyed-head variants instead.
static int UH_DestroyedHeadBodygroup( CBaseAnimating *pBody )
{
	const char *pszModel = STRING( pBody->GetModelName() );
	if ( V_stristr( pszModel, "combine_soldier_prisonguard" ) )
		return 1;
	if ( V_stristr( pszModel, "combine_soldier" ) )
	{
		// CNPC_CombineS path in sub_10031BF0: normal combine variants use
		// destroyed head 10, or 11 when their current head is already one of
		// the high (helmet/gear) variants. Value 9 belongs to other NPC types.
		int iHead = UH_FindBodygroup( pBody, "head" );
		const int iDestroyed = ( iHead >= 0 && pBody->GetBodygroup( iHead ) >= 9 ) ? 11 : 10;
		// Some map replacements use a reduced combine model.  Never assign an
		// out-of-range studio body value: that leaves the networked ragdoll in
		// an invalid state rather than producing the destroyed head.
		if ( iHead >= 0 && iDestroyed < pBody->GetBodygroupCount( iHead ) )
			return iDestroyed;
	}
	return 1;
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
	else
	{
		// No explicit bodygroups: randomize armor / helmet / legs, exactly like
		// the original (sub_10338D30): Armor 0-2, Helmet 0-5, Legs 0 or 4
		// (heavy). This is what gives combine soldiers their helmets + armor
		// variety. Clamped to each bodygroup's actual variant count.
		int iArmor = FindBodygroupByName( "Armor" );
		if ( iArmor >= 0 )
			SetBodygroup( iArmor, random->RandomInt( 0, min( 2, GetBodygroupCount( iArmor ) - 1 ) ) );

		int iHelmet = FindBodygroupByName( "Helmet" );
		if ( iHelmet >= 0 )
			SetBodygroup( iHelmet, random->RandomInt( 0, min( 5, GetBodygroupCount( iHelmet ) - 1 ) ) );

		int iLegs = FindBodygroupByName( "Legs" );
		if ( iLegs >= 0 )
		{
			int iHeavy = ( GetBodygroupCount( iLegs ) > 4 ) ? 4 : 0;
			SetBodygroup( iLegs, random->RandomInt( 0, 1 ) ? iHeavy : 0 );
		}
	}

	// Underhell keeps dead NPCs as server-side ragdolls (pickable + dismemberable),
	// capped by uh_maxseragdolls. Vanilla only does this for vehicle kills / the
	// mega-physcannon, otherwise it becomes a client ragdoll ("soft body").
	m_bForceServerRagdoll = true;

	// Field of view ("uh_fos" in degrees -> dot product).
	if ( m_flUhFOV > 0.0f )
	{
		// The SDK stores m_flFieldOfView as a cosine; a larger dot = narrower
		// cone. Convert degrees to the dot product.
		m_flFieldOfView = cos( DEG2RAD( m_flUhFOV * 0.5f ) );
	}

	// View distance ("uh_viewdistance"). The original (sub_10022930) sets BOTH
	// the NPC's "too far to attack" distance AND the senses' look distance
	// (CAI_Senses::m_LookDist), so the soldier can't SEE the player past that
	// range. Setting only m_flDistTooFar left the senses at the 2048 default.
	if ( m_flUhViewDistance > 0.0f )
	{
		m_flDistTooFar = m_flUhViewDistance;
		GetSenses()->SetDistLook( m_flUhViewDistance );
	}
}

//-----------------------------------------------------------------------------
// Precache the severed-limb and helmet models for this NPC's body, so they are
// available when dismemberment / helmet-loss spawns them dynamically.
//-----------------------------------------------------------------------------
void CAI_BaseNPC::UH_PrecacheGibModels( void )
{
	const char *pszModel = STRING( GetModelName() );

	// Severed limbs (arms + legs) — lowercase paths, matching serveror.dll.
	if ( V_stristr( pszModel, "combine_soldier_prisonguard" ) )
	{
		PrecacheModel( "models/gibs/bodyparts/soldier_prisonguard/leftarm.mdl" );
		PrecacheModel( "models/gibs/bodyparts/soldier_prisonguard/rightarm.mdl" );
		PrecacheModel( "models/gibs/bodyparts/soldier_prisonguard/leftleg.mdl" );
		PrecacheModel( "models/gibs/bodyparts/soldier_prisonguard/rightleg.mdl" );
	}
	else if ( V_stristr( pszModel, "combine_soldier" ) )
	{
		PrecacheModel( "models/gibs/bodyparts/soldier/leftarm.mdl" );
		PrecacheModel( "models/gibs/bodyparts/soldier/rightarm.mdl" );
		PrecacheModel( "models/gibs/bodyparts/soldier/leftleg.mdl" );
		PrecacheModel( "models/gibs/bodyparts/soldier/rightleg.mdl" );
		PrecacheModel( "models/gibs/bodyparts/soldier/leftleg2.mdl" );
		PrecacheModel( "models/gibs/bodyparts/soldier/rightleg2.mdl" );
	}

	// Helmet / respirator / gasmask drops (the original spawns item_* entities,
	// which precache their own models) + the knock-off sound.
	if ( V_stristr( pszModel, "prisonguard" ) )
	{
		PrecacheModel( "models/items/helmet.mdl" );
		PrecacheModel( "models/items/gasmask.mdl" );
	}
	else if ( V_stristr( pszModel, "combine_soldier" ) )
	{
		PrecacheModel( "models/items/helmet_visor.mdl" );
		PrecacheModel( "models/items/respirator.mdl" );
	}
	if ( V_stristr( pszModel, "pmc" ) )
		PrecacheModel( "models/items/pmc_helmet.mdl" );

	// Dismemberment blood sprays + sounds (1:1 with sub_10021D80 precache).
	PrecacheParticleSystem( "blood_zombie_split_spray" );
	PrecacheParticleSystem( "blood_advisor_puncture_withdraw" );
	PrecacheScriptSound( "Player.Helmet" );
	PrecacheScriptSound( "Player.Splat" );
	PrecacheScriptSound( "Player.HeadShot" );
}

//-----------------------------------------------------------------------------
// Spawn a severed limb as a ragdoll prop (matches the original sub_101CDCC0,
// which creates a "prop_ragdoll" with the per-bodypart gib model). A ragdoll
// limb flops naturally; a plain prop_physics with these models renders as an
// invisible/static body.
//-----------------------------------------------------------------------------
static CBaseEntity *UH_SpawnGibProp( const char *pszModel, const Vector &vecPosition, const QAngle &angPosition, const Vector &vecDir, CBaseEntity *pOwner )
{
	CBaseAnimating *pAnimatingOwner = pOwner ? pOwner->GetBaseAnimating() : NULL;
	if ( !pAnimatingOwner )
		return NULL;

	// This is the SDK equivalent of serveror's sub_101CDCC0.  It copies the
	// source skeleton through a temporary bone merge before initializing VPhysics;
	// spawning an independent prop_ragdoll lost that pose and made detached limbs
	// start in their reference/T pose.
	CBaseAnimating *pGib = CreateServerRagdollSubmodel(
		pAnimatingOwner, pszModel, vecPosition, angPosition, COLLISION_GROUP_DEBRIS );
	if ( !pGib )
		return NULL;

	// sub_10031BF0 explicitly copies the source skin to its sub-ragdoll.
	pGib->m_nSkin = pAnimatingOwner->m_nSkin;

	// Inherit the current corpse motion.  InitRagdoll receives a pose, not the
	// parent physics object's linear/angular velocity; leaving it at zero made
	// detached legs solve their first frame as a violent, comic launch.
	IPhysicsObject *pSourcePhysics = pAnimatingOwner->VPhysicsGetObject();
	IPhysicsObject *pGibPhysics = pGib->VPhysicsGetObject();
	if ( pSourcePhysics && pGibPhysics )
	{
		Vector vecVelocity;
		AngularImpulse angVelocity;
		pSourcePhysics->GetVelocity( &vecVelocity, &angVelocity );
		pGibPhysics->SetVelocity( &vecVelocity, &angVelocity );
	}
	(void)vecDir;

	pGib->SetOwnerEntity( pOwner );
	return pGib;
}

// The original creates a sub-ragdoll at the authored sever attachment, not
// at the bullet impact point.  The impact may be anywhere along a forearm or
// calf and produced visibly detached/floating gibs in the old port.
static void UH_GetLimbSpawnTransform( CBaseAnimating *pBody, int iHitGroup, const Vector &vecFallback, Vector &vecOrigin, QAngle &angOrigin )
{
	const char *pszAttachment = NULL;
	switch ( iHitGroup )
	{
	case HITGROUP_LEFTARM:  pszAttachment = "ForeArm_L"; break;
	case HITGROUP_RIGHTARM: pszAttachment = "ForeArm_R"; break;
	case HITGROUP_LEFTLEG:  pszAttachment = "Calf_L"; break;
	case HITGROUP_RIGHTLEG: pszAttachment = "Calf_R"; break;
	}
	vecOrigin = vecFallback;
	angOrigin = vec3_angle;
	if ( pszAttachment )
		pBody->GetAttachment( pszAttachment, vecOrigin, angOrigin );
}

// sub_10031BF0 copies the relevant glove variant to an arm sub-ragdoll.
// Without this, a gloved soldier leaves behind a bare detached hand because a
// new prop starts with model bodygroups at zero.
static void UH_CopyLimbBodygroups( CBaseAnimating *pSource, CBaseAnimating *pGib, int iHitGroup )
{
	const char *pszGlove = NULL;
	if ( iHitGroup == HITGROUP_LEFTARM )
		pszGlove = "Glove_L";
	else if ( iHitGroup == HITGROUP_RIGHTARM )
		pszGlove = "Glove_R";
	if ( !pszGlove )
		return;

	int iSource = UH_FindBodygroup( pSource, pszGlove );
	int iGib = UH_FindBodygroup( pGib, pszGlove );
	if ( iSource >= 0 && iGib >= 0 )
		pGib->SetBodygroup( iGib, min( pSource->GetBodygroup( iSource ), pGib->GetBodygroupCount( iGib ) - 1 ) );
}

//-----------------------------------------------------------------------------
// Blood sprays for a severed limb — 1:1 with sub_10031BF0:
//   arms: "blood_zombie_split_spray" on the body ("UpperArm_L/R") + on the
//         severed gib ("ForeArm_L/R")
//   legs: "blood_advisor_puncture_withdraw" on the severed gib ("Calf_L/R")
//   head: "Blood_Trace" decal (handled separately via the trace)
//-----------------------------------------------------------------------------
static void UH_DispatchLimbBlood( CBaseAnimating *pBody, int iHitGroup, CBaseAnimating *pGib, const Vector &vecPosition, const Vector &vecDir )
{
	const char *pszParticle = NULL;
	const char *pszBodyAttach = NULL;
	const char *pszGibAttach = NULL;

	switch ( iHitGroup )
	{
	case HITGROUP_LEFTARM:
		pszParticle = "blood_zombie_split_spray";
		pszBodyAttach = "UpperArm_L";
		pszGibAttach = "ForeArm_L";
		break;
	case HITGROUP_RIGHTARM:
		pszParticle = "blood_zombie_split_spray";
		pszBodyAttach = "UpperArm_R";
		pszGibAttach = "ForeArm_R";
		break;
	case HITGROUP_LEFTLEG:
		pszParticle = "blood_advisor_puncture_withdraw";
		pszGibAttach = "Calf_L";
		break;
	case HITGROUP_RIGHTLEG:
		pszParticle = "blood_advisor_puncture_withdraw";
		pszGibAttach = "Calf_R";
		break;
	case HITGROUP_HEAD:
		// Decal, not a particle: trace backwards from the impact and lay the
		// "Blood_Trace" (headshot "brainy" blood) decal.
		{
			trace_t tr;
			UTIL_TraceLine( vecPosition, vecPosition - vecDir * 16.0f, MASK_SHOT, pBody, COLLISION_GROUP_NONE, &tr );
			if ( tr.fraction < 1.0f )
				UTIL_DecalTrace( &tr, "Blood_Trace" );
		}
		return;
	}

	if ( !pszParticle )
		return;

	if ( pBody && pszBodyAttach )
	{
		DispatchParticleEffect( pszParticle, PATTACH_POINT_FOLLOW, pBody, pszBodyAttach );
	}
	if ( pGib && pszGibAttach )
	{
		DispatchParticleEffect( pszParticle, PATTACH_POINT_FOLLOW, pGib, pszGibAttach );
	}
}

// Select the exact item family used by the original CNPC_CombineS branches.
// PMC is tested before generic combine because its model may use a soldier
// skeleton/bodygroups but drops its own helmet item.
static const char *UH_HelmetItemFor( CBaseAnimating *pBody )
{
	const char *pszModel = STRING( pBody->GetModelName() );
	if ( V_stristr( pszModel, "pmc" ) )
		return "item_helmet_pmc";
	if ( V_stristr( pszModel, "prisonguard" ) )
		return "item_helmet_prison";
	if ( V_stristr( pszModel, "worker" ) )
		return "item_helmet_worker";
	return "item_helmet_guard";
}

//-----------------------------------------------------------------------------
// Shoot the helmet off: HELMET bodygroup -> 0 and drop the helmet model.
//-----------------------------------------------------------------------------
void CAI_BaseNPC::UH_ShootOffHelmet( const Vector &vecPosition, const Vector &vecDir )
{
	int iGroup = UH_FindBodygroup( this, "helmet" );
	if ( iGroup < 0 || GetBodygroup( iGroup ) < 1 )
		return;	// no helmet worn

	SetBodygroup( iGroup, 0 );

	// The original spawns an item_helmet_* entity (pickable as armor), chosen
	// by the NPC's body: prison guard -> plain helmet, soldier -> visored
	// helmet, worker -> worker helmet, pmc -> pmc helmet.
	const char *pszItem = UH_HelmetItemFor( this );

	CBaseEntity *pHelmet = CreateEntityByName( pszItem );
	if ( pHelmet )
	{
		// sub_10031BF0 drops head equipment from Eyes, rather than from the
		// arbitrary bullet impact on the hitbox.
		Vector vecDropOrigin = vecPosition;
		QAngle angDrop = vec3_angle;
		GetAttachment( "Eyes", vecDropOrigin, angDrop );
		pHelmet->SetAbsOrigin( vecDropOrigin );
		pHelmet->SetAbsAngles( angDrop );
		DispatchSpawn( pHelmet );

		IPhysicsObject *pPhys = pHelmet->VPhysicsGetObject();
		if ( pPhys )
		{
			Vector vecDropVelocity = vecDir * 0.25f;
			pPhys->SetVelocity( &vecDropVelocity, NULL );
		}
	}

	EmitSound( "Player.Helmet" );
}

//-----------------------------------------------------------------------------
// Remove a worn gear bodygroup and drop the matching item (1:1 with
// sub_10031BF0, which spawns item_respirator_guard / item_gasmask_guard when
// a head is destroyed while a respirator / gasmask is worn).
//-----------------------------------------------------------------------------
static void UH_DropGearItem( CBaseAnimating *pNPC, const char *pszBodygroup, const char *pszItem, const Vector &vecPosition, const Vector &vecDir )
{
	int iGroup = UH_FindBodygroup( pNPC, pszBodygroup );
	if ( iGroup < 0 || pNPC->GetBodygroup( iGroup ) < 1 )
		return;	// not worn

	pNPC->SetBodygroup( iGroup, 0 );

	CBaseEntity *pItem = CreateEntityByName( pszItem );
	if ( !pItem )
		return;

	Vector vecDropOrigin = vecPosition;
	QAngle angDrop = vec3_angle;
	pNPC->GetAttachment( "Eyes", vecDropOrigin, angDrop );
	pItem->SetAbsOrigin( vecDropOrigin );
	pItem->SetAbsAngles( angDrop );
	DispatchSpawn( pItem );

	IPhysicsObject *pPhys = pItem->VPhysicsGetObject();
	if ( pPhys )
	{
		Vector vecDropVelocity = vecDir * 0.25f;
		pPhys->SetVelocity( &vecDropVelocity, NULL );
	}
}

//-----------------------------------------------------------------------------
// Gib a body part: change the bodygroup to remove the limb and spawn a severed
// gib model at the hit position.
//-----------------------------------------------------------------------------
void CAI_BaseNPC::UH_GibBodyPart( int iHitGroup, const Vector &vecPosition, const Vector &vecDir )
{
	// Bodygroups are the authoritative state shared by the living NPC and the
	// server ragdoll created on death. Do not spawn duplicate gibs once a part
	// is already absent.
	bool bRemoved = false;
	switch ( iHitGroup )
	{
	case HITGROUP_LEFTARM:	bRemoved = UH_RemoveBodygroupSide( this, "arms", 0 ); break;
	case HITGROUP_RIGHTARM:	bRemoved = UH_RemoveBodygroupSide( this, "arms", 1 ); break;
	case HITGROUP_LEFTLEG:	bRemoved = UH_RemoveBodygroupSide( this, "legs", 0 ); break;
	case HITGROUP_RIGHTLEG:	bRemoved = UH_RemoveBodygroupSide( this, "legs", 1 ); break;
	case HITGROUP_HEAD:
		{
			int iGroup = UH_FindBodygroup( this, "head" );
			int iDestroyed = UH_DestroyedHeadBodygroup( this );
			if ( iGroup >= 0 && GetBodygroup( iGroup ) != iDestroyed )
			{
				SetBodygroup( iGroup, iDestroyed );
				bRemoved = true;
			}

			UH_DropGearItem( this, "respirator", "item_respirator_guard", vecPosition, vecDir );
			UH_DropGearItem( this, "gasmask",
				V_stristr( STRING( GetModelName() ), "prisonguard" ) ? "item_gasmask_prison" : "item_gasmask_guard",
				vecPosition, vecDir );
		}
		break;
	}
	if ( !bRemoved )
		return;

	// Spawn the severed gib (a pickable physics prop) at the hit position. The
	// head is "destroyed" via bodygroup only (no severed-head gib), matching
	// the tutorial's gib list (arms + legs only).
	const char *pszLimb = NULL;
	switch ( iHitGroup )
	{
	case HITGROUP_LEFTARM:	pszLimb = "leftarm"; break;
	case HITGROUP_RIGHTARM:	pszLimb = "rightarm"; break;
	case HITGROUP_LEFTLEG:	pszLimb = "leftleg"; break;
	case HITGROUP_RIGHTLEG:	pszLimb = "rightleg"; break;
	}

	CBaseEntity *pGib = NULL;
	if ( pszLimb )
	{
		const char *pszModel = UH_GibModelFor( this, pszLimb );
		if ( pszModel )
		{
			Vector vecGibOrigin;
			QAngle angGibOrigin;
			UH_GetLimbSpawnTransform( this, iHitGroup, vecPosition, vecGibOrigin, angGibOrigin );
			pGib = UH_SpawnGibProp( pszModel, vecGibOrigin, angGibOrigin, vecDir, this );
			if ( pGib )
				UH_CopyLimbBodygroups( this, pGib->GetBaseAnimating(), iHitGroup );
		}
	}

	// The original clears old decals and emits the splat once a limb is actually
	// severed (not while merely accumulating damage).
	RemoveAllDecals();
	EmitSound( "Player.Splat" );

	// Arms/legs use their authored particles. The original uses the separate
	// headshot spray and sentence only when the head transition succeeds.
	UH_DispatchLimbBlood( this, iHitGroup, pGib ? pGib->GetBaseAnimating() : NULL, vecPosition, vecDir );
	if ( iHitGroup == HITGROUP_HEAD )
	{
		// Original string is "Player.Headshot" (lowercase 's').
		EmitSound( "Player.Headshot" );
		UTIL_BloodSpray( vecPosition, vecDir, BLOOD_COLOR_RED, 8, FX_BLOODSPRAY_ALL );
	}

	// Losing an arm takes the NPC's weapon out of play. The original
	// (sub_10031BF0 @130952/@130990) clears byte 1712 once the arm bodygroup
	// state passes the "both arms gone" mark, and either drops the held weapon
	// or strips the shoot capability depending on the current activity.
	// Without this an armless NPC keeps aiming and firing from thin air.
	if ( iHitGroup == HITGROUP_LEFTARM || iHitGroup == HITGROUP_RIGHTARM )
		UH_OnArmSevered();
}

//-----------------------------------------------------------------------------
// Arm loss consequences (original @130952-130975 / @130990-131018).
//-----------------------------------------------------------------------------
void CAI_BaseNPC::UH_OnArmSevered( void )
{
	// Track both arms through the gib pools: a pool at/below zero means that
	// arm has already been severed.
	const bool bLeftGone  = ( m_iGibHealth[1] <= 0 );
	const bool bRightGone = ( m_iGibHealth[2] <= 0 );

	CBaseCombatWeapon *pWeapon = GetActiveWeapon();
	if ( pWeapon )
	{
		// The original checks the current activity (34/35/36 in its enum) and
		// only physically drops the weapon in those states; otherwise it just
		// removes the ability to shoot. Approximate that with "is the NPC
		// mid-animation on something other than a normal stand/aim pose".
		Activity eActivity = GetActivity();
		const bool bDropIt = ( eActivity == ACT_RANGE_ATTACK1 ||
							   eActivity == ACT_RANGE_ATTACK2 ||
							   eActivity == ACT_RELOAD ||
							   bLeftGone );

		if ( bDropIt )
		{
			Weapon_Drop( pWeapon );
		}
		else
		{
			CapabilitiesRemove( bits_CAP_WEAPON_RANGE_ATTACK1 | bits_CAP_WEAPON_RANGE_ATTACK2 );
		}
	}

	if ( bLeftGone && bRightGone )
	{
		// Both arms gone: no weapons at all from here on.
		m_bUhCanUseWeapons = false;
		CapabilitiesRemove( bits_CAP_WEAPON_RANGE_ATTACK1 | bits_CAP_WEAPON_RANGE_ATTACK2 |
							bits_CAP_WEAPON_MELEE_ATTACK1 | bits_CAP_WEAPON_MELEE_ATTACK2 );

		CBaseCombatWeapon *pHeld = GetActiveWeapon();
		if ( pHeld )
			Weapon_Drop( pHeld );
	}
}

//-----------------------------------------------------------------------------
// Accumulate damage per hitgroup and gib a limb once its threshold is crossed.
// A worn helmet absorbs head damage and is shot off (uh_helmethealth) before
// the head itself can be destroyed (uh_headhealth).
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Seed the per-bodypart health pools. Original constructor @463685-463693:
//     helmet = uh_helmethealth        (30)
//     head   = uh_headhealth  / 2     (21/2 = 10)
//     arms   = uh_gibhealth   / 4     (80/4 = 20 each)
//     legs   = uh_gibhealth   / 2     (80/2 = 40 each)
// Note the divisors: an earlier port used the raw convar values for the head
// and legs and half of uh_gibhealth for the arms, i.e. exactly double the
// original cost for every part.
//-----------------------------------------------------------------------------
void CAI_BaseNPC::UH_InitGibHealth( void )
{
	m_iHelmetHealth		= uh_helmethealth.GetInt();
	m_iGibHealth[0]		= uh_headhealth.GetInt() / 2;		// head
	m_iGibHealth[1]		= uh_gibhealth.GetInt() / 4;		// left arm
	m_iGibHealth[2]		= uh_gibhealth.GetInt() / 4;		// right arm
	m_iGibHealth[3]		= uh_gibhealth.GetInt() / 2;		// left leg
	m_iGibHealth[4]		= uh_gibhealth.GetInt() / 2;		// right leg
}

bool CAI_BaseNPC::UH_ConsiderGib( int iHitGroup, float flDamage, const Vector &vecPosition, const Vector &vecDir )
{
	// Original gate @130572: byte 1713 is "dismemberment enabled for this NPC",
	// checked before anything else. It is NOT an IsAlive() test.
	if ( !m_bUhGibEnabled )
		return false;

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

	// Head: a worn helmet absorbs the damage until it is shot off.
	if ( iHitGroup == HITGROUP_HEAD )
	{
		int iHelmet = UH_FindBodygroup( this, "helmet" );
		if ( iHelmet >= 0 && GetBodygroup( iHelmet ) >= 1 )
		{
			m_iHelmetHealth -= (int)flDamage;
			if ( m_iHelmetHealth <= 0 )
			{
				UH_ShootOffHelmet( vecPosition, vecDir );
				m_iHelmetHealth = 0;
			}
			return true;	// helmet absorbed the hit
		}
	}

	// Subtract from the remaining pool and sever once it is exhausted. The
	// original applies this to living AND dead NPCs alike — there is no
	// IsAlive() branch in sub_10031BF0, so shooting a corpse apart costs the
	// same damage as dismembering a living one. The previous port severed a
	// corpse limb on the very first bullet.
	m_iGibHealth[idx] -= (int)flDamage;
	if ( m_iGibHealth[idx] > 0 )
		return false;

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

void CAI_BaseNPC::InputSetFos( inputdata_t &inputdata )
{
	// Original sub_100228B0: FOS in degrees -> m_flFieldOfView = cos(fos/2).
	m_flUhFOV = atof( inputdata.value.String() );
	if ( m_flUhFOV > 0.0f )
	{
		m_flFieldOfView = cos( DEG2RAD( m_flUhFOV * 0.5f ) );
	}
}

void CAI_BaseNPC::InputSetViewDistance( inputdata_t &inputdata )
{
	m_flUhViewDistance = atof( inputdata.value.String() );
	m_flDistTooFar = m_flUhViewDistance;
	GetSenses()->SetDistLook( m_flUhViewDistance );
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
		m_flNextScan = gpGlobals->curtime + 0.25f;

		// Keep the LRU cap in sync (convar may change at runtime).
		s_RagdollLRU.SetMaxRagdollCount( uh_maxseragdolls.GetInt() );

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
					// sub_101CCD80 (DraggedThink): a held corpse leaves a blood
					// decal only after it has moved more than four units in XY, then
					// updates its anchor every 0.25 s. The former implementation
					// sprayed a decal continuously even while the body was stationary.
					IPhysicsObject *pPhys = pRagdoll->VPhysicsGetObject();
					bool bHeld = pPhys && ( pPhys->GetGameFlags() & FVPHYSICS_PLAYER_HELD );
					Vector vecPos = pRagdoll->GetAbsOrigin();
					if ( bHeld )
					{
						if ( pRagdoll->m_bUHDragged &&
							( fabs( vecPos.x - pRagdoll->m_vecUHDraggedLastPos.x ) > 4.0f ||
							  fabs( vecPos.y - pRagdoll->m_vecUHDraggedLastPos.y ) > 4.0f ) )
						{
							Vector vecDown = vecPos - Vector( 0, 0, 32.0f );
							trace_t tr;
							UTIL_TraceLine( vecPos, vecDown, MASK_SOLID_BRUSHONLY, pRagdoll, COLLISION_GROUP_NONE, &tr );
							if ( tr.fraction < 1.0f )
								UTIL_DecalTrace( &tr, "blood_drop" );
						}
						pRagdoll->m_bUHDragged = true;
						pRagdoll->m_vecUHDraggedLastPos = vecPos;
					}
					else
					{
						pRagdoll->m_bUHDragged = false;
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

//-----------------------------------------------------------------------------
// Ragdoll dismemberment: shoot limbs off a dead body. Called from
// CRagdollProp::TraceAttack. Accumulates per-hitgroup damage and, once a limb's
// threshold is crossed, severs it (breaks the ragdoll constraint) and spawns a
// gib at the hit position — matching the original's "ragdolls can be
// dismembered completely".
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Map a ragdoll physics bone -> hitgroup. Ragdolls are SOLID_VPHYSICS, so a
// bullet trace against them returns physicsbone but leaves hitgroup = generic
// (CRagdollProp::TestCollision only sets tr.physicsbone). Recover the hitgroup
// from the physics bone's studio bone via the model's hitbox set.
//-----------------------------------------------------------------------------
// Resolve a studio bone to its hitgroup. A ragdoll physics element often uses
// a parent of the hitbox bone, so callers also walk its physics parents.
static int UH_StudioBoneToHitgroup( CStudioHdr *pHdr, int iStudioBone )
{
	if ( !pHdr || iStudioBone < 0 )
		return HITGROUP_GENERIC;

	for ( int iSet = 0; iSet < pHdr->numhitboxsets(); iSet++ )
	{
		mstudiohitboxset_t *pSet = pHdr->pHitboxSet( iSet );
		for ( int i = 0; i < pSet->numhitboxes; i++ )
		{
			mstudiobbox_t *pBox = pSet->pHitbox( i );
			if ( pBox->bone == iStudioBone )
				return pBox->group;
		}
	}
	return HITGROUP_GENERIC;
}

static int UH_RagdollBoneToHitgroup( CRagdollProp *pRagdoll, int iPhysicsBone )
{
	ragdoll_t *pRagdollPhys = pRagdoll->GetRagdoll();
	CStudioHdr *pHdr = pRagdoll->GetModelPtr();
	if ( iPhysicsBone < 0 || iPhysicsBone >= pRagdollPhys->listCount || !pHdr )
		return HITGROUP_GENERIC;

	// Check the struck element then its parents. This covers a physics bone
	// such as a forearm whose hitbox is authored on the upper-arm bone.
	for ( int iElement = iPhysicsBone; iElement >= 0; iElement = pRagdollPhys->list[iElement].parentIndex )
	{
		int iHitgroup = UH_StudioBoneToHitgroup( pHdr, pRagdollPhys->boneIndex[iElement] );
		if ( iHitgroup != HITGROUP_GENERIC )
			return iHitgroup;
	}
	return HITGROUP_GENERIC;
}

void UH_RagdollDismember( CRagdollProp *pRagdoll, int iHitGroup, float flDamage, int iPhysicsBone, const Vector &pos, const Vector &dir )
{
	if ( !pRagdoll )
		return;

	// A bullet trace against a ragdoll reports hitgroup = generic (see
	// CRagdollProp::TestCollision); recover the real hitgroup from the
	// physics bone so limb damage / helmet knock-off actually trigger.
	if ( iHitGroup == HITGROUP_GENERIC )
		iHitGroup = UH_RagdollBoneToHitgroup( pRagdoll, iPhysicsBone );

	// Helmet knock-off (head hit while the corpse still wears a helmet):
	// accumulate helmet damage, and once past uh_helmethealth, remove the
	// HELMET bodygroup + drop the helmet item + play the sound. Mirrors the
	// living-NPC path in sub_10031BF0.
	if ( iHitGroup == HITGROUP_HEAD )
	{
		int iHelmet = UH_FindBodygroup( pRagdoll, "helmet" );
		if ( iHelmet >= 0 && pRagdoll->GetBodygroup( iHelmet ) >= 1 )
		{
			pRagdoll->m_iHelmetHealth -= (int)flDamage;
			if ( pRagdoll->m_iHelmetHealth <= 0 )
			{
				pRagdoll->m_iHelmetHealth = 0;
				pRagdoll->SetBodygroup( iHelmet, 0 );

				const char *pszItem = UH_HelmetItemFor( pRagdoll );
				CBaseEntity *pHelmet = CreateEntityByName( pszItem );
				if ( pHelmet )
				{
					pHelmet->SetAbsOrigin( pos );
					pHelmet->SetAbsAngles( vec3_angle );
					DispatchSpawn( pHelmet );
				}
				pRagdoll->EmitSound( "Player.Helmet" );
			}
			return;	// helmet absorbed the hit
		}
	}

	int idx = -1;
	const char *pszLimb = NULL;
	switch ( iHitGroup )
	{
	case HITGROUP_HEAD:		idx = 0; break;
	case HITGROUP_LEFTARM:	idx = 1; pszLimb = "leftarm"; break;
	case HITGROUP_RIGHTARM:	idx = 2; pszLimb = "rightarm"; break;
	case HITGROUP_LEFTLEG:	idx = 3; pszLimb = "leftleg"; break;
	case HITGROUP_RIGHTLEG:	idx = 4; pszLimb = "rightleg"; break;
	}
	if ( idx < 0 )
		return;

	// Same subtractive pool as the living NPC path — the original has no
	// separate corpse rule, a dead body costs exactly as much damage to take
	// apart as a live one.
	pRagdoll->m_iGibHealth[idx] -= (int)flDamage;
	if ( pRagdoll->m_iGibHealth[idx] > 0 )
		return;

	// Visually remove the limb before touching physics. A bodygroup transition
	// is also the persistent one-shot state: once it has already been removed,
	// do not emit another gib/blood burst on every later bullet.
	bool bRemoved = false;
	switch ( iHitGroup )
	{
	case HITGROUP_LEFTARM:	bRemoved = UH_RemoveBodygroupSide( pRagdoll, "arms", 0 ); break;
	case HITGROUP_RIGHTARM:	bRemoved = UH_RemoveBodygroupSide( pRagdoll, "arms", 1 ); break;
	case HITGROUP_LEFTLEG:	bRemoved = UH_RemoveBodygroupSide( pRagdoll, "legs", 0 ); break;
	case HITGROUP_RIGHTLEG:	bRemoved = UH_RemoveBodygroupSide( pRagdoll, "legs", 1 ); break;
	case HITGROUP_HEAD:
		{
			int iGroup = UH_FindBodygroup( pRagdoll, "head" );
			int iDestroyed = UH_DestroyedHeadBodygroup( pRagdoll );
			if ( iGroup >= 0 && pRagdoll->GetBodygroup( iGroup ) != iDestroyed )
			{
				pRagdoll->SetBodygroup( iGroup, iDestroyed );
				bRemoved = true;
			}

			// Respirator / gasmask are part of the head destruction path.
			UH_DropGearItem( pRagdoll, "respirator", "item_respirator_guard", pos, dir );
			UH_DropGearItem( pRagdoll, "gasmask",
				V_stristr( STRING( pRagdoll->GetModelName() ), "prisonguard" ) ? "item_gasmask_prison" : "item_gasmask_guard",
				pos, dir );
		}
		break;
	}
	if ( !bRemoved )
		return;

	// sub_10031BF0 does *not* break a constraint in the source ragdoll.  It
	// changes the source bodygroup and creates a separately simulated limb with
	// sub_101CDCC0.  Breaking the original constraint as well made the hidden
	// limb's physics pull the torso into a second, distorted pose.
	// The generated sub-ragdoll below is the only detached physics object.

	// Spawn a severed-limb gib at the hit position.
	CBaseEntity *pGib = NULL;
	if ( pszLimb )
	{
		const char *pszModel = UH_GibModelFor( pRagdoll, pszLimb );
		if ( pszModel )
		{
			Vector vecGibOrigin;
			QAngle angGibOrigin;
			UH_GetLimbSpawnTransform( pRagdoll, iHitGroup, pos, vecGibOrigin, angGibOrigin );
			pGib = UH_SpawnGibProp( pszModel, vecGibOrigin, angGibOrigin, dir, pRagdoll );
			if ( pGib )
				UH_CopyLimbBodygroups( pRagdoll, pGib->GetBaseAnimating(), iHitGroup );
		}
	}

	pRagdoll->RemoveAllDecals();
	pRagdoll->EmitSound( "Player.Splat" );

	UH_DispatchLimbBlood( pRagdoll, iHitGroup, pGib ? pGib->GetBaseAnimating() : NULL, pos, dir );
	if ( iHitGroup == HITGROUP_HEAD )
	{
		pRagdoll->EmitSound( "Player.HeadShot" );
		UTIL_BloodSpray( pos, dir, BLOOD_COLOR_RED, 8, FX_BLOODSPRAY_ALL );
	}
}
