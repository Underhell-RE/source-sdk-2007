//========= Copyright (c) 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Handling for the suit batteries.
//
// Underhell: batteries power the flashlight (CHL2_Player::m_iUHBatteryCount)
// instead of charging suit armour, and are picked up with +use (no touch
// auto-pickup), matching every other Underhell item. Model and count are from
// the original CItemBattery (sub_102EE3F0: +1, max 20).
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hl2_player.h"
#include "basecombatweapon.h"
#include "gamerules.h"
#include "items.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Maximum batteries the player can carry (original sub_102E1EC0 clamps to 20).
#define UH_MAX_BATTERIES 20

class CItemBattery : public CItem
{
public:
	DECLARE_CLASS( CItemBattery, CItem );

	void Spawn( void )
	{
		Precache();
		SetModel( "models/PG_props/pg_obj/pg_battery.mdl" );
		BaseClass::Spawn();

		// Underhell: no touch auto-pickup; taken with +use.
		SetTouch( NULL );
	}
	void Precache( void )
	{
		PrecacheModel( "models/PG_props/pg_obj/pg_battery.mdl" );

		PrecacheScriptSound( "ItemBattery.Touch" );
	}
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
	{
		CBasePlayer *pPlayer = ToBasePlayer( pActivator );
		if ( pPlayer ) MyTouch( pPlayer );
	}
	bool MyTouch( CBasePlayer *pPlayer )
	{
		// Underhell: batteries power the flashlight (m_iUHBatteryCount)
		// instead of charging suit armour like vanilla HL2.
		CHL2_Player *pHL2Player = dynamic_cast<CHL2_Player *>( pPlayer );
		if ( !pHL2Player )
			return false;

		if ( pHL2Player->UH_GetBatteryCount() >= UH_MAX_BATTERIES )
			return false;

		FirePlayerPickupOutput( pHL2Player );
		pHL2Player->UH_AddBattery( 1 );

		CPASAttenuationFilter filter( pPlayer, "ItemBattery.Touch" );
		EmitSound( filter, pPlayer->entindex(), "ItemBattery.Touch" );

		UTIL_Remove( this );

		return true;
	}
};

LINK_ENTITY_TO_CLASS(item_battery, CItemBattery);
PRECACHE_REGISTER(item_battery);
