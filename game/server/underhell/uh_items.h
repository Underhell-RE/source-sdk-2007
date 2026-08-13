//========= Copyright (c) 2008, Mxthe (Underhell). All rights reserved. ============//
//
// Purpose: Underhell inventory item entities.
//          Class names match the original server.dll exactly (RTTI dump from
//          the original binaries — see docs/UNDERHELL.md).
//
// The original KEPT the vanilla HL2 items (CItemBattery, CHealthKit,
// CHealthVial, CItem_Box*Rounds, CItem_RPG_Round, ...) and added its own
// CItem* classes for the new inventory items. This file mirrors that split:
// vanilla entities stay vanilla, only Underhell's own items live here.
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
// Picking one up adds GetInventoryItemType() to the player's m_iInventory.
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
// Underhell ammo pickups (boxed). Vanilla rounds keep their vanilla classes.
//-----------------------------------------------------------------------------
UH_DECLARE_ITEM( CItem_UHPistolAmmo,	UH_ITEM_UNKNOWN )	// TODO: ammo uses a separate id space
UH_DECLARE_ITEM( CItem_UH357Ammo,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItem_UHSMG1Ammo,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItem_UHRifleAmmo,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItem_UHBuckShot,		UH_ITEM_UNKNOWN )

//-----------------------------------------------------------------------------
// Equipment
//-----------------------------------------------------------------------------
UH_DECLARE_ITEM( CItemBatteryPack,		UH_ITEM_UNKNOWN )	// TODO: equipment ids
UH_DECLARE_ITEM( CItemArmor,			UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemHeavyArmor,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemFlashlight,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemNightVision,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemGasMask,			UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemHelmetGuard,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemHelmetPrison,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemHelmetPMC,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemHelmetWorker,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItemFlarePack,		UH_ITEM_FLARE_PACK )
UH_DECLARE_ITEM( CItemGlowStick,		UH_ITEM_GLOWSTICK_FIRST )
UH_DECLARE_ITEM( CItemFMRadio,			UH_ITEM_FM_RADIO )
UH_DECLARE_ITEM( CItemRadioCracker,		UH_ITEM_RADIO_CRACKER )

//-----------------------------------------------------------------------------
// Health
// NOTE: item_healthkit / item_healthvial stay VANILLA (CHealthKit/CHealthVial
// in hl2/item_healthkit.cpp) — the original kept them, and the inventory
// use-flow spawns them by classname.
//-----------------------------------------------------------------------------
UH_DECLARE_ITEM( CItemBandages,			UH_ITEM_BANDAGES )
UH_DECLARE_ITEM( CItemPainkillers,		UH_ITEM_PAINKILLERS )	// TODO: original class name unknown (no RTTI)
UH_DECLARE_ITEM( CItemSyringe,			UH_ITEM_SYRINGE )		// TODO: original class name unknown (no RTTI)

//-----------------------------------------------------------------------------
// Items seen in the original RTTI but not yet mapped to a classname — stubs
// registered once their entity names are recovered.
//-----------------------------------------------------------------------------
// UH_DECLARE_ITEM( CItemSuit,				UH_ITEM_UNKNOWN )	// TODO: classname?
// UH_DECLARE_ITEM( CItemShield,			UH_ITEM_UNKNOWN )	// TODO: classname?
// UH_DECLARE_ITEM( CItemShoulderFlashlight,	UH_ITEM_UNKNOWN )	// TODO: classname?
// UH_DECLARE_ITEM( CItemCapPMC,			UH_ITEM_UNKNOWN )	// TODO: classname?
// UH_DECLARE_ITEM( CItemHeadsetPMC,		UH_ITEM_UNKNOWN )	// TODO: classname?
// UH_DECLARE_ITEM( CItemRespiratorGuard,	UH_ITEM_UNKNOWN )	// TODO: classname?
// UH_DECLARE_ITEM( CItemGasmaskGuard,		UH_ITEM_UNKNOWN )	// TODO: classname?

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
