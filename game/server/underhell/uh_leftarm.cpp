//========= Copyright (c) 2008, Mxthe (Underhell). All rights reserved. ============//
//
// Purpose: Underhell second hand / left arm (viewmodel index 1).
//
// The left arm holds the flashlight (while a one-handed weapon is active), a
// flare (from a flare pack), or a grenade mid-throw. Decoded from the original
// binary:
//   - Throw_Nade (sub_101ED130): staged throw — the grenade viewmodel goes into
//     the left hand, the arm raises, and the actual throw happens 0.4 s later
//     from the "FlashLightContext" think (sub_101EE050).
//   - Flashlight deploy (sub_101F0C60): toggles the hand-held flashlight
//     viewmodel in the left arm (vs the shoulder-mounted flashlight).
//   - Flare throw (sub_101E9580): spawns a lit flare prop_physics with a 90 s
//     fuse and a 200 u/s throw velocity.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hl2_player.h"
#include "baseviewmodel_shared.h"
#include "ammodef.h"
#include "grenade_frag.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static const char *UH_LEFTARM_GRENADE    = "models/weapons/v_grenade.mdl";
static const char *UH_LEFTARM_FLASHLIGHT = "models/weapons/v_flashlight_pg.mdl";
static const char *UH_LEFTARM_FLARE      = "models/weapons/v_flare_pg.mdl";
static const char *UH_FLARE_PROP         = "models/PG_props/pg_obj/pg_flare.mdl";

#define UH_THROW_STAGE_DELAY  0.4f   // command -> grenade actually leaves the hand
#define UH_FLARE_FUSE         90.0f  // seconds a thrown flare burns
#define UH_GRENADE_TIMER      3.0f   // frag detonation fuse (vanilla GRENADE_TIMER)

//-----------------------------------------------------------------------------
// Purpose: set (or clear) the left-arm viewmodel's model, skin and raised state.
// Pass model == NULL to holster the arm entirely (hide the viewmodel).
//-----------------------------------------------------------------------------
void CHL2_Player::UH_SetLeftArmModel( const char *pszModel, int nSkin, bool bDeployed )
{
	CBaseViewModel *vm = GetViewModel( 1 );
	if ( !vm )
		return;

	if ( pszModel && *pszModel )
	{
		vm->SetWeaponModel( pszModel, NULL );
		vm->m_nSkin = nSkin;
		vm->RemoveEffects( EF_NODRAW );
		// Sequence 0 = raised/idle, 1 = holstered (matches the kick viewmodel).
		vm->SendViewModelMatchingSequence( bDeployed ? 0 : 1 );
	}
	else
	{
		// Nothing in the left hand: clear the model and hide the viewmodel.
		vm->SetWeaponModel( NULL, NULL );
		vm->AddEffects( EF_NODRAW );
	}

	m_bLeftArmDeployed = bDeployed;
}

//-----------------------------------------------------------------------------
// Purpose: Decide what the left arm should be holding right now. Priority:
// grenade throw in progress > flare > hand-held flashlight > nothing.
//-----------------------------------------------------------------------------
void CHL2_Player::UH_UpdateLeftArm( void )
{
	// A grenade throw is a transient 0.4 s state; leave it alone.
	if ( m_bFlareMarker )
		return;

	if ( m_bHoldingFlare )
	{
		UH_SetLeftArmModel( UH_LEFTARM_FLARE, 1, true );
		return;
	}

	// Hand-held (non-shoulder) flashlight, raised while on and the left arm is
	// free: no weapon, a one-handed weapon (pistol) or melee. Must match
	// FlashlightTurnOn's gate exactly, otherwise the flashlight can be ON but
	// its left-hand viewmodel never appears.
	CBaseCombatWeapon *pWeapon = GetActiveWeapon();
	bool bLeftArmFree = !pWeapon || pWeapon->GetWpnData().m_bOneHanded || pWeapon->GetWpnData().m_bMeleeWeapon;

	if ( FlashlightIsOn() && UH_IsFlashlightInLeftArm() && bLeftArmFree )
	{
		UH_SetLeftArmModel( UH_LEFTARM_FLASHLIGHT, 1, true );
		return;
	}

	UH_SetLeftArmModel( NULL, 0, false );
}

//-----------------------------------------------------------------------------
// Purpose: Holster the left arm (used on weapon switch / drop / death).
//-----------------------------------------------------------------------------
void CHL2_Player::UH_HolsterLeftArm( void )
{
	m_bLeftArmDeployed = false;
	UH_SetLeftArmModel( NULL, 0, false );
}

//-----------------------------------------------------------------------------
// Purpose: Put a flare in the left hand (flare-pack "useitem").
//-----------------------------------------------------------------------------
void CHL2_Player::UH_EquipFlare( void )
{
	m_bHoldingFlare = true;
	UH_UpdateLeftArm();
}

//-----------------------------------------------------------------------------
// Purpose: Throw a grenade (client command "Throw_Nade"). Decoded from
// sub_101ED130: if holding a flare, throw the flare; otherwise stage the
// grenade throw — the grenade viewmodel goes into the left hand, the arm
// raises, and the grenade actually leaves 0.4 s later.
//-----------------------------------------------------------------------------
void CHL2_Player::UH_ThrowNade( void )
{
	if ( GetHealth() <= 0 )
		return;

	if ( IsSprinting() )
		return;

	// A held flare is thrown instead of a grenade.
	if ( m_bHoldingFlare )
	{
		UH_ThrowFlare();
		return;
	}

	// Underhell grenades are AMMO, not a weapon: picking up a weapon_frag
	// converts it into "grenade" ammo (CHL2_Player::BumpWeapon) and removes the
	// entity, so Weapon_OwnsThisType("weapon_frag") is always NULL here. The
	// original gates the throw on the grenade ammo count instead
	// (sub_101ED130 -> sub_100CF610 reads GetAmmoCount("grenade") > 0).
	int iGrenadeAmmo = GetAmmoDef()->Index( "grenade" );
	if ( iGrenadeAmmo < 0 || GetAmmoCount( iGrenadeAmmo ) <= 0 )
		return;

	// Un-sight (the original does this before throwing).
	if ( m_bIronSighted )
		UH_ToggleIronsight();

	// Stage the throw: grenade in the left hand + raise the arm.
	m_bFlareMarker = true;
	UH_SetLeftArmModel( UH_LEFTARM_GRENADE, 1, true );

	// Throw gesture on the active weapon's viewmodel.
	CBaseCombatWeapon *pWeapon = GetActiveWeapon();
	if ( pWeapon )
		pWeapon->SendWeaponAnim( ACT_VM_THROW );

	SetContextThink( &CHL2_Player::UH_LeftArmContextThink, gpGlobals->curtime + UH_THROW_STAGE_DELAY, "FlashLightContext" );
}

//-----------------------------------------------------------------------------
// Purpose: The delayed grenade throw (original "FlashLightContext" think,
// sub_101EE050). Fires the actual grenade projectile and restores the left arm.
//-----------------------------------------------------------------------------
void CHL2_Player::UH_LeftArmContextThink( void )
{
	m_bFlareMarker = false;

	// Actually throw the grenade: spawn the frag projectile and consume one
	// grenade ammo. This replicates CWeaponFrag::ThrowGrenade + DecrementAmmo
	// directly against the player's "grenade" ammo (the original stages the
	// throw here rather than through a weapon_frag the player never owns).
	int iGrenadeAmmo = GetAmmoDef()->Index( "grenade" );
	if ( iGrenadeAmmo >= 0 && GetAmmoCount( iGrenadeAmmo ) > 0 )
	{
		Vector vecEye = EyePosition();
		Vector vForward, vRight;
		EyeVectors( &vForward, &vRight, NULL );

		Vector vecSrc = vecEye + vForward * 18.0f + vRight * 8.0f;
		vForward[2] += 0.1f;

		Vector vecThrow;
		GetVelocity( &vecThrow, NULL );
		vecThrow += vForward * 1200.0f;

		Fraggrenade_Create( vecSrc, vec3_angle, vecThrow,
							AngularImpulse( 600, random->RandomInt( -1200, 1200 ), 0 ),
							this, UH_GRENADE_TIMER, false );

		RemoveAmmo( 1, iGrenadeAmmo );
	}

	// Restore the left arm (flashlight / flare / nothing).
	UH_UpdateLeftArm();
}

//-----------------------------------------------------------------------------
// Purpose: Throw the held flare (original sub_101E9580). Spawns a lit flare
// prop_physics in front of the player with a 200 u/s throw and a 90 s fuse.
//-----------------------------------------------------------------------------
void CHL2_Player::UH_ThrowFlare( void )
{
	Vector vecForward;
	EyeVectors( &vecForward );

	Vector vecSrc = EyePosition() + vecForward * 16.0f;

	CBaseEntity *pFlare = CreateEntityByName( "prop_physics" );
	if ( !pFlare )
		return;

	pFlare->SetModel( UH_FLARE_PROP );
	pFlare->SetAbsOrigin( vecSrc );
	pFlare->SetAbsAngles( GetAbsAngles() );
	DispatchSpawn( pFlare );

	// Light the flare: glow render mode + a dlight tint so it actually emits
	// light when thrown (matches the original's lit flare prop).
	pFlare->SetRenderColor( 255, 200, 80 );
	pFlare->SetRenderMode( kRenderGlow );
	pFlare->AddEffects( EF_BRIGHTLIGHT | EF_NOSHADOW );

	// Throw velocity (original: view direction * 200).
	IPhysicsObject *pPhys = pFlare->VPhysicsGetObject();
	if ( pPhys )
	{
		Vector vecVel = vecForward * 200.0f + Vector( 0, 0, 40.0f );
		AngularImpulse angImp( 0, 0, 0 );
		pPhys->SetVelocity( &vecVel, &angImp );
	}

	// Burn out after the fuse.
	pFlare->SetThink( &CBaseEntity::SUB_Remove );
	pFlare->SetNextThink( gpGlobals->curtime + UH_FLARE_FUSE );

	m_bHoldingFlare = false;
	UH_UpdateLeftArm();
}
