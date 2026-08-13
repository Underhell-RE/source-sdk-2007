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
// Pickup flow (decoded from the original, sub_101720D0 / sub_10171F30 range):
//   MyTouch -> RTTI-cast to CHL2_Player -> free-slot check -> pickup sound
//   -> SetOwnerEntity -> CHL2_Player::UH_GiveItem (vtable [410]) -> remove.
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
//-----------------------------------------------------------------------------
class CUHItem : public CItem
{
public:
	DECLARE_CLASS( CUHItem, CItem );

	// Item id written into CHL2_Player::m_iInventory when picked up.
	// UH_ITEM_UNKNOWN = not an inventory item (auto-applies on pickup).
	virtual int		GetInventoryItemType() const = 0;

	// Default pickup: give the item to the player's inventory.
	virtual bool	MyTouch( CBasePlayer *pPlayer );
};

// Declares a plain item class (Spawn/Precache + standard pickup).
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
UH_DECLARE_ITEM( CItemOrange,			UH_ITEM_ORANGE )
UH_DECLARE_ITEM( CItemBanana,			UH_ITEM_BANANA )
UH_DECLARE_ITEM( CItemBananaBunch,		UH_ITEM_BANANA_BUNCH )
UH_DECLARE_ITEM( CItemSandwich,			UH_ITEM_SANDWICH )
UH_DECLARE_ITEM( CItemChocobar,			UH_ITEM_CHOCOBAR )
UH_DECLARE_ITEM( CItemBurrito,			UH_ITEM_BURRITO )

// Apples randomize their skin on spawn (red/green) and give the matching
// inventory id — original CItemApple::Spawn (sub_10171E90).
class CItemApple : public CUHItem
{
public:
	DECLARE_CLASS( CItemApple, CUHItem );

	virtual void Spawn( void );
	virtual void Precache( void );
	virtual bool MyTouch( CBasePlayer *pPlayer );
	virtual int	 GetInventoryItemType() const { return UH_ITEM_APPLE_RED; }
};

// Sodas cover six flavours (ids 7..12). TODO: verify the original picks the
// flavour on spawn (skin) or on pickup.
class CItemUHSoda : public CUHItem
{
public:
	DECLARE_CLASS( CItemUHSoda, CUHItem );

	virtual void Spawn( void );
	virtual void Precache( void );
	virtual bool MyTouch( CBasePlayer *pPlayer );
	virtual int	 GetInventoryItemType() const { return UH_ITEM_SODA_FIRST; }
};

// Glowsticks cover five colours (ids 14..18). TODO: verify colour source.
class CItemGlowStick : public CUHItem
{
public:
	DECLARE_CLASS( CItemGlowStick, CUHItem );

	virtual void Spawn( void );
	virtual void Precache( void );
	virtual bool MyTouch( CBasePlayer *pPlayer );
	virtual int	 GetInventoryItemType() const { return UH_ITEM_GLOWSTICK_FIRST; }
};

//-----------------------------------------------------------------------------
// Underhell ammo pickups (boxed). Vanilla rounds keep their vanilla classes.
// TODO: ammo uses a separate id space — pickups auto-add ammo.
//-----------------------------------------------------------------------------
UH_DECLARE_ITEM( CItem_UHPistolAmmo,	UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItem_UH357Ammo,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItem_UHSMG1Ammo,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItem_UHRifleAmmo,		UH_ITEM_UNKNOWN )
UH_DECLARE_ITEM( CItem_UHBuckShot,		UH_ITEM_UNKNOWN )

//-----------------------------------------------------------------------------
// Equipment
//-----------------------------------------------------------------------------
UH_DECLARE_ITEM( CItemBatteryPack,		UH_ITEM_UNKNOWN )	// TODO: battery system
UH_DECLARE_ITEM( CItemFlashlight,		UH_ITEM_UNKNOWN )	// TODO: flashlight system
UH_DECLARE_ITEM( CItemNightVision,		UH_ITEM_UNKNOWN )	// TODO: gear system
UH_DECLARE_ITEM( CItemGasMask,			UH_ITEM_UNKNOWN )	// TODO: gear system
UH_DECLARE_ITEM( CItemHelmetGuard,		UH_ITEM_UNKNOWN )	// TODO: gear system
UH_DECLARE_ITEM( CItemHelmetPrison,		UH_ITEM_UNKNOWN )	// TODO: gear system
UH_DECLARE_ITEM( CItemHelmetPMC,		UH_ITEM_UNKNOWN )	// TODO: gear system
UH_DECLARE_ITEM( CItemHelmetWorker,		UH_ITEM_UNKNOWN )	// TODO: gear system
UH_DECLARE_ITEM( CItemFlarePack,		UH_ITEM_FLARE_PACK )
UH_DECLARE_ITEM( CItemFMRadio,			UH_ITEM_FM_RADIO )
UH_DECLARE_ITEM( CItemRadioCracker,		UH_ITEM_RADIO_CRACKER )

// Armour auto-applies on pickup (original sub_10171F60: only while armour
// is below full). TODO: verify the amount granted.
class CItemArmor : public CUHItem
{
public:
	DECLARE_CLASS( CItemArmor, CUHItem );

	virtual void Spawn( void );
	virtual void Precache( void );
	virtual bool MyTouch( CBasePlayer *pPlayer );
	virtual int	 GetInventoryItemType() const { return UH_ITEM_UNKNOWN; }
};

UH_DECLARE_ITEM( CItemHeavyArmor,		UH_ITEM_UNKNOWN )	// TODO: heavy armour pickup

//-----------------------------------------------------------------------------
// Health
// NOTE: item_healthkit / item_healthvial stay VANILLA (CHealthKit/CHealthVial
// in hl2/item_healthkit.cpp) — the original kept them, and the inventory
// use-flow spawns them by classname.
//-----------------------------------------------------------------------------
class CItemBandages : public CUHItem
{
public:
	DECLARE_CLASS( CItemBandages, CUHItem );

	virtual void Spawn( void );
	virtual void Precache( void );
	virtual bool MyTouch( CBasePlayer *pPlayer );
	virtual int	 GetInventoryItemType() const { return UH_ITEM_BANDAGES; }
};

UH_DECLARE_ITEM( CItemPainkillers,		UH_ITEM_PAINKILLERS )	// TODO: original class name unknown (no RTTI)
UH_DECLARE_ITEM( CItemSyringe,			UH_ITEM_SYRINGE )		// TODO: original class name unknown (no RTTI)

//-----------------------------------------------------------------------------
// Items seen in the original RTTI / dll strings — classnames recovered from
// the original serveror.dll string table, models still TODO.
//-----------------------------------------------------------------------------
UH_DECLARE_ITEM( CItemSoda,				UH_ITEM_UNKNOWN )	// item_sodacan — TODO: id, model
UH_DECLARE_ITEM( CItemShield,			UH_ITEM_UNKNOWN )	// item_shield — TODO: id, model
UH_DECLARE_ITEM( CItemShoulderFlashlight, UH_ITEM_UNKNOWN )	// item_shoulderflashlight — TODO
UH_DECLARE_ITEM( CItemCapPMC,			UH_ITEM_UNKNOWN )	// item_cap_pmc — TODO
UH_DECLARE_ITEM( CItemHeadsetPMC,		UH_ITEM_UNKNOWN )	// item_headset_pmc — TODO
UH_DECLARE_ITEM( CItemRespiratorGuard,	UH_ITEM_UNKNOWN )	// item_respirator_guard — TODO
UH_DECLARE_ITEM( CItemGasmaskGuard,		UH_ITEM_UNKNOWN )	// item_gasmask_guard — TODO

// TODO: item_bandagespack, item_syringepack, item_flags, item_health — class
// names unknown (no RTTI entries found for them).

//-----------------------------------------------------------------------------
// item_random — spawns a random item from the fixed pool (original
// CItemRandom::Spawn, sub_101757D0).
//
// Keyvalues: one per pool entry (item_*/weapon_* names), "nothing" (chance
// of spawning nothing, scaled by sk_itemrandom1/2/3), "respawn" (entity
// survives after spawning), "disableshadows" (EF_NOSHADOW on the spawned
// item). Input: Respawn.
//-----------------------------------------------------------------------------
class CItemRandom : public CUHItem
{
public:
	DECLARE_CLASS( CItemRandom, CUHItem );

	virtual void Spawn( void );
	virtual void Precache( void );
	virtual int	 GetInventoryItemType() const { return UH_ITEM_NONE; }

	void InputRespawn( inputdata_t &inputdata );

private:
	void SpawnRandomItem( void );

	// Enable flags per pool entry. Names + order = the original datamap.
	bool	m_bitem_chocobar, m_bitem_sandwich, m_bitem_apple, m_bitem_orange;
	bool	m_bitem_banana, m_bitem_bananabunch, m_bitem_burrito, m_bitem_soda;
	bool	m_bitem_flarepack, m_bitem_glowstick, m_bitem_painkillers;
	bool	m_bitem_syringe, m_bitem_syringepack, m_bitem_bandages, m_bitem_bandagespack;
	bool	m_bitem_armor, m_bitem_heavyarmor, m_bitem_battery, m_bitem_batterypack;
	bool	m_bitem_healthkit, m_bitem_healthvial, m_bitem_nightvision, m_bitem_flashlight;
	bool	m_bitem_helmet_prison, m_bitem_helmet_guard, m_bitem_helmet_worker;
	bool	m_bitem_fmradio, m_bitem_radiocracker;
	bool	m_bitem_ammo_357, m_bitem_ammo_357_large, m_bitem_ammo_ar2;
	bool	m_bitem_ammo_ar2_altfire, m_bitem_ammo_ar2_large, m_bitem_ammo_crossbow;
	bool	m_bitem_ammo_pistol, m_bitem_ammo_pistol_large, m_bitem_ammo_smg1;
	bool	m_bitem_ammo_smg1_grenade, m_bitem_ammo_smg1_large, m_bitem_box_buckshot;
	bool	m_bitem_box_357_ammo, m_bitem_box_pistol_ammo, m_bitem_box_smg1_ammo;
	bool	m_bitem_box_rifle_ammo, m_bitem_ammo_buckshot, m_bitem_rpg_round;
	bool	m_bweapon_physcannon, m_bweapon_crowbar, m_bweapon_wrench, m_bweapon_pipe;
	bool	m_bweapon_axe, m_bweapon_hammer, m_bweapon_shiv, m_bweapon_pistol;
	bool	m_bweapon_pistol_glock, m_bweapon_pistol_socom, m_bweapon_pistol_beretta;
	bool	m_bweapon_pistol_dualberetta, m_bweapon_pistol_python, m_bweapon_357;
	bool	m_bweapon_smg1, m_bweapon_smg_mp5, m_bweapon_smg_mp5_eod, m_bweapon_smg_mp7;
	bool	m_bweapon_shotgun, m_bweapon_shotgun_spas12, m_bweapon_shotgun_m3;
	bool	m_bweapon_shotgun_m5, m_bweapon_shotgun_xm1014, m_bweapon_ar2;
	bool	m_bweapon_rifle_g36k, m_bweapon_rifle_sniper, m_bweapon_crossbow;
	bool	m_bweapon_mgl, m_bweapon_rpg, m_bweapon_minigun, m_bweapon_frag;

	float	m_iNothingChance;	// 0..100 percent chance of spawning nothing
	bool	m_bRespawn;			// stay alive after spawning (input Respawn re-rolls)
	bool	m_bDisableShadows;	// EF_NOSHADOW on the spawned item
	EHANDLE	m_hOldItem;			// last spawned item (original m_hOldItem)

	DECLARE_DATADESC();
};

#endif // UH_ITEMS_H
