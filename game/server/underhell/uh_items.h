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
	DECLARE_DATADESC();

	// Item id written into CHL2_Player::m_iInventory when picked up.
	// UH_ITEM_UNKNOWN = not an inventory item (auto-applies on pickup).
	virtual int		GetInventoryItemType() const = 0;

	// Underhell items are NOT auto-picked on touch: Spawn clears the vanilla
	// CItem::ItemTouch handler so walking over an item does nothing. Items are
	// taken with +use (Use -> MyTouch).
	virtual void	Spawn( void );

	// +use: pick the item up into the player's inventory.
	// Consumable items (food/health) only apply their effect here when called
	// with USE_ON (the inventory "useitem" path); any other use type is a
	// world "+use" pickup.
	virtual void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	// Default pickup: give the item to the player's inventory.
	virtual bool	MyTouch( CBasePlayer *pPlayer );
};

//-----------------------------------------------------------------------------
// Base class for edible items. Using one from the inventory restores the
// player's endurance (the "hunger" meter) and a little health, then removes
// the item entity (original per-class Use() handlers sub_10171E10 etc.).
//-----------------------------------------------------------------------------
class CUHFoodItem : public CUHItem
{
public:
	DECLARE_CLASS( CUHFoodItem, CUHItem );

	virtual void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	// Per-item eat parameters (endurance gain, health gain, eat sound).
	virtual float	GetEnduranceGain() const = 0;
	virtual float	GetHealthGain() const = 0;
	virtual const char *GetEatSound() const = 0;
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
//
// Eat parameters come from the original per-item Use() handlers
// (sub_10171E10 / sub_10172170 / sub_10172370 / sub_101751C0 /
//  sub_10177130) and the item descriptions in Underhell_english.txt:
//   Apple/Banana/Burrito/Chocobar/Orange = small  (5 endurance, 1 health)
//   Sandwich                              = medium (10 endurance, 1 health)
//   Banana Bunch                          = large  (15 endurance, 3 health)
//-----------------------------------------------------------------------------
#define UH_DECLARE_FOOD_ITEM( _className, _itemId, _endurance, _health, _sound ) \
	class _className : public CUHFoodItem										\
	{																			\
	public:																		\
		DECLARE_CLASS( _className, CUHFoodItem );								\
		virtual void Spawn( void );												\
		virtual void Precache( void );											\
		virtual int	 GetInventoryItemType() const { return _itemId; }			\
		virtual float GetEnduranceGain() const { return _endurance; }			\
		virtual float GetHealthGain() const { return _health; }					\
		virtual const char *GetEatSound() const { return _sound; }				\
	};

UH_DECLARE_FOOD_ITEM( CItemOrange,		UH_ITEM_ORANGE,		5.0f, 1.0f, "Player.Eat" )
UH_DECLARE_FOOD_ITEM( CItemBanana,		UH_ITEM_BANANA,		5.0f, 1.0f, "Player.Eat" )
UH_DECLARE_FOOD_ITEM( CItemBananaBunch,	UH_ITEM_BANANA_BUNCH, 15.0f, 3.0f, "Player.Eat" )
UH_DECLARE_FOOD_ITEM( CItemSandwich,	UH_ITEM_SANDWICH,	10.0f, 1.0f, "Player.Eat" )
UH_DECLARE_FOOD_ITEM( CItemChocobar,	UH_ITEM_CHOCOBAR,	5.0f, 1.0f, "Player.Eat" )
UH_DECLARE_FOOD_ITEM( CItemBurrito,		UH_ITEM_BURRITO,	5.0f, 1.0f, "Player.Eat" )

// Apples randomize their skin on spawn (red/green) and give the matching
// inventory id — original CItemApple::Spawn (sub_10171E90).
class CItemApple : public CUHFoodItem
{
public:
	DECLARE_CLASS( CItemApple, CUHFoodItem );

	virtual void Spawn( void );
	virtual void Precache( void );
	virtual bool MyTouch( CBasePlayer *pPlayer );
	virtual int	 GetInventoryItemType() const { return UH_ITEM_APPLE_RED; }

	// Eat parameters (original sub_10171E10: 5 endurance, 1 health).
	virtual float GetEnduranceGain() const { return 5.0f; }
	virtual float GetHealthGain() const { return 1.0f; }
	virtual const char *GetEatSound() const { return "Player.Eat.Apple"; }
};

// Sodas cover six flavours (ids 7..12). The flavour arrives in Use()'s value
// (the item id); Mega Soda (id 12) multiplies the endurance gain by 2.5.
class CItemUHSoda : public CUHItem
{
public:
	DECLARE_CLASS( CItemUHSoda, CUHItem );

	virtual void Spawn( void );
	virtual void Precache( void );
	virtual bool MyTouch( CBasePlayer *pPlayer );
	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
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
// Battery pack: grants several flashlight batteries on pickup (the vanilla
// item_battery grants one). Not stored in the inventory — applies directly.
class CItemBatteryPack : public CUHItem
{
public:
	DECLARE_CLASS( CItemBatteryPack, CUHItem );

	virtual void Spawn( void );
	virtual void Precache( void );
	virtual bool MyTouch( CBasePlayer *pPlayer );
	virtual int	 GetInventoryItemType() const { return UH_ITEM_UNKNOWN; }
};

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
// NOTE: item_healthkit / item_healthvial are the vanilla CHealthKit/CHealthVial
// (hl2/item_healthkit.cpp). Underhell modified them to stash into the player's
// inventory on touch (UH_ITEM_HEALTHKIT / UH_ITEM_HEALTH_VIAL) and heal +
// stop/slow bleeding when used from the inventory — see that file.
//-----------------------------------------------------------------------------
class CItemBandages : public CUHItem
{
public:
	DECLARE_CLASS( CItemBandages, CUHItem );

	virtual void Spawn( void );
	virtual void Precache( void );
	virtual bool MyTouch( CBasePlayer *pPlayer );
	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int	 GetInventoryItemType() const { return UH_ITEM_BANDAGES; }
};

// Painkillers / syringe heal on use. Original class names have no RTTI dump;
// the exact heal amounts are a reconstruction (documented TODO).
class CItemPainkillers : public CUHItem
{
public:
	DECLARE_CLASS( CItemPainkillers, CUHItem );

	virtual void Spawn( void );
	virtual void Precache( void );
	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int	 GetInventoryItemType() const { return UH_ITEM_PAINKILLERS; }
};

class CItemSyringe : public CUHItem
{
public:
	DECLARE_CLASS( CItemSyringe, CUHItem );

	virtual void Spawn( void );
	virtual void Precache( void );
	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int	 GetInventoryItemType() const { return UH_ITEM_SYRINGE; }
};

//-----------------------------------------------------------------------------
// Items seen in the original RTTI / dll strings — classnames recovered from
// the original serveror.dll string table, models still TODO.
//
// NOTE: item_sodacan / CItemSoda stays VANILLA (effects.cpp, the env_beverage
// can: CanThink + CanTouch + 1 HP). Original RTTI still has that class next
// to CEnvBeverage. Inventory sodas are item_uhsoda / CItemUHSoda.
//-----------------------------------------------------------------------------
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
	DECLARE_DATADESC();

	virtual void Spawn( void );
	virtual void Precache( void );
	virtual int	 GetInventoryItemType() const { return UH_ITEM_NONE; }

	void InputRespawn( inputdata_t &inputdata );

	// Enable flags per pool entry. Names + order = the original datamap.
	// Public: s_ItemRandomPool takes offsetof() on these (MSVC C2248 if private).
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

private:
	void SpawnRandomItem( void );
};

#endif // UH_ITEMS_H
