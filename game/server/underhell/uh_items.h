//========= Copyright (c) 2008, Mxthe (Underhell). All rights reserved. ============//
//
// Purpose: Underhell inventory item entities.
//          One C++ class per entity classname, mirroring the original
//          factory dictionary. Functionality is stubbed where the original
//          behaviour still needs to be reconstructed — see TODOs.
//
// $NoKeywords: $
//=============================================================================//

#ifndef UH_ITEMS_H
#define UH_ITEMS_H
#ifdef _WIN32
#pragma once
#endif

#include "items.h"
#include "underhell/uh_inventory.h"

//-----------------------------------------------------------------------------
// Base class for all Underhell inventory item entities.
// Picking one up adds UH_GetInventoryItemType() to the player's m_iInventory.
//-----------------------------------------------------------------------------
class CUHItem : public CItem
{
public:
	DECLARE_CLASS( CUHItem, CItem );

	// Item id written into CHL2_Player::m_iInventory when picked up.
	virtual int		GetInventoryItemType() const = 0;

	// Called when the player touches the item. Adds it to the inventory.
	virtual bool	MyTouch( CBasePlayer *pPlayer );
};

// Declares an item class with no unique behaviour.
#define UH_DECLARE_ITEM( _className, _itemId )					\
	class _className : public CUHItem								\
	{																\
	public:															\
		DECLARE_CLASS( _className, CUHItem );						\
		virtual void Spawn( void );									\
		virtual void Precache( void );								\
		virtual int	 GetInventoryItemType() const { return _itemId; } \
	};

//-----------------------------------------------------------------------------
// Food
//-----------------------------------------------------------------------------
UH_DECLARE_ITEM( CUHItemApple,			UH_ITEM_APPLE_RED )
UH_DECLARE_ITEM( CUHItemOrange,			UH_ITEM_ORANGE )
UH_DECLARE_ITEM( CUHItemBanana,			UH_ITEM_BANANA )
UH_DECLARE_ITEM( CUHItemBananaBunch,		UH_ITEM_BANANA_BUNCH )
UH_DECLARE_ITEM( CUHItemSandwich,			UH_ITEM_SANDWICH )
UH_DECLARE_ITEM( CUHItemChocobar,			UH_ITEM_CHOCOBAR )
UH_DECLARE_ITEM( CUHItemBurrito,			UH_ITEM_BURRITO )
UH_DECLARE_ITEM( CUHItemUHSoda,			UH_ITEM_SODA_FIRST )

//-----------------------------------------------------------------------------
// Ammo — boxed and loose rounds.
//-----------------------------------------------------------------------------
UH_DECLARE_ITEM( CUHItemAmmoPistol,		UH_ITEM_UNKNOWN )	// TODO: ammo ids — original uses a separate ammo namespace
UH_DECLARE_ITEM( CUHItemAmmoPistolLarge,	UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CUHItemBoxPistolAmmo,	UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CUHItemAmmo357,			UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CUHItemAmmo357Large,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CUHItemBox357Ammo,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CUHItemAmmoSMG1,			UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CUHItemAmmoSMG1Large,	UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CUHItemAmmoSMG1Grenade,	UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CUHItemBoxSMG1Ammo,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CUHItemAmmoBuckshot,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CUHItemBoxBuckshot,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CUHItemAmmoAR2,			UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CUHItemAmmoAR2Large,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CUHItemAmmoAR2AltFire,	UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CUHItemBoxRifleAmmo,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CUHItemRPGRound,			UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CUHItemAmmoCrossbow,		UH_ITEM_UNKNOWN )

//-----------------------------------------------------------------------------
// Equipment
//-----------------------------------------------------------------------------
UH_DECLARE_ITEM( CUHItemBattery,			UH_ITEM_UNKNOWN )	// TODO: equipment ids
UH_DECLARE_ITEM( CUHItemBatteryPack,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CUHItemArmor,			UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CUHItemHeavyArmor,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CUHItemFlashlight,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CUHItemNightvision,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CUHItemGasmask,			UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CUHItemHelmetGuard,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CUHItemHelmetPrison,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CUHItemHelmetPMC,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CUHItemHelmetWorker,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CUHItemFlarePack,		UH_ITEM_FLARE_PACK )
UH_DECLARE_ITEM( CUHItemGlowstick,		UH_ITEM_GLOWSTICK_FIRST )
UH_DECLARE_ITEM( CUHItemFMRadio,			UH_ITEM_FM_RADIO )
UH_DECLARE_ITEM( CUHItemRadioCracker,		UH_ITEM_RADIO_CRACKER )

//-----------------------------------------------------------------------------
// Health
//-----------------------------------------------------------------------------
UH_DECLARE_ITEM( CUHItemHealthKit,			UH_ITEM_HEALTHKIT )
UH_DECLARE_ITEM( CUHItemHealthVial,			UH_ITEM_HEALTH_VIAL )
UH_DECLARE_ITEM( CUHItemBandages,				UH_ITEM_BANDAGES )
UH_DECLARE_ITEM( CUHItemPainkillers,			UH_ITEM_PAINKILLERS )
UH_DECLARE_ITEM( CUHItemSyringe,				UH_ITEM_SYRINGE )

//-----------------------------------------------------------------------------
// item_random — spawns a random item from the fixed pool.
//-----------------------------------------------------------------------------
class CUHItemRandom : public CUHItem
{
public:
	DECLARE_CLASS( CUHItemRandom, CUHItem );

	virtual void Spawn( void );
	virtual void Precache( void );
	virtual int	 GetInventoryItemType() const { return UH_ITEM_NONE; }

	// TODO: original behaviour — pick a random id from the pool, honour the
	// "nothing" chance keyvalue, spawn the matching entity, remove self.
	// Inputs: Respawn.
	// Keyvalues: disableshadows, respawn, nothing.

private:
	bool	m_bDisableShadows;
	bool	m_bCanRespawn;
	int		m_iNothingChance;	// 1..100 percent chance of spawning nothing

	DECLARE_DATADESC();
};

#endif // UH_ITEMS_H
