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
UH_DEFINE_ITEM( CItemApple,			item_apple,			"models/pg_props/pg_food/pg_apple.mdl" )
UH_DEFINE_ITEM( CItemOrange,		item_orange,		"models/props_junk/orange.mdl" )
UH_DEFINE_ITEM( CItemBanana,		item_banana,		"models/props_junk/bananna.mdl" )
UH_DEFINE_ITEM( CItemBananaBunch,	item_bananabunch,	"models/props_junk/bananna_bunch.mdl" )
UH_DEFINE_ITEM( CItemSandwich,		item_sandwich,		"models/pg_props/pg_food/pg_sandwich.mdl" )
UH_DEFINE_ITEM( CItemChocobar,		item_chocobar,		"models/pg_props/pg_food/pg_choco_bar.mdl" )
UH_DEFINE_ITEM( CItemBurrito,		item_burrito,		"models/pg_props/pg_food/pg_burrito_pack.mdl" )
UH_DEFINE_ITEM( CItemUHSoda,		item_uhsoda,		"models/props_junk/popcan01a.mdl" )

//-----------------------------------------------------------------------------
// Underhell ammo pickups (boxed). Models from Underhell.fgd.
//-----------------------------------------------------------------------------
UH_DEFINE_ITEM( CItem_UHPistolAmmo,	item_box_pistol_ammo,	"models/pg_props/pg_weapons/pg_pistol_ammo.mdl" )
UH_DEFINE_ITEM( CItem_UH357Ammo,	item_box_357_ammo,		"models/pg_props/pg_weapons/pg_357_ammo.mdl" )
UH_DEFINE_ITEM( CItem_UHSMG1Ammo,	item_box_smg1_ammo,		"models/pg_props/pg_weapons/pg_smg_ammo.mdl" )
UH_DEFINE_ITEM( CItem_UHRifleAmmo,	item_box_rifle_ammo,	"models/pg_props/pg_weapons/pg_rifle_ammo.mdl" )
UH_DEFINE_ITEM( CItem_UHBuckShot,	item_ammo_buckshot,		"models/items/buckshot.mdl" )

//-----------------------------------------------------------------------------
// Equipment
//-----------------------------------------------------------------------------
// Original precaches "item_battery_pack"; the FGD also names it "item_batterypack".
// Both classnames registered so old maps keep working.
LINK_ENTITY_TO_CLASS( item_batterypack, CItemBatteryPack );
UH_DEFINE_ITEM( CItemBatteryPack,	item_battery_pack,	"models/pg_props/pg_obj/pg_battery_pack.mdl" )
UH_DEFINE_ITEM( CItemArmor,			item_armor,			"models/items/kevlar.mdl" )
UH_DEFINE_ITEM( CItemHeavyArmor,	item_heavyarmor,	"models/items/kevlar.mdl" )
UH_DEFINE_ITEM( CItemFlashlight,	item_flashlight,	"models/pg_props/pg_obj/pg_flashlight.mdl" )
UH_DEFINE_ITEM( CItemNightVision,	item_nightvision,	"models/items/nightvision.mdl" )
UH_DEFINE_ITEM( CItemGasMask,		item_gasmask,		"models/items/gasmask.mdl" )
UH_DEFINE_ITEM( CItemHelmetGuard,	item_helmet_guard,	"models/items/helmet_visor.mdl" )
UH_DEFINE_ITEM( CItemHelmetPrison,	item_helmet_prison,	"models/items/helmet.mdl" )
UH_DEFINE_ITEM( CItemHelmetPMC,		item_helmet_pmc,	"models/items/pmc_helmet.mdl" )
UH_DEFINE_ITEM( CItemHelmetWorker,	item_helmet_worker,	"models/items/worker_helmet.mdl" )
UH_DEFINE_ITEM( CItemFlarePack,		item_flarepack,		"models/pg_props/pg_obj/pg_flare_pack.mdl" )
UH_DEFINE_ITEM( CItemGlowStick,		item_glowstick,		"models/pg_props/pg_obj/pg_glow_stick_pack.mdl" )
UH_DEFINE_ITEM( CItemFMRadio,		item_fmradio,		"models/items/fmradio.mdl" )
UH_DEFINE_ITEM( CItemRadioCracker,	item_radiocracker,	"models/items/fmradio.mdl" )

//-----------------------------------------------------------------------------
// Health
// TODO: original models for the bandages/painkillers/syringe entities.
//-----------------------------------------------------------------------------
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
