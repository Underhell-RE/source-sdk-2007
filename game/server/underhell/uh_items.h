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
UH_DECLARE_ITEM( CItemApple,			UH_ITEM_APPLE_RED )
UH_DECLARE_ITEM( CItemOrange,			UH_ITEM_ORANGE )
UH_DECLARE_ITEM( CItemBanana,			UH_ITEM_BANANA )
UH_DECLARE_ITEM( CItemBananaBunch,		UH_ITEM_BANANA_BUNCH )
UH_DECLARE_ITEM( CItemSandwich,			UH_ITEM_SANDWICH )
UH_DECLARE_ITEM( CItemChocobar,			UH_ITEM_CHOCOBAR )
UH_DECLARE_ITEM( CItemBurrito,			UH_ITEM_BURRITO )
UH_DECLARE_ITEM( CItemUHSoda,			UH_ITEM_SODA_FIRST )

//-----------------------------------------------------------------------------
// Ammo — boxed and loose rounds.
//-----------------------------------------------------------------------------
UH_DECLARE_ITEM( CItemAmmoPistol,		UH_ITEM_UNKNOWN )	// TODO: ammo ids — original uses a separate ammo namespace
UH_DECLARE_ITEM( CItemAmmoPistolLarge,	UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemBoxPistolAmmo,	UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemAmmo357,			UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemAmmo357Large,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemBox357Ammo,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemAmmoSMG1,			UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemAmmoSMG1Large,	UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemAmmoSMG1Grenade,	UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemBoxSMG1Ammo,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemAmmoBuckshot,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemBoxBuckshot,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemAmmoAR2,			UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemAmmoAR2Large,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemAmmoAR2AltFire,	UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemBoxRifleAmmo,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemRPGRound,			UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemAmmoCrossbow,		UH_ITEM_UNKNOWN )

//-----------------------------------------------------------------------------
// Equipment
//-----------------------------------------------------------------------------
UH_DECLARE_ITEM( CItemBattery,			UH_ITEM_UNKNOWN )	// TODO: equipment ids
UH_DECLARE_ITEM( CItemBatteryPack,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemArmor,			UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemHeavyArmor,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemFlashlight,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemNightvision,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemGasmask,			UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemHelmetGuard,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemHelmetPrison,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemHelmetPMC,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemHelmetWorker,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemFlarePack,		UH_ITEM_FLARE_PACK )
UH_DECLARE_ITEM( CItemGlowstick,		UH_ITEM_GLOWSTICK_FIRST )
UH_DECLARE_ITEM( CItemFMRadio,			UH_ITEM_FM_RADIO )
UH_DECLARE_ITEM( CItemRadioCracker,		UH_ITEM_RADIO_CRACKER )

//-----------------------------------------------------------------------------
// Health
//-----------------------------------------------------------------------------
UH_DECLARE_ITEM( CItemHealthKit,			UH_ITEM_HEALTHKIT )
UH_DECLARE_ITEM( CItemHealthVial,			UH_ITEM_HEALTH_VIAL )
UH_DECLARE_ITEM( CItemBandages,				UH_ITEM_BANDAGES )
UH_DECLARE_ITEM( CItemPainkillers,			UH_ITEM_PAINKILLERS )
UH_DECLARE_ITEM( CItemSyringe,				UH_ITEM_SYRINGE )

//-----------------------------------------------------------------------------
// item_random — spawns a random item from the fixed pool.
//-----------------------------------------------------------------------------
class CItemRandom : public CUHItem
{
public:
	DECLARE_CLASS( CItemRandom, CUHItem );

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
