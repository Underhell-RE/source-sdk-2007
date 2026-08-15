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
#include "grenade_frag.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static const char *UH_LEFTARM_GRENADE    = "models/weapons/v_grenade.mdl";
static const char *UH_LEFTARM_FLASHLIGHT = "models/weapons/v_flashlight_pg.mdl";
static const char *UH_LEFTARM_FLARE      = "models/weapons/v_flare_pg.mdl";
static const char *UH_FLARE_PROP         = "models/PG_props/pg_obj/pg_flare.mdl";

#define UH_THROW_STAGE_DELAY  0.4f   // command -> grenade actually leaves the hand
#define UH_FLARE_FUSE         90.0f  // seconds a thrown flare burns

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

	// Hand-held (non-shoulder) flashlight, raised while on and the active
	// weapon is one-handed (pistol) or melee.
	CBaseCombatWeapon *pWeapon = GetActiveWeapon();
	bool bOneHanded = pWeapon && ( pWeapon->GetWpnData().m_bOneHanded || pWeapon->GetWpnData().m_bMeleeWeapon );

	if ( FlashlightIsOn() && UH_IsFlashlightInLeftArm() && bOneHanded )
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

	CBaseCombatWeapon *pGrenade = Weapon_OwnsThisType( "weapon_frag" );
	if ( !pGrenade )
		return;

	if ( !pGrenade->HasPrimaryAmmo() )
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

	// Actually throw the grenade.
	CBaseCombatWeapon *pGrenade = Weapon_OwnsThisType( "weapon_frag" );
	if ( pGrenade )
		WeaponFrag_ThrowNow( pGrenade );

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

	// Light the flare: self-illuminated model + a dlight tint.
	pFlare->SetRenderColor( 255, 200, 80 );
	pFlare->AddEffects( EF_BRIGHTLIGHT );

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
