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
// TODO: original items likely use SOLID_VPHYSICS + pickup physics (CItem).
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
UH_DEFINE_ITEM( CItemApple,		item_apple,			"models/pg_props/pg_food/pg_apple.mdl" )
UH_DEFINE_ITEM( CItemOrange,	item_orange,		"models/props_junk/orange.mdl" )
UH_DEFINE_ITEM( CItemBanana,	item_banana,		"models/props_junk/bananna.mdl" )
UH_DEFINE_ITEM( CItemBananaBunch, item_bananabunch,	"models/props_junk/bananna_bunch.mdl" )
UH_DEFINE_ITEM( CItemSandwich,	item_sandwich,		"models/pg_props/pg_food/pg_sandwich.mdl" )
UH_DEFINE_ITEM( CItemChocobar,	item_chocobar,		"models/pg_props/pg_food/pg_choco_bar.mdl" )
UH_DEFINE_ITEM( CItemBurrito,	item_burrito,		"models/pg_props/pg_food/pg_burrito_pack.mdl" )
UH_DEFINE_ITEM( CItemUHSoda,	item_uhsoda,		"models/props_junk/popcan01a.mdl" )

//-----------------------------------------------------------------------------
// Ammo
// TODO: original models for the loose ammo entities.
//-----------------------------------------------------------------------------
UH_DEFINE_ITEM( CItemAmmoPistol,		item_ammo_pistol,		"models/items/boxsrounds.mdl" )
UH_DEFINE_ITEM( CItemAmmoPistolLarge,	item_ammo_pistol_large,	"models/items/boxsrounds.mdl" )
UH_DEFINE_ITEM( CItemBoxPistolAmmo,		item_box_pistol_ammo,	"models/pg_props/pg_weapons/pg_pistol_ammo.mdl" )
UH_DEFINE_ITEM( CItemAmmo357,			item_ammo_357,			"models/items/357ammo.mdl" )
UH_DEFINE_ITEM( CItemAmmo357Large,		item_ammo_357_large,	"models/items/357ammobox.mdl" )
UH_DEFINE_ITEM( CItemBox357Ammo,		item_box_357_ammo,		"models/pg_props/pg_weapons/pg_357_ammo.mdl" )
UH_DEFINE_ITEM( CItemAmmoSMG1,			item_ammo_smg1,			"models/items/boxsrounds.mdl" )
UH_DEFINE_ITEM( CItemAmmoSMG1Large,		item_ammo_smg1_large,	"models/items/boxmrounds.mdl" )
UH_DEFINE_ITEM( CItemAmmoSMG1Grenade,	item_ammo_smg1_grenade,	"models/items/ar2_grenade.mdl" )
UH_DEFINE_ITEM( CItemBoxSMG1Ammo,		item_box_smg1_ammo,		"models/pg_props/pg_weapons/pg_smg_ammo.mdl" )
UH_DEFINE_ITEM( CItemAmmoBuckshot,		item_ammo_buckshot,		"models/items/buckshot.mdl" )
UH_DEFINE_ITEM( CItemBoxBuckshot,		item_box_buckshot,		"models/items/boxbuckshot.mdl" )
UH_DEFINE_ITEM( CItemAmmoAR2,			item_ammo_ar2,			"models/items/ar2_ammo.mdl" )
UH_DEFINE_ITEM( CItemAmmoAR2Large,		item_ammo_ar2_large,	"models/items/combine_rifle_cartridge01.mdl" )
UH_DEFINE_ITEM( CItemAmmoAR2AltFire,	item_ammo_ar2_altfire,	"models/items/ar2_grenade.mdl" )
UH_DEFINE_ITEM( CItemBoxRifleAmmo,		item_box_rifle_ammo,	"models/pg_props/pg_weapons/pg_rifle_ammo.mdl" )
UH_DEFINE_ITEM( CItemRPGRound,			item_rpg_round,			"models/weapons/w_missile_closed.mdl" )
UH_DEFINE_ITEM( CItemAmmoCrossbow,		item_ammo_crossbow,		"models/items/crossbowrounds.mdl" )

//-----------------------------------------------------------------------------
// Equipment
//-----------------------------------------------------------------------------
UH_DEFINE_ITEM( CItemBattery,		item_battery,		"models/items/battery.mdl" )
// Original precaches "item_battery_pack"; FGD also names it "item_batterypack".
// Both classnames registered below so old maps keep working.
LINK_ENTITY_TO_CLASS( item_batterypack, CItemBatteryPack );
UH_DEFINE_ITEM( CItemBatteryPack,	item_battery_pack,	"models/pg_props/pg_obj/pg_battery_pack.mdl" )
UH_DEFINE_ITEM( CItemArmor,			item_armor,			"models/items/kevlar.mdl" )
UH_DEFINE_ITEM( CItemHeavyArmor,	item_heavyarmor,	"models/items/kevlar.mdl" )
UH_DEFINE_ITEM( CItemFlashlight,	item_flashlight,	"models/pg_props/pg_obj/pg_flashlight.mdl" )
UH_DEFINE_ITEM( CItemNightvision,	item_nightvision,	"models/items/nightvision.mdl" )
UH_DEFINE_ITEM( CItemGasmask,		item_gasmask,		"models/items/gasmask.mdl" )
UH_DEFINE_ITEM( CItemHelmetGuard,	item_helmet_guard,	"models/items/helmet_visor.mdl" )
UH_DEFINE_ITEM( CItemHelmetPrison,	item_helmet_prison,	"models/items/helmet.mdl" )
UH_DEFINE_ITEM( CItemHelmetPMC,		item_helmet_pmc,	"models/items/pmc_helmet.mdl" )
UH_DEFINE_ITEM( CItemHelmetWorker,	item_helmet_worker,	"models/items/worker_helmet.mdl" )
UH_DEFINE_ITEM( CItemFlarePack,		item_flarepack,		"models/pg_props/pg_obj/pg_flare_pack.mdl" )
UH_DEFINE_ITEM( CItemGlowstick,		item_glowstick,		"models/pg_props/pg_obj/pg_glow_stick_pack.mdl" )
UH_DEFINE_ITEM( CItemFMRadio,		item_fmradio,		"models/items/fmradio.mdl" )
UH_DEFINE_ITEM( CItemRadioCracker,	item_radiocracker,	"models/items/fmradio.mdl" )

//-----------------------------------------------------------------------------
// Health
// TODO: original health item models.
//-----------------------------------------------------------------------------
UH_DEFINE_ITEM( CItemHealthKit,		item_healthkit,		"models/healthkit.mdl" )
UH_DEFINE_ITEM( CItemHealthVial,	item_healthvial,	"models/healthvial.mdl" )
UH_DEFINE_ITEM( CItemBandages,		item_bandages,		"models/pg_props/pg_obj/pg_bandage.mdl" )
UH_DEFINE_ITEM( CItemPainkillers,	item_painkillers,	"models/healthvial.mdl" )
UH_DEFINE_ITEM( CItemSyringe,		item_syringe,		"models/healthvial.mdl" )

//-----------------------------------------------------------------------------
// item_random
// TODO: full behaviour — random pool selection, respawn, "nothing" chance.
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( item_random, CItemRandom );

BEGIN_DATADESC( CItemRandom )
	DEFINE_KEYFIELD( m_bDisableShadows,	FIELD_BOOLEAN,	"disableshadows" ),
	DEFINE_KEYFIELD( m_bCanRespawn,		FIELD_BOOLEAN,	"respawn" ),
	DEFINE_KEYFIELD( m_iNothingChance,	FIELD_INTEGER,	"nothing" ),

	// TODO: input Respawn(void)
END_DATADESC()

void CItemRandom::Precache( void )
{
	BaseClass::Precache();

	// TODO: precache the full random pool (food, ammo, equipment classes).
}

void CItemRandom::Spawn( void )
{
	Precache();
	BaseClass::Spawn();

	// TODO: pick a random item, spawn it at this origin, remove self.
	// "nothing" chance means nothing spawns at all.
}
