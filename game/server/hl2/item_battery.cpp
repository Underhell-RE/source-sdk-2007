//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Handling for the suit batteries.
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

class CItemBattery : public CItem
{
public:
	DECLARE_CLASS( CItemBattery, CItem );

	void Spawn( void )
	{ 
		Precache( );
		SetModel( "models/items/battery.mdl" );
		BaseClass::Spawn( );
	}
	void Precache( void )
	{
		PrecacheModel ("models/items/battery.mdl");

		PrecacheScriptSound( "ItemBattery.Touch" );

	}
		bool MyTouch( CBasePlayer *pPlayer )
		{
			// Underhell: batteries power the flashlight (m_iUHBatteryCount)
			// instead of charging suit armour like vanilla HL2.
			CHL2_Player *pHL2Player = dynamic_cast<CHL2_Player *>( pPlayer );
			if ( !pHL2Player )
				return false;

			pHL2Player->UH_AddBattery( 1 );

			CPASAttenuationFilter filter( pPlayer, "ItemBattery.Touch" );
			EmitSound( filter, pPlayer->entindex(), "ItemBattery.Touch" );

			return true;
		}
};

LINK_ENTITY_TO_CLASS(item_battery, CItemBattery);
PRECACHE_REGISTER(item_battery);

