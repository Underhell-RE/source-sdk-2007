//========= Copyright (c) 2008, Mxthe (Underhell). All rights reserved. ============//
//
// Purpose: Underhell inventory item entities — implementation.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "uh_items.h"

//-----------------------------------------------------------------------------
// TODO: pickup behaviour (CUHItem::MyTouch).
// Original: item entity on player touch adds its id to
// CHL2_Player::m_iInventory, plays a pickup sound and removes itself.
// The inventory add logic (first free slot / stacking) still needs to be
// reconstructed from the hexrays dump.
//-----------------------------------------------------------------------------
bool CUHItem::MyTouch( CBasePlayer *pPlayer )
{
	// TODO: call pPlayer->UH_AddInventoryItem( GetInventoryItemType() );
	// then UTIL_Remove( this ). For now fall back to the HL2 item behaviour.
	return BaseClass::MyTouch( pPlayer );
}

//-----------------------------------------------------------------------------
// Shared Spawn/Precache for simple items. Model names come from Underhell.fgd.
// TODO: original items likely use SOLID_VPHYSICS + pickup physics (CUHItem).
//-----------------------------------------------------------------------------
#define UH_DEFINE_ITEM( _className, _entityName, _modelName )		\
	LINK_ENTITY_TO_CLASS( _entityName, _className );				\
	void _className::Precache( void )								\
	{																\
		BaseClass::Precache();										\
		PrecacheModel( _modelName );								\
	}																\
	void _className::Spawn( void )									\
	{																\
		Precache();													\
		SetModel( _modelName );										\
		BaseClass::Spawn();											\
	}

//-----------------------------------------------------------------------------
// Food
//-----------------------------------------------------------------------------
UH_DEFINE_ITEM( CUHItemApple,		item_apple,			"models/pg_props/pg_food/pg_apple.mdl" )
UH_DEFINE_ITEM( CUHItemOrange,	item_orange,		"models/props_junk/orange.mdl" )
UH_DEFINE_ITEM( CUHItemBanana,	item_banana,		"models/props_junk/bananna.mdl" )
UH_DEFINE_ITEM( CUHItemBananaBunch, item_bananabunch,	"models/props_junk/bananna_bunch.mdl" )
UH_DEFINE_ITEM( CUHItemSandwich,	item_sandwich,		"models/pg_props/pg_food/pg_sandwich.mdl" )
UH_DEFINE_ITEM( CUHItemChocobar,	item_chocobar,		"models/pg_props/pg_food/pg_choco_bar.mdl" )
UH_DEFINE_ITEM( CUHItemBurrito,	item_burrito,		"models/pg_props/pg_food/pg_burrito_pack.mdl" )
UH_DEFINE_ITEM( CUHItemUHSoda,	item_uhsoda,		"models/props_junk/popcan01a.mdl" )

//-----------------------------------------------------------------------------
// Ammo
// TODO: original models for the loose ammo entities.
//-----------------------------------------------------------------------------
UH_DEFINE_ITEM( CUHItemAmmoPistol,		item_ammo_pistol,		"models/items/boxsrounds.mdl" )
UH_DEFINE_ITEM( CUHItemAmmoPistolLarge,	item_ammo_pistol_large,	"models/items/boxsrounds.mdl" )
UH_DEFINE_ITEM( CUHItemBoxPistolAmmo,		item_box_pistol_ammo,	"models/pg_props/pg_weapons/pg_pistol_ammo.mdl" )
UH_DEFINE_ITEM( CUHItemAmmo357,			item_ammo_357,			"models/items/357ammo.mdl" )
UH_DEFINE_ITEM( CUHItemAmmo357Large,		item_ammo_357_large,	"models/items/357ammobox.mdl" )
UH_DEFINE_ITEM( CUHItemBox357Ammo,		item_box_357_ammo,		"models/pg_props/pg_weapons/pg_357_ammo.mdl" )
UH_DEFINE_ITEM( CUHItemAmmoSMG1,			item_ammo_smg1,			"models/items/boxsrounds.mdl" )
UH_DEFINE_ITEM( CUHItemAmmoSMG1Large,		item_ammo_smg1_large,	"models/items/boxmrounds.mdl" )
UH_DEFINE_ITEM( CUHItemAmmoSMG1Grenade,	item_ammo_smg1_grenade,	"models/items/ar2_grenade.mdl" )
UH_DEFINE_ITEM( CUHItemBoxSMG1Ammo,		item_box_smg1_ammo,		"models/pg_props/pg_weapons/pg_smg_ammo.mdl" )
UH_DEFINE_ITEM( CUHItemAmmoBuckshot,		item_ammo_buckshot,		"models/items/buckshot.mdl" )
UH_DEFINE_ITEM( CUHItemBoxBuckshot,		item_box_buckshot,		"models/items/boxbuckshot.mdl" )
UH_DEFINE_ITEM( CUHItemAmmoAR2,			item_ammo_ar2,			"models/items/ar2_ammo.mdl" )
UH_DEFINE_ITEM( CUHItemAmmoAR2Large,		item_ammo_ar2_large,	"models/items/combine_rifle_cartridge01.mdl" )
UH_DEFINE_ITEM( CUHItemAmmoAR2AltFire,	item_ammo_ar2_altfire,	"models/items/ar2_grenade.mdl" )
UH_DEFINE_ITEM( CUHItemBoxRifleAmmo,		item_box_rifle_ammo,	"models/pg_props/pg_weapons/pg_rifle_ammo.mdl" )
UH_DEFINE_ITEM( CUHItemRPGRound,			item_rpg_round,			"models/weapons/w_missile_closed.mdl" )
UH_DEFINE_ITEM( CUHItemAmmoCrossbow,		item_ammo_crossbow,		"models/items/crossbowrounds.mdl" )

//-----------------------------------------------------------------------------
// Equipment
//-----------------------------------------------------------------------------
UH_DEFINE_ITEM( CUHItemBattery,		item_battery,		"models/items/battery.mdl" )
// Original precaches "item_battery_pack"; FGD also names it "item_batterypack".
// Both classnames registered below so old maps keep working.
LINK_ENTITY_TO_CLASS( item_batterypack, CUHItemBatteryPack );
UH_DEFINE_ITEM( CUHItemBatteryPack,	item_battery_pack,	"models/pg_props/pg_obj/pg_battery_pack.mdl" )
UH_DEFINE_ITEM( CUHItemArmor,			item_armor,			"models/items/kevlar.mdl" )
UH_DEFINE_ITEM( CUHItemHeavyArmor,	item_heavyarmor,	"models/items/kevlar.mdl" )
UH_DEFINE_ITEM( CUHItemFlashlight,	item_flashlight,	"models/pg_props/pg_obj/pg_flashlight.mdl" )
UH_DEFINE_ITEM( CUHItemNightvision,	item_nightvision,	"models/items/nightvision.mdl" )
UH_DEFINE_ITEM( CUHItemGasmask,		item_gasmask,		"models/items/gasmask.mdl" )
UH_DEFINE_ITEM( CUHItemHelmetGuard,	item_helmet_guard,	"models/items/helmet_visor.mdl" )
UH_DEFINE_ITEM( CUHItemHelmetPrison,	item_helmet_prison,	"models/items/helmet.mdl" )
UH_DEFINE_ITEM( CUHItemHelmetPMC,		item_helmet_pmc,	"models/items/pmc_helmet.mdl" )
UH_DEFINE_ITEM( CUHItemHelmetWorker,	item_helmet_worker,	"models/items/worker_helmet.mdl" )
UH_DEFINE_ITEM( CUHItemFlarePack,		item_flarepack,		"models/pg_props/pg_obj/pg_flare_pack.mdl" )
UH_DEFINE_ITEM( CUHItemGlowstick,		item_glowstick,		"models/pg_props/pg_obj/pg_glow_stick_pack.mdl" )
UH_DEFINE_ITEM( CUHItemFMRadio,		item_fmradio,		"models/items/fmradio.mdl" )
UH_DEFINE_ITEM( CUHItemRadioCracker,	item_radiocracker,	"models/items/fmradio.mdl" )

//-----------------------------------------------------------------------------
// Health
// TODO: original health item models.
//-----------------------------------------------------------------------------
UH_DEFINE_ITEM( CUHItemHealthKit,		item_healthkit,		"models/healthkit.mdl" )
UH_DEFINE_ITEM( CUHItemHealthVial,	item_healthvial,	"models/healthvial.mdl" )
UH_DEFINE_ITEM( CUHItemBandages,		item_bandages,		"models/pg_props/pg_obj/pg_bandage.mdl" )
UH_DEFINE_ITEM( CUHItemPainkillers,	item_painkillers,	"models/healthvial.mdl" )
UH_DEFINE_ITEM( CUHItemSyringe,		item_syringe,		"models/healthvial.mdl" )

//-----------------------------------------------------------------------------
// item_random
// TODO: full behaviour — random pool selection, respawn, "nothing" chance.
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( item_random, CUHItemRandom );

BEGIN_DATADESC( CUHItemRandom )
	DEFINE_KEYFIELD( m_bDisableShadows,	FIELD_BOOLEAN,	"disableshadows" ),
	DEFINE_KEYFIELD( m_bCanRespawn,		FIELD_BOOLEAN,	"respawn" ),
	DEFINE_KEYFIELD( m_iNothingChance,	FIELD_INTEGER,	"nothing" ),

	// TODO: input Respawn(void)
END_DATADESC()

void CUHItemRandom::Precache( void )
{
	BaseClass::Precache();

	// TODO: precache the full random pool (food, ammo, equipment classes).
}

void CUHItemRandom::Spawn( void )
{
	Precache();
	BaseClass::Spawn();

	// TODO: pick a random item, spawn it at this origin, remove self.
	// "nothing" chance means nothing spawns at all.
}
