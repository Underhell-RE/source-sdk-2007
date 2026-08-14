//========= Copyright (c) 2008, Mxthe (Underhell). All rights reserved. ============//
//
// Purpose: Underhell ironsight — server-side CHL2_Player.
//
// "ironsight_toggle" is a client command (not a ConCommand): the engine
// forwards it into CHL2_Player::ClientCommand, which toggles the ironsight.
// Decoded from sub_101ECF40 (toggle) + sub_101F11D0 (ClientCommand dispatch).
//
// State, all networked (mirrors the original binary):
//   - CBaseViewModel::m_bExpSighted  — viewmodel slide target (client reads it
//     in CalcViewModelView and interpolates m_expFactor). Server sendtable
//     offset 1120 / client recv offset 1960 (hexrays sub_100F8EA0/10015160).
//   - CHL2_Player::m_bIronSighted     — authoritative flag (accuracy + HUD).
//   - CHL2_Player::m_fIronsightedTime — last toggle time (debounce).
//
// The FOV zoom is a mild multiplier (uh_ironsight_zoom, default 0.9 — the
// original does FOV = GetDefaultFOV() * uh_ironsight_zoom). Despite its name,
// uh_ironsight_zoom_focus is NOT subtracted from the FOV: the original writes
// it to m_flMaxspeed (hexrays sub_100EA7B0 → player offset 4132, the networked
// m_flMaxspeed), so it is the aim-walk speed while sighted.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hl2_player.h"
#include "baseviewmodel_shared.h"
#include "uh_weapons.h"
#include "grenade_frag.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// The sighted FOV is GetDefaultFOV() * uh_ironsight_zoom (original convar
// "uh_ironsight_zoom", default 0.9 — a mild zoom; the real sighting effect is
// the viewmodel sliding to the eye). Not FCVAR_ARCHIVE in the original.
static ConVar uh_ironsight_zoom( "uh_ironsight_zoom", "0.9", 0,
	"FOV multiplier when ironsighted (sighted FOV = defaultFOV * this)." );

// Despite the name, this is the aim-walk speed while ironsighted, NOT a FOV
// value: sub_101ECF40 writes it to m_flMaxspeed on enter and restores
// hl2_walkspeed on leave. (The original help text claims it is subtracted from
// the default FOV — that is stale; the code uses it as a move speed.)
static ConVar uh_ironsight_zoom_focus( "uh_ironsight_zoom_focus", "40", FCVAR_ARCHIVE,
	"Aim-walk speed while ironsighted (original writes this to m_flMaxspeed)." );

// FOV transition duration in seconds (original passes a data-section constant,
// flt_1063C554, that Hex-Rays could not resolve — reconstructed to match the
// ~0.1 s viewmodel slide, VDC "Adding Ironsights" gMoveTime).
#define UH_IRONSIGHT_FOV_TIME 0.1f

// Minimum time between ironsight toggles (original sub_101ECF40 debounce).
#define UH_IRONSIGHT_TOGGLE_DELAY 0.1f

extern ConVar hl2_walkspeed;

//-----------------------------------------------------------------------------
// Purpose: Toggle ironsight (called from the "ironsight_toggle" client
// command). Mirrors sub_101ECF40: debounce, drop sprint, toggle the networked
// flags, zoom the FOV, slow to aim-walk speed, and play the enter/leave sound.
//
// The FOV is set directly (m_iFOV + m_flFOVTime + m_iFOVStart + m_flFOVRate),
// NOT through CBasePlayer::SetFOV. SetFOV claims m_hZoomOwner (which the
// original's custom setter sub_100F8040 also does, passing the player itself);
// going through it makes IsZooming() return true while only ironsighted and
// collides with the vanilla suit zoom (CheckSuitZoom / Weapon_Switch call
// StopZooming on IN_ZOOM). A direct set produces the same GetFOV() result
// without touching m_hZoomOwner — intentional, documented divergence.
//-----------------------------------------------------------------------------
void CHL2_Player::UH_ToggleIronsight( void )
{
	// Debounce.
	if ( gpGlobals->curtime - m_fIronsightedTime < UH_IRONSIGHT_TOGGLE_DELAY )
		return;

	CBaseCombatWeapon *pWeapon = GetActiveWeapon();
	if ( !pWeapon )
		return;

	// Preconditions (sub_101ECF40):
	//  - not a melee weapon (weapon script "MeleeWeapon" flag → info.m_bMeleeWeapon,
	//    hexrays sub_100D0E00 reads weapon-info offset 1832). No escape clause:
	//    a melee weapon can never sight.
	//  - the original also gates on a weapon flag @1144 (likely "weapon lowered")
	//    and m_bHardLowered, each with an "|| m_bIronSighted" escape so you can
	//    always UN-sight while lowered. TODO: both are not ported yet (no-ops).
	if ( pWeapon->GetWpnData().m_bMeleeWeapon )
		return;

	// Can't sight while sprinting — drop sprint first.
	if ( IsSprinting() )
	{
		StopSprinting();
	}

	CBaseViewModel *pVm = GetViewModel();
	if ( !pVm )
		return;

	// Hide the crosshair while sighted (original toggles bit 256 =
	// HIDEHUD_CROSSHAIR regardless of direction).
	m_Local.m_iHideHUD ^= HIDEHUD_CROSSHAIR;

	// Toggle the viewmodel's networked flag (original @1120). It is the source
	// the client reads to slide the gun up to the eye.
	bool bSighted = !pVm->IsExpSighted();
	pVm->SetExpSighted( bSighted );

	if ( bSighted )
	{
		// Entering ironsight.
		ClearUseEntity();
		EmitSound( "HL2Player.Ironsighton" );

		// Sighted FOV = defaultFOV * uh_ironsight_zoom (original multiplies).
		int iFOV = (int)( (float)GetDefaultFOV() * uh_ironsight_zoom.GetFloat() );

		m_iFOVStart = GetFOV();
		m_flFOVTime = gpGlobals->curtime;
		m_iFOV = iFOV;
		m_Local.m_flFOVRate = UH_IRONSIGHT_FOV_TIME;

		m_bIronSighted = true;

		// Aim-walk slowdown (original writes uh_ironsight_zoom_focus to
		// m_flMaxspeed if it is lower than the current speed).
		if ( uh_ironsight_zoom_focus.GetFloat() < MaxSpeed() )
			SetMaxSpeed( uh_ironsight_zoom_focus.GetFloat() );
	}
	else
	{
		// Leaving ironsight.
		EmitSound( "HL2Player.Ironsightoff" );

		m_bIronSighted = false;

		// FOV 0 = "use default" in GetFOV().
		m_iFOVStart = GetFOV();
		m_flFOVTime = gpGlobals->curtime;
		m_iFOV = 0;
		m_Local.m_flFOVRate = UH_IRONSIGHT_FOV_TIME;

		// Restore walk speed (original restores hl2_walkspeed if it is higher).
		if ( hl2_walkspeed.GetFloat() > MaxSpeed() )
			SetMaxSpeed( hl2_walkspeed.GetFloat() );
	}

	m_fIronsightedTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: Force ironsight off without the debounce/sound (used when the
// weapon changes or is dropped so the FOV can't stay zoomed). Restores every
// piece of state UH_ToggleIronsight changed.
//-----------------------------------------------------------------------------
void CHL2_Player::UH_DisableIronsight( void )
{
	if ( !m_bIronSighted )
		return;

	m_bIronSighted = false;

	CBaseViewModel *pVm = GetViewModel();
	if ( pVm )
		pVm->SetExpSighted( false );

	// Restore the crosshair (clear the bit the toggle set).
	m_Local.m_iHideHUD &= ~HIDEHUD_CROSSHAIR;

	m_iFOVStart = GetFOV();
	m_flFOVTime = gpGlobals->curtime;
	m_iFOV = 0;
	m_Local.m_flFOVRate = UH_IRONSIGHT_FOV_TIME;

	if ( hl2_walkspeed.GetFloat() > MaxSpeed() )
		SetMaxSpeed( hl2_walkspeed.GetFloat() );

	m_fIronsightedTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: Toggle the active weapon's silencer (called from the
// "silencer_toggle" client command). Decoded from sub_101E2F50:
//   - pistols (GetWeaponType() == 1) require m_bHavePistolSilencer,
//   - rifles (GetWeaponType() == 4) require m_bHaveRifleSilencer,
//   - everything else (SMG / shotgun / BFG) toggles freely.
// TODO: the original also skips when the weapon is lowered/holstered
// (weapon flags @1144/@1145).
//-----------------------------------------------------------------------------
void CHL2_Player::UH_ToggleSilencer( void )
{
	CUHGunWeapon *pWeapon = dynamic_cast<CUHGunWeapon *>( GetActiveWeapon() );
	if ( !pWeapon )
		return;

	int iType = pWeapon->GetWeaponType();
	if ( ( iType == 1 && !m_bHavePistolSilencer ) ||
		 ( iType == 4 && !m_bHaveRifleSilencer ) )
	{
		return;
	}

	bool bSilenced = !pWeapon->IsSilenced();
	pWeapon->SetSilenced( bSilenced );

	// Play the attach / detach animation (ACT_VM_ATTACH_SILENCER /
	// ACT_VM_DETACH_SILENCER). SendWeaponAnim falls back to idle if the
	// viewmodel lacks the sequence.
	pWeapon->SendWeaponAnim( bSilenced ? ACT_VM_ATTACH_SILENCER : ACT_VM_DETACH_SILENCER );
}

//-----------------------------------------------------------------------------
// Purpose: Toggle the laser sight. Toggles m_bLaserToggleState (networked);
// the client draws a beam + impact dot (see uh_laser.cpp) while it is set.
//-----------------------------------------------------------------------------
void CHL2_Player::UH_ToggleLaser( void )
{
	m_bLaserToggleState = !m_bLaserToggleState;
}

//-----------------------------------------------------------------------------
// Purpose: Give / take the pistol or rifle silencer (entity inputs
// "SetPistolSilencer" / "SetRifleSilencer"). The original reads a FIELD_BOOLEAN
// parameter (sub_101E36F0 / sub_101E3720), so "1" grants the silencer and "0"
// removes it — see the Entity List tutorial ("setpistolsilencer 1" / "... 0").
//-----------------------------------------------------------------------------
void CHL2_Player::InputSetPistolSilencer( inputdata_t &inputdata )
{
	m_bHavePistolSilencer = inputdata.value.Bool();
}

void CHL2_Player::InputSetRifleSilencer( inputdata_t &inputdata )
{
	m_bHaveRifleSilencer = inputdata.value.Bool();
}

//-----------------------------------------------------------------------------
// Purpose: Throw the active weapon forward (client command "DropWeapon").
// Decoded from sub_101F11D0: un-sight, refuse while in a vehicle / sprinting /
// holding a melee weapon, then Weapon_Drop with a forward velocity of 300.
//-----------------------------------------------------------------------------
void CHL2_Player::UH_DropWeapon( void )
{
	if ( m_bDisableWeaponDrop )
		return;

	CBaseCombatWeapon *pWeapon = GetActiveWeapon();
	if ( !pWeapon )
		return;

	// Come out of ironsight so the FOV / viewmodel don't stay zoomed.
	if ( m_bIronSighted )
		UH_ToggleIronsight();

	// Can't drop while in a vehicle.
	if ( GetVehicle() )
		return;

	// The original refuses to drop melee weapons (weapon-info MeleeWeapon flag).
	if ( pWeapon->GetWpnData().m_bMeleeWeapon )
		return;

	// Can't drop while sprinting.
	if ( IsSprinting() )
		return;

	// Toss it forward (original velocity = forward * 300).
	Vector forward;
	AngleVectors( EyeAngles(), &forward );
	Vector velocity = forward * 300.0f;

	Weapon_Drop( pWeapon, NULL, &velocity );
}

//-----------------------------------------------------------------------------
// Purpose: Throw a grenade (client command "Throw_Nade"). Decoded from
// sub_101ED130. The original is part of the arm-deploy system (m_bLeftArmDeployed
// / m_bHoldingFlare) and throws via the grenade weapon's fire path; the flare
// branch + grenade-viewmodel animation are TODO.
//-----------------------------------------------------------------------------
void CHL2_Player::UH_ThrowNade( void )
{
	if ( GetHealth() <= 0 )
		return;

	if ( IsSprinting() )
		return;

	CBaseCombatWeapon *pGrenade = Weapon_OwnsThisType( "weapon_frag" );
	if ( !pGrenade )
		return;

	if ( !pGrenade->HasPrimaryAmmo() )
		return;

	// Un-sight (the original does this before throwing).
	if ( m_bIronSighted )
		UH_ToggleIronsight();

	// Throw immediately (the original switches to a grenade-armed left arm and
	// throws via the fire path; here we throw straight away).
	WeaponFrag_ThrowNow( pGrenade );
}

//-----------------------------------------------------------------------------
// Purpose: Block / allow the "DropWeapon" command (map inputs, original
// "DisableDropWeapon" / "EnableDropWeapon" — datamap sub_101F2D30).
//-----------------------------------------------------------------------------
void CHL2_Player::InputDisableDropWeapon( inputdata_t &inputdata )
{
	m_bDisableWeaponDrop = true;
}

void CHL2_Player::InputEnableDropWeapon( inputdata_t &inputdata )
{
	m_bDisableWeaponDrop = false;
}
