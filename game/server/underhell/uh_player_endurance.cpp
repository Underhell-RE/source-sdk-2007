//========= Copyright (c) 2008, Mxthe (Underhell). All rights reserved. ============//
//
// Purpose: Underhell endurance ("hunger") system — server-side CHL2_Player.
//
// Reconstructed 1:1 from the original server.dll:
//   * UH_Eat  / sub_102DF1A0 — eating food restores endurance + a little health
//   * UH_Drink / sub_102DF2E0 — drinking restores endurance + a little health
//                              (Mega Soda flavour multiplies endurance by 2.5)
//   * UH_UpdateEndurance / sub_102E0E60 (recharge branch) — the suit-power
//     (stamina) recharge rate is scaled by endurance and consumes endurance.
//
// Model (matches the original HUD's two bars):
//   * RED bar   = suit power (m_HL2Local.m_flSuitPower) — the sprint "stamina",
//                 drained by sprinting / melee and recharged over time.
//   * GREEN bar = m_iEndurance — the "hunger" meter (0..100), restored by
//                 eating/drinking, consumed as stamina recharges. The lower it
//                 is, the slower stamina recharges.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hl2_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Tuning (registered for parity with the original cvars; defaults match the
// original binary's ConVar registrations sub_1045C800..sub_1045C8C0).
//-----------------------------------------------------------------------------
static ConVar uh_player_endurance( "uh_player_endurance", "100", FCVAR_ARCHIVE,
	"Maximum (and starting) player endurance. Endurance gates stamina recharge." );
static ConVar uh_player_endurance_rate( "uh_player_endurance_rate", "1600", FCVAR_ARCHIVE,
	"Endurance drain rate 1 (unused by the decoded path, kept for parity)." );
static ConVar uh_player_endurance_rate2( "uh_player_endurance_rate2", "8000", FCVAR_ARCHIVE,
	"Endurance drain rate 2 (unused by the decoded path, kept for parity)." );
static ConVar uh_player_endurance_stamina_effect( "uh_player_endurance_stamina_effect", "100", FCVAR_ARCHIVE,
	"Divisor scaling how strongly endurance affects stamina recharge rate." );
static ConVar uh_player_bleed_rate( "uh_player_bleed_rate", "8000", FCVAR_ARCHIVE,
	"Bleeding damage interval (ms). TODO: full bleeding system." );
static ConVar uh_bleeding_chance( "uh_bleeding_chance", "5", FCVAR_ARCHIVE,
	"Chance (percent) per hit that the player starts bleeding. TODO." );

// How long one flashlight battery lasts (seconds of use).
ConVar uh_flashlight_battery_time( "uh_flashlight_battery_time", "60", FCVAR_ARCHIVE,
	"Seconds of flashlight use per battery." );

// Vanilla suit-power recharge rate (see SUITPOWER_CHARGE_RATE in hl2_player.cpp).
#define UH_SUITPOWER_CHARGE_RATE 12.5f

// Decoded constants from sub_102E0E60 (kept verbatim for 1:1 behaviour).
#define UH_ENDURANCE_RECHARGE_MIN		25.0f	// recharge is clamped to at least this endurance
#define UH_ENDURANCE_STAMINA_DRAIN_AT	50.0f	// charge points per 1 endurance consumed

// Health can be overhealed up to this value by food (original sub_102DF1A0
// clamps m_iHealth to 200).
#define UH_MAX_HEALTH 200

//-----------------------------------------------------------------------------
// Purpose: Reset endurance state (called from the player constructor).
// Original: m_iEndurance is initialised to the convar value (100).
//-----------------------------------------------------------------------------
void CHL2_Player::UH_InitializeEndurance( void )
{
	m_iEndurance = uh_player_endurance.GetInt();
	m_iBleedCounter = 0;
	m_flPseudoEndurance = 0.0f;
	m_fEStaminaCount = 0.0f;
	m_flLastBleedTime = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Eat food — restore endurance and a little health, then play the
// eat sound. Original sub_102DF1A0: adds flEndurance to m_iEndurance
// (clamped to 100) and flHealth to m_iHealth (clamped to 200).
//-----------------------------------------------------------------------------
bool CHL2_Player::UH_Eat( float flEndurance, float flHealth, const char *pszEatSound )
{
	// Original gates on an "able" flag (player offset 2329); the item Use()
	// handlers already gate on alive + gas mask before calling in. A plain
	// alive check is kept here as a cheap safety net.
	if ( !IsAlive() )
		return false;

	int iEndurance = m_iEndurance + (int)flEndurance;
	m_iEndurance = clamp( iEndurance, 0, uh_player_endurance.GetInt() );

	int iHealth = GetHealth() + (int)flHealth;
	SetHealth( clamp( iHealth, 0, UH_MAX_HEALTH ) );

	if ( pszEatSound )
	{
		EmitSound( pszEatSound );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Drink — restore endurance and a little health (the "Mega Soda"
// flavour multiplies the endurance gain by 2.5). Original sub_102DF2E0.
//-----------------------------------------------------------------------------
bool CHL2_Player::UH_Drink( float flEndurance, float flHealth, int iFlavor )
{
	if ( !IsAlive() )
		return false;

	// Original: flavour 5 (Mega Soda, item id 12) packs a stronger punch.
	if ( iFlavor == 5 )
	{
		flEndurance *= 2.5f;
	}

	int iEndurance = m_iEndurance + (int)flEndurance;
	m_iEndurance = clamp( iEndurance, 0, uh_player_endurance.GetInt() );

	int iHealth = GetHealth() + (int)flHealth;
	SetHealth( clamp( iHealth, 0, UH_MAX_HEALTH ) );

	EmitSound( "Player.Drink" );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Endurance-gated suit-power (stamina) recharge. Called from
// SuitPower_Update() whenever the suit would normally recharge.
//
// Original sub_102E0E60 (recharge branch):
//   rate = max( endurance, 25 ) * 0.01 * 12.5 * frametime
//   charge( rate )
//   m_fEStaminaCount += rate
//   if ( m_fEStaminaCount >= 50 ) { m_fEStaminaCount = 0; --endurance; clamp 0 }
//
// So a higher endurance recharges stamina faster, and recharging slowly drains
// endurance (one point per 50 suit-power points recovered).
//-----------------------------------------------------------------------------
void CHL2_Player::UH_UpdateEndurance( void )
{
	float flEndurance = (float)m_iEndurance;
	if ( flEndurance < UH_ENDURANCE_RECHARGE_MIN )
	{
		flEndurance = UH_ENDURANCE_RECHARGE_MIN;
	}

	float flDivisor = uh_player_endurance_stamina_effect.GetFloat();
	if ( flDivisor <= 0.0f )
	{
		flDivisor = 100.0f;
	}

	float flCharge = flEndurance * ( 1.0f / flDivisor ) * UH_SUITPOWER_CHARGE_RATE * gpGlobals->frametime;

	SuitPower_Charge( flCharge );

	m_fEStaminaCount += flCharge;
	if ( m_fEStaminaCount >= UH_ENDURANCE_STAMINA_DRAIN_AT )
	{
		m_fEStaminaCount = 0.0f;

		int iEndurance = m_iEndurance - 1;
		m_iEndurance = clamp( iEndurance, 0, uh_player_endurance.GetInt() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Drain the flashlight's battery while it is on. One battery lasts
// uh_flashlight_battery_time seconds; when the last battery is spent the
// flashlight switches itself off.
//
// The original flashlight is a full viewmodel system (shoulder-mounted,
// holster animation, "FlashlightViewModelThink"); this reproduces the core
// battery mechanic on the vanilla EF_DIMLIGHT flashlight. TODO: port the
// viewmodel flashlight entity.
//-----------------------------------------------------------------------------
void CHL2_Player::UH_UpdateFlashlightBattery( void )
{
	if ( !FlashlightIsOn() )
		return;

	if ( gpGlobals->curtime < m_flNextFlashlightBatteryTime )
		return;

	if ( m_iUHBatteryCount > 0 )
	{
		m_iUHBatteryCount = m_iUHBatteryCount - 1;
	}

	if ( m_iUHBatteryCount <= 0 )
	{
		// Out of batteries: the light goes out.
		m_iUHBatteryCount = 0;
		FlashlightTurnOff();
		return;
	}

	m_flNextFlashlightBatteryTime = gpGlobals->curtime + uh_flashlight_battery_time.GetFloat();
}
