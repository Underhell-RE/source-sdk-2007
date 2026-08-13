//========= Copyright (c) 2008, Mxthe (Underhell). All rights reserved. ============//
//
// Purpose: Underhell inventory item entities — implementation.
//
// Reconstructed from the original server.dll (see docs/UNDERHELL.md for the
// full decode map).
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hl2_player.h"
#include "uh_items.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Sounds shared by the pickup flow (original precaches these per item).
//-----------------------------------------------------------------------------
static void UH_PrecacheItemSounds( void )
{
	PrecacheScriptSound( "HL2Player.PickupItems" );
}

//-----------------------------------------------------------------------------
// Default pickup: the original casts the toucher to CHL2_Player via RTTI,
// refuses when the inventory is full, plays the pickup sound, sets the
// player as owner and gives the item (CHL2_Player vtable [410]).
//-----------------------------------------------------------------------------
bool CUHItem::MyTouch( CBasePlayer *pPlayer )
{
	CHL2_Player *pHL2Player = dynamic_cast<CHL2_Player *>( pPlayer );
	if ( !pHL2Player )
		return false;

	int iItem = GetInventoryItemType();
	if ( !UH_IsValidInventoryItem( iItem ) )
		return false;	// not an inventory item — derived classes auto-apply

	// Original free-slot gate (sub_10171D30 != 28).
	if ( pHL2Player->UH_FindFreeSlot() < 0 )
		return false;

	pHL2Player->EmitSound( "HL2Player.PickupItems" );
	SetOwnerEntity( pHL2Player );
	pHL2Player->UH_GiveItem( iItem );
	UTIL_Remove( this );

	return true;
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
		UH_PrecacheItemSounds();									\
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
UH_DEFINE_ITEM( CItemOrange,		item_orange,		"models/props_junk/orange.mdl" )
UH_DEFINE_ITEM( CItemBanana,		item_banana,		"models/props_junk/bananna.mdl" )
UH_DEFINE_ITEM( CItemBananaBunch,	item_bananabunch,	"models/props_junk/bananna_bunch.mdl" )
UH_DEFINE_ITEM( CItemSandwich,		item_sandwich,		"models/pg_props/pg_food/pg_sandwich.mdl" )
UH_DEFINE_ITEM( CItemChocobar,		item_chocobar,		"models/pg_props/pg_food/pg_choco_bar.mdl" )
UH_DEFINE_ITEM( CItemBurrito,		item_burrito,		"models/pg_props/pg_food/pg_burrito_pack.mdl" )

//-----------------------------------------------------------------------------
// Apples: random red/green skin on spawn, the skin picks the inventory id
// (original CItemApple::Spawn, sub_10171E90).
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( item_apple, CItemApple );

void CItemApple::Precache( void )
{
	BaseClass::Precache();
	PrecacheModel( "models/pg_props/pg_food/pg_apple.mdl" );
	UH_PrecacheItemSounds();
	PrecacheScriptSound( "Player.Eat" );
}

void CItemApple::Spawn( void )
{
	Precache();
	SetModel( "models/pg_props/pg_food/pg_apple.mdl" );
	m_nSkin = random->RandomInt( 0, 1 );
	BaseClass::Spawn();
}

bool CItemApple::MyTouch( CBasePlayer *pPlayer )
{
	CHL2_Player *pHL2Player = dynamic_cast<CHL2_Player *>( pPlayer );
	if ( !pHL2Player )
		return false;

	if ( pHL2Player->UH_FindFreeSlot() < 0 )
		return false;

	pHL2Player->EmitSound( "HL2Player.PickupItems" );
	SetOwnerEntity( pHL2Player );

	// Red skin = id 1, green skin = id 2 (the original picks the id by skin).
	pHL2Player->UH_GiveItem( UH_ITEM_APPLE_RED + m_nSkin );
	UTIL_Remove( this );

	return true;
}

//-----------------------------------------------------------------------------
// Sodas: six flavours, ids 7..12.
// TODO: verify the original picks the flavour on spawn (skin) or on pickup.
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( item_uhsoda, CItemUHSoda );

void CItemUHSoda::Precache( void )
{
	BaseClass::Precache();
	PrecacheModel( "models/props_junk/popcan01a.mdl" );
	UH_PrecacheItemSounds();
	PrecacheScriptSound( "Player.Eat" );
}

void CItemUHSoda::Spawn( void )
{
	Precache();
	SetModel( "models/props_junk/popcan01a.mdl" );
	m_nSkin = random->RandomInt( 0, 5 );
	BaseClass::Spawn();
}

bool CItemUHSoda::MyTouch( CBasePlayer *pPlayer )
{
	CHL2_Player *pHL2Player = dynamic_cast<CHL2_Player *>( pPlayer );
	if ( !pHL2Player )
		return false;

	if ( pHL2Player->UH_FindFreeSlot() < 0 )
		return false;

	pHL2Player->EmitSound( "HL2Player.PickupItems" );
	SetOwnerEntity( pHL2Player );

	pHL2Player->UH_GiveItem( UH_ITEM_SODA_FIRST + m_nSkin );
	UTIL_Remove( this );

	return true;
}

//-----------------------------------------------------------------------------
// Glowsticks: five colours, ids 14..18.
// TODO: verify the colour source (spawn skin vs pickup roll).
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( item_glowstick, CItemGlowStick );

void CItemGlowStick::Precache( void )
{
	BaseClass::Precache();
	PrecacheModel( "models/pg_props/pg_obj/pg_glow_stick_pack.mdl" );
	UH_PrecacheItemSounds();
}

void CItemGlowStick::Spawn( void )
{
	Precache();
	SetModel( "models/pg_props/pg_obj/pg_glow_stick_pack.mdl" );
	BaseClass::Spawn();
}

bool CItemGlowStick::MyTouch( CBasePlayer *pPlayer )
{
	CHL2_Player *pHL2Player = dynamic_cast<CHL2_Player *>( pPlayer );
	if ( !pHL2Player )
		return false;

	if ( pHL2Player->UH_FindFreeSlot() < 0 )
		return false;

	pHL2Player->EmitSound( "HL2Player.PickupItems" );
	SetOwnerEntity( pHL2Player );

	pHL2Player->UH_GiveItem( UH_ITEM_GLOWSTICK_FIRST + random->RandomInt( 0, 4 ) );
	UTIL_Remove( this );

	return true;
}

//-----------------------------------------------------------------------------
// Underhell ammo pickups (boxed). Models from Underhell.fgd.
// TODO: ammo auto-add semantics (amounts per weapon type).
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
UH_DEFINE_ITEM( CItemHeavyArmor,	item_heavyarmor,	"models/items/kevlar.mdl" )
UH_DEFINE_ITEM( CItemFlashlight,	item_flashlight,	"models/pg_props/pg_obj/pg_flashlight.mdl" )
UH_DEFINE_ITEM( CItemNightVision,	item_nightvision,	"models/items/nightvision.mdl" )
UH_DEFINE_ITEM( CItemGasMask,		item_gasmask,		"models/items/gasmask.mdl" )
UH_DEFINE_ITEM( CItemHelmetGuard,	item_helmet_guard,	"models/items/helmet_visor.mdl" )
UH_DEFINE_ITEM( CItemHelmetPrison,	item_helmet_prison,	"models/items/helmet.mdl" )
UH_DEFINE_ITEM( CItemHelmetPMC,		item_helmet_pmc,	"models/items/pmc_helmet.mdl" )
UH_DEFINE_ITEM( CItemHelmetWorker,	item_helmet_worker,	"models/items/worker_helmet.mdl" )
UH_DEFINE_ITEM( CItemFlarePack,		item_flarepack,		"models/pg_props/pg_obj/pg_flare_pack.mdl" )
UH_DEFINE_ITEM( CItemFMRadio,		item_fmradio,		"models/items/fmradio.mdl" )
UH_DEFINE_ITEM( CItemRadioCracker,	item_radiocracker,	"models/items/fmradio.mdl" )

//-----------------------------------------------------------------------------
// Armour auto-applies on pickup, only while below full
// (original sub_10171F60: gate on armour < 100, sound PickupArmor).
// TODO: verify the granted amount (10 was passed to the original check).
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( item_armor, CItemArmor );

void CItemArmor::Precache( void )
{
	BaseClass::Precache();
	PrecacheModel( "models/items/kevlar.mdl" );
	UH_PrecacheItemSounds();
	PrecacheScriptSound( "HL2Player.PickupArmor" );
}

void CItemArmor::Spawn( void )
{
	Precache();
	SetModel( "models/items/kevlar.mdl" );
	BaseClass::Spawn();
}

bool CItemArmor::MyTouch( CBasePlayer *pPlayer )
{
	CHL2_Player *pHL2Player = dynamic_cast<CHL2_Player *>( pPlayer );
	if ( !pHL2Player )
		return false;

	// Original gate: only pick up while armour is below full.
	if ( pHL2Player->ArmorValue() >= 100 )
		return false;

	pHL2Player->EmitSound( "HL2Player.PickupArmor" );
	SetOwnerEntity( pHL2Player );

	pHL2Player->IncrementArmorValue( 10, 100 );
	UTIL_Remove( this );

	return true;
}

//-----------------------------------------------------------------------------
// Health
//-----------------------------------------------------------------------------
UH_DEFINE_ITEM( CItemPainkillers,	item_painkillers,	"models/healthvial.mdl" )
UH_DEFINE_ITEM( CItemSyringe,		item_syringe,		"models/healthvial.mdl" )

// Bandages: original gates the pickup on the player being hurt or bleeding
// (sub_101725C0) and plays its own pickup sound.
// TODO: port the bleed-stop effect (player bleed state, offset 547 in the
// original — the SDK has no equivalent member yet).
LINK_ENTITY_TO_CLASS( item_bandages, CItemBandages );

void CItemBandages::Precache( void )
{
	BaseClass::Precache();
	PrecacheModel( "models/pg_props/pg_obj/pg_bandage.mdl" );
	UH_PrecacheItemSounds();
	PrecacheScriptSound( "HL2Player.PickupBandages" );
}

void CItemBandages::Spawn( void )
{
	Precache();
	SetModel( "models/pg_props/pg_obj/pg_bandage.mdl" );
	BaseClass::Spawn();
}

bool CItemBandages::MyTouch( CBasePlayer *pPlayer )
{
	CHL2_Player *pHL2Player = dynamic_cast<CHL2_Player *>( pPlayer );
	if ( !pHL2Player )
		return false;

	// Original gate: only while hurt or bleeding.
	// TODO: bleeding check once the bleed system is ported.
	if ( pHL2Player->GetHealth() >= 100 )
		return false;

	if ( pHL2Player->UH_FindFreeSlot() < 0 )
		return false;

	pHL2Player->EmitSound( "HL2Player.PickupBandages" );
	SetOwnerEntity( pHL2Player );
	pHL2Player->UH_GiveItem( UH_ITEM_BANDAGES );
	UTIL_Remove( this );

	return true;
}

//-----------------------------------------------------------------------------
// Classnames from the original serveror.dll string table; models TODO.
//-----------------------------------------------------------------------------
UH_DEFINE_ITEM( CItemSoda,				item_sodacan,			"models/props_junk/popcan01a.mdl" )
UH_DEFINE_ITEM( CItemShield,			item_shield,			"models/error.mdl" )	// TODO: model
UH_DEFINE_ITEM( CItemShoulderFlashlight, item_shoulderflashlight, "models/error.mdl" )	// TODO: model
UH_DEFINE_ITEM( CItemCapPMC,			item_cap_pmc,			"models/error.mdl" )	// TODO: model
UH_DEFINE_ITEM( CItemHeadsetPMC,		item_headset_pmc,		"models/error.mdl" )	// TODO: model
UH_DEFINE_ITEM( CItemRespiratorGuard,	item_respirator_guard,	"models/error.mdl" )	// TODO: model
UH_DEFINE_ITEM( CItemGasmaskGuard,		item_gasmask_guard,		"models/error.mdl" )	// TODO: model

//-----------------------------------------------------------------------------
// item_random
//
// Original (sub_101757D0): roll 0..99 against "nothing" * skill multiplier
// (sk_itemrandom1/2/3 by skill level); on a hit, pick a random entry from
// the enabled pool and spawn its entity here. The spawned item inherits the
// spawnflags; EF_NOSHADOW ("disableshadows") carries over.
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( item_random, CItemRandom );

// Skill multipliers for the "nothing" chance (original convar names).
static ConVar sk_itemrandom1( "sk_itemrandom1", "1", FCVAR_ARCHIVE, "Item random: nothing-chance multiplier on Easy" );
static ConVar sk_itemrandom2( "sk_itemrandom2", "1", FCVAR_ARCHIVE, "Item random: nothing-chance multiplier on Normal" );
static ConVar sk_itemrandom3( "sk_itemrandom3", "1", FCVAR_ARCHIVE, "Item random: nothing-chance multiplier on Hard" );

static float UH_GetItemRandomSkillMultiplier( void )
{
	switch ( g_iSkillLevel )
	{
	case SKILL_EASY:
		return sk_itemrandom1.GetFloat();
	case SKILL_HARD:
		return sk_itemrandom3.GetFloat();
	default:
		return sk_itemrandom2.GetFloat();
	}
}

// The fixed pool: entry id -> entity classname + enable flag. Ids, classnames
// and keyvalue names all come from the original (datamap blob + Spawn switch).
struct UHRandomPoolEntry_t
{
	int		m_iType;		// pool id (0-based, as in the original switch)
	const char *m_pszClass;	// entity classname AND keyvalue name
	size_t	m_nFlagOffset;	// offsetof( CItemRandom, m_bXxx )
};

#define UH_RANDOM_ENTRY( _id, _class, _flag ) \
	{ _id, _class, offsetof( CItemRandom, _flag ) }

static const UHRandomPoolEntry_t s_ItemRandomPool[] =
{
	UH_RANDOM_ENTRY(  0, "item_chocobar",				m_bitem_chocobar ),
	UH_RANDOM_ENTRY(  1, "item_orange",				m_bitem_orange ),
	UH_RANDOM_ENTRY(  2, "item_burrito",				m_bitem_burrito ),
	UH_RANDOM_ENTRY(  3, "item_sandwich",			m_bitem_sandwich ),
	UH_RANDOM_ENTRY(  4, "item_apple",				m_bitem_apple ),
	UH_RANDOM_ENTRY(  5, "item_banana",				m_bitem_banana ),
	UH_RANDOM_ENTRY(  6, "item_bananabunch",			m_bitem_bananabunch ),
	UH_RANDOM_ENTRY(  7, "item_uhsoda",				m_bitem_soda ),
	UH_RANDOM_ENTRY(  8, "item_flarepack",			m_bitem_flarepack ),
	UH_RANDOM_ENTRY(  9, "item_glowstick",			m_bitem_glowstick ),
	UH_RANDOM_ENTRY( 10, "item_painkillers",			m_bitem_painkillers ),
	UH_RANDOM_ENTRY( 11, "item_syringe",				m_bitem_syringe ),
	UH_RANDOM_ENTRY( 12, "item_syringepack",			m_bitem_syringepack ),
	UH_RANDOM_ENTRY( 13, "item_bandages",			m_bitem_bandages ),
	UH_RANDOM_ENTRY( 14, "item_bandagespack",		m_bitem_bandagespack ),
	UH_RANDOM_ENTRY( 15, "item_armor",				m_bitem_armor ),
	UH_RANDOM_ENTRY( 16, "item_heavyarmor",			m_bitem_heavyarmor ),
	UH_RANDOM_ENTRY( 17, "item_battery",				m_bitem_battery ),
	UH_RANDOM_ENTRY( 18, "item_battery_pack",		m_bitem_batterypack ),
	UH_RANDOM_ENTRY( 19, "item_healthkit",			m_bitem_healthkit ),
	UH_RANDOM_ENTRY( 20, "item_healthvial",			m_bitem_healthvial ),
	UH_RANDOM_ENTRY( 21, "item_nightvision",			m_bitem_nightvision ),
	UH_RANDOM_ENTRY( 22, "item_flashlight",			m_bitem_flashlight ),
	UH_RANDOM_ENTRY( 23, "item_helmet_prison",		m_bitem_helmet_prison ),
	UH_RANDOM_ENTRY( 24, "item_helmet_guard",		m_bitem_helmet_guard ),
	UH_RANDOM_ENTRY( 25, "item_helmet_worker",		m_bitem_helmet_worker ),
	UH_RANDOM_ENTRY( 26, "item_ammo_357",			m_bitem_ammo_357 ),
	UH_RANDOM_ENTRY( 27, "item_ammo_357_large",		m_bitem_ammo_357_large ),
	UH_RANDOM_ENTRY( 28, "item_ammo_ar2",			m_bitem_ammo_ar2 ),
	UH_RANDOM_ENTRY( 29, "item_ammo_ar2_altfire",	m_bitem_ammo_ar2_altfire ),
	UH_RANDOM_ENTRY( 30, "item_ammo_ar2_large",		m_bitem_ammo_ar2_large ),
	UH_RANDOM_ENTRY( 31, "item_ammo_crossbow",		m_bitem_ammo_crossbow ),
	UH_RANDOM_ENTRY( 32, "item_ammo_pistol",			m_bitem_ammo_pistol ),
	UH_RANDOM_ENTRY( 33, "item_ammo_pistol_large",	m_bitem_ammo_pistol_large ),
	UH_RANDOM_ENTRY( 34, "item_ammo_smg1",			m_bitem_ammo_smg1 ),
	UH_RANDOM_ENTRY( 35, "item_ammo_smg1_grenade",	m_bitem_ammo_smg1_grenade ),
	UH_RANDOM_ENTRY( 36, "item_ammo_smg1_large",		m_bitem_ammo_smg1_large ),
	UH_RANDOM_ENTRY( 37, "item_box_buckshot",		m_bitem_box_buckshot ),
	UH_RANDOM_ENTRY( 38, "item_box_357_ammo",		m_bitem_box_357_ammo ),
	UH_RANDOM_ENTRY( 39, "item_box_pistol_ammo",		m_bitem_box_pistol_ammo ),
	UH_RANDOM_ENTRY( 40, "item_box_smg1_ammo",		m_bitem_box_smg1_ammo ),
	UH_RANDOM_ENTRY( 41, "item_box_rifle_ammo",		m_bitem_box_rifle_ammo ),
	UH_RANDOM_ENTRY( 42, "item_ammo_buckshot",		m_bitem_ammo_buckshot ),
	UH_RANDOM_ENTRY( 43, "item_rpg_round",			m_bitem_rpg_round ),
	UH_RANDOM_ENTRY( 44, "weapon_physcannon",		m_bweapon_physcannon ),
	UH_RANDOM_ENTRY( 45, "weapon_crowbar",			m_bweapon_crowbar ),
	UH_RANDOM_ENTRY( 46, "weapon_wrench",			m_bweapon_wrench ),
	UH_RANDOM_ENTRY( 47, "weapon_pipe",				m_bweapon_pipe ),
	UH_RANDOM_ENTRY( 48, "weapon_axe",				m_bweapon_axe ),
	UH_RANDOM_ENTRY( 49, "weapon_hammer",			m_bweapon_hammer ),
	UH_RANDOM_ENTRY( 50, "weapon_shiv",				m_bweapon_shiv ),
	UH_RANDOM_ENTRY( 51, "weapon_pistol",			m_bweapon_pistol ),
	UH_RANDOM_ENTRY( 52, "weapon_pistol_glock",		m_bweapon_pistol_glock ),
	UH_RANDOM_ENTRY( 53, "weapon_pistol_socom",		m_bweapon_pistol_socom ),
	UH_RANDOM_ENTRY( 54, "weapon_pistol_beretta",	m_bweapon_pistol_beretta ),
	UH_RANDOM_ENTRY( 55, "weapon_pistol_dualberetta", m_bweapon_pistol_dualberetta ),
	UH_RANDOM_ENTRY( 56, "weapon_pistol_python",		m_bweapon_pistol_python ),
	UH_RANDOM_ENTRY( 57, "weapon_357",				m_bweapon_357 ),
	UH_RANDOM_ENTRY( 58, "weapon_smg1",				m_bweapon_smg1 ),
	UH_RANDOM_ENTRY( 59, "weapon_smg_mp5",			m_bweapon_smg_mp5 ),
	UH_RANDOM_ENTRY( 60, "weapon_smg_mp5_eod",		m_bweapon_smg_mp5_eod ),
	UH_RANDOM_ENTRY( 61, "weapon_smg_mp7",			m_bweapon_smg_mp7 ),
	UH_RANDOM_ENTRY( 62, "weapon_shotgun",			m_bweapon_shotgun ),
	UH_RANDOM_ENTRY( 63, "weapon_shotgun_spas12",	m_bweapon_shotgun_spas12 ),
	UH_RANDOM_ENTRY( 64, "weapon_shotgun_m3",		m_bweapon_shotgun_m3 ),
	UH_RANDOM_ENTRY( 65, "weapon_shotgun_m5",		m_bweapon_shotgun_m5 ),
	UH_RANDOM_ENTRY( 66, "weapon_shotgun_xm1014",	m_bweapon_shotgun_xm1014 ),
	UH_RANDOM_ENTRY( 67, "weapon_ar2",				m_bweapon_ar2 ),
	UH_RANDOM_ENTRY( 68, "weapon_rifle_g36k",		m_bweapon_rifle_g36k ),
	UH_RANDOM_ENTRY( 69, "weapon_rifle_sniper",		m_bweapon_rifle_sniper ),
	UH_RANDOM_ENTRY( 70, "weapon_crossbow",			m_bweapon_crossbow ),
	UH_RANDOM_ENTRY( 71, "weapon_bfg_mgl",			m_bweapon_mgl ),
	UH_RANDOM_ENTRY( 72, "weapon_rpg",				m_bweapon_rpg ),
	UH_RANDOM_ENTRY( 73, "weapon_bfg_minigun",		m_bweapon_minigun ),
	UH_RANDOM_ENTRY( 74, "weapon_frag",				m_bweapon_frag ),
	UH_RANDOM_ENTRY( 75, "item_fmradio",				m_bitem_fmradio ),
	UH_RANDOM_ENTRY( 76, "item_radiocracker",		m_bitem_radiocracker ),
};

//-----------------------------------------------------------------------------
// Datamap: keyvalue names = the original external names (extracted from the
// original datamap blob in serveror.dll).
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CItemRandom )
	DEFINE_KEYFIELD( m_bitem_chocobar,			FIELD_BOOLEAN,	"item_chocobar" ),
	DEFINE_KEYFIELD( m_bitem_sandwich,			FIELD_BOOLEAN,	"item_sandwich" ),
	DEFINE_KEYFIELD( m_bitem_apple,				FIELD_BOOLEAN,	"item_apple" ),
	DEFINE_KEYFIELD( m_bitem_orange,				FIELD_BOOLEAN,	"item_orange" ),
	DEFINE_KEYFIELD( m_bitem_banana,				FIELD_BOOLEAN,	"item_banana" ),
	DEFINE_KEYFIELD( m_bitem_bananabunch,		FIELD_BOOLEAN,	"item_bananabunch" ),
	DEFINE_KEYFIELD( m_bitem_burrito,			FIELD_BOOLEAN,	"item_burrito" ),
	DEFINE_KEYFIELD( m_bitem_soda,				FIELD_BOOLEAN,	"item_uhsoda" ),
	DEFINE_KEYFIELD( m_bitem_flarepack,			FIELD_BOOLEAN,	"item_flarepack" ),
	DEFINE_KEYFIELD( m_bitem_glowstick,			FIELD_BOOLEAN,	"item_glowstick" ),
	DEFINE_KEYFIELD( m_bitem_painkillers,		FIELD_BOOLEAN,	"item_painkillers" ),
	DEFINE_KEYFIELD( m_bitem_syringe,			FIELD_BOOLEAN,	"item_syringe" ),
	DEFINE_KEYFIELD( m_bitem_syringepack,		FIELD_BOOLEAN,	"item_syringepack" ),
	DEFINE_KEYFIELD( m_bitem_bandages,			FIELD_BOOLEAN,	"item_bandages" ),
	DEFINE_KEYFIELD( m_bitem_bandagespack,		FIELD_BOOLEAN,	"item_bandagespack" ),
	DEFINE_KEYFIELD( m_bitem_armor,				FIELD_BOOLEAN,	"item_armor" ),
	DEFINE_KEYFIELD( m_bitem_heavyarmor,			FIELD_BOOLEAN,	"item_heavyarmor" ),
	DEFINE_KEYFIELD( m_bitem_battery,			FIELD_BOOLEAN,	"item_battery" ),
	DEFINE_KEYFIELD( m_bitem_batterypack,		FIELD_BOOLEAN,	"item_battery_pack" ),
	DEFINE_KEYFIELD( m_bitem_healthkit,			FIELD_BOOLEAN,	"item_healthkit" ),
	DEFINE_KEYFIELD( m_bitem_healthvial,			FIELD_BOOLEAN,	"item_healthvial" ),
	DEFINE_KEYFIELD( m_bitem_nightvision,		FIELD_BOOLEAN,	"item_nightvision" ),
	DEFINE_KEYFIELD( m_bitem_flashlight,			FIELD_BOOLEAN,	"item_flashlight" ),
	DEFINE_KEYFIELD( m_bitem_helmet_prison,		FIELD_BOOLEAN,	"item_helmet_prison" ),
	DEFINE_KEYFIELD( m_bitem_helmet_guard,		FIELD_BOOLEAN,	"item_helmet_guard" ),
	DEFINE_KEYFIELD( m_bitem_helmet_worker,		FIELD_BOOLEAN,	"item_helmet_worker" ),
	DEFINE_KEYFIELD( m_bitem_fmradio,			FIELD_BOOLEAN,	"item_fmradio" ),
	DEFINE_KEYFIELD( m_bitem_radiocracker,		FIELD_BOOLEAN,	"item_radiocracker" ),
	DEFINE_KEYFIELD( m_bitem_ammo_357,			FIELD_BOOLEAN,	"item_ammo_357" ),
	DEFINE_KEYFIELD( m_bitem_ammo_357_large,		FIELD_BOOLEAN,	"item_ammo_357_large" ),
	DEFINE_KEYFIELD( m_bitem_ammo_ar2,			FIELD_BOOLEAN,	"item_ammo_ar2" ),
	DEFINE_KEYFIELD( m_bitem_ammo_ar2_altfire,	FIELD_BOOLEAN,	"item_ammo_ar2_altfire" ),
	DEFINE_KEYFIELD( m_bitem_ammo_ar2_large,		FIELD_BOOLEAN,	"item_ammo_ar2_large" ),
	DEFINE_KEYFIELD( m_bitem_ammo_crossbow,		FIELD_BOOLEAN,	"item_ammo_crossbow" ),
	DEFINE_KEYFIELD( m_bitem_ammo_pistol,		FIELD_BOOLEAN,	"item_ammo_pistol" ),
	DEFINE_KEYFIELD( m_bitem_ammo_pistol_large,	FIELD_BOOLEAN,	"item_ammo_pistol_large" ),
	DEFINE_KEYFIELD( m_bitem_ammo_smg1,			FIELD_BOOLEAN,	"item_ammo_smg1" ),
	DEFINE_KEYFIELD( m_bitem_ammo_smg1_grenade,	FIELD_BOOLEAN,	"item_ammo_smg1_grenade" ),
	DEFINE_KEYFIELD( m_bitem_ammo_smg1_large,	FIELD_BOOLEAN,	"item_ammo_smg1_large" ),
	DEFINE_KEYFIELD( m_bitem_box_buckshot,		FIELD_BOOLEAN,	"item_box_buckshot" ),
	DEFINE_KEYFIELD( m_bitem_box_357_ammo,		FIELD_BOOLEAN,	"item_box_357_ammo" ),
	DEFINE_KEYFIELD( m_bitem_box_pistol_ammo,	FIELD_BOOLEAN,	"item_box_pistol_ammo" ),
	DEFINE_KEYFIELD( m_bitem_box_smg1_ammo,		FIELD_BOOLEAN,	"item_box_smg1_ammo" ),
	DEFINE_KEYFIELD( m_bitem_box_rifle_ammo,		FIELD_BOOLEAN,	"item_box_rifle_ammo" ),
	DEFINE_KEYFIELD( m_bitem_ammo_buckshot,		FIELD_BOOLEAN,	"item_ammo_buckshot" ),
	DEFINE_KEYFIELD( m_bitem_rpg_round,			FIELD_BOOLEAN,	"item_rpg_round" ),
	DEFINE_KEYFIELD( m_bweapon_physcannon,		FIELD_BOOLEAN,	"weapon_physcannon" ),
	DEFINE_KEYFIELD( m_bweapon_crowbar,			FIELD_BOOLEAN,	"weapon_crowbar" ),
	DEFINE_KEYFIELD( m_bweapon_wrench,			FIELD_BOOLEAN,	"weapon_wrench" ),
	DEFINE_KEYFIELD( m_bweapon_pipe,				FIELD_BOOLEAN,	"weapon_pipe" ),
	DEFINE_KEYFIELD( m_bweapon_axe,				FIELD_BOOLEAN,	"weapon_axe" ),
	DEFINE_KEYFIELD( m_bweapon_hammer,			FIELD_BOOLEAN,	"weapon_hammer" ),
	DEFINE_KEYFIELD( m_bweapon_shiv,				FIELD_BOOLEAN,	"weapon_shiv" ),
	DEFINE_KEYFIELD( m_bweapon_pistol,			FIELD_BOOLEAN,	"weapon_pistol" ),
	DEFINE_KEYFIELD( m_bweapon_pistol_glock,		FIELD_BOOLEAN,	"weapon_pistol_glock" ),
	DEFINE_KEYFIELD( m_bweapon_pistol_socom,		FIELD_BOOLEAN,	"weapon_pistol_socom" ),
	DEFINE_KEYFIELD( m_bweapon_pistol_beretta,	FIELD_BOOLEAN,	"weapon_pistol_beretta" ),
	DEFINE_KEYFIELD( m_bweapon_pistol_dualberetta, FIELD_BOOLEAN, "weapon_pistol_dualberetta" ),
	DEFINE_KEYFIELD( m_bweapon_pistol_python,	FIELD_BOOLEAN,	"weapon_pistol_python" ),
	DEFINE_KEYFIELD( m_bweapon_357,				FIELD_BOOLEAN,	"weapon_357" ),
	DEFINE_KEYFIELD( m_bweapon_smg1,				FIELD_BOOLEAN,	"weapon_smg1" ),
	DEFINE_KEYFIELD( m_bweapon_smg_mp5,			FIELD_BOOLEAN,	"weapon_smg_mp5" ),
	DEFINE_KEYFIELD( m_bweapon_smg_mp5_eod,		FIELD_BOOLEAN,	"weapon_smg_mp5_eod" ),
	DEFINE_KEYFIELD( m_bweapon_smg_mp7,			FIELD_BOOLEAN,	"weapon_smg_mp7" ),
	DEFINE_KEYFIELD( m_bweapon_shotgun,			FIELD_BOOLEAN,	"weapon_shotgun" ),
	DEFINE_KEYFIELD( m_bweapon_shotgun_spas12,	FIELD_BOOLEAN,	"weapon_shotgun_spas12" ),
	DEFINE_KEYFIELD( m_bweapon_shotgun_m3,		FIELD_BOOLEAN,	"weapon_shotgun_m3" ),
	DEFINE_KEYFIELD( m_bweapon_shotgun_m5,		FIELD_BOOLEAN,	"weapon_shotgun_m5" ),
	DEFINE_KEYFIELD( m_bweapon_shotgun_xm1014,	FIELD_BOOLEAN,	"weapon_shotgun_xm1014" ),
	DEFINE_KEYFIELD( m_bweapon_ar2,				FIELD_BOOLEAN,	"weapon_ar2" ),
	DEFINE_KEYFIELD( m_bweapon_rifle_g36k,		FIELD_BOOLEAN,	"weapon_rifle_g36k" ),
	DEFINE_KEYFIELD( m_bweapon_rifle_sniper,		FIELD_BOOLEAN,	"weapon_rifle_sniper" ),
	DEFINE_KEYFIELD( m_bweapon_crossbow,			FIELD_BOOLEAN,	"weapon_crossbow" ),
	DEFINE_KEYFIELD( m_bweapon_mgl,				FIELD_BOOLEAN,	"weapon_bfg_mgl" ),
	DEFINE_KEYFIELD( m_bweapon_rpg,				FIELD_BOOLEAN,	"weapon_rpg" ),
	DEFINE_KEYFIELD( m_bweapon_minigun,			FIELD_BOOLEAN,	"weapon_bfg_minigun" ),
	DEFINE_KEYFIELD( m_bweapon_frag,				FIELD_BOOLEAN,	"weapon_frag" ),

	DEFINE_KEYFIELD( m_iNothingChance,	FIELD_FLOAT,	"nothing" ),
	DEFINE_KEYFIELD( m_bRespawn,		FIELD_BOOLEAN,	"respawn" ),
	DEFINE_KEYFIELD( m_bDisableShadows,	FIELD_BOOLEAN,	"disableshadows" ),
	DEFINE_FIELD( m_hOldItem,			FIELD_EHANDLE ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Respawn", InputRespawn ),
END_DATADESC()

void CItemRandom::Precache( void )
{
	BaseClass::Precache();

	// The original precaches the whole pool up front (its Precache installs
	// the item registry list, sub_101753E0). The classname links above do
	// the same job here; PrecacheModel per entry is handled by each entity's
	// own Spawn.
}

void CItemRandom::Spawn( void )
{
	Precache();
	BaseClass::Spawn();

	// Original roll: 0..99 + 1 > nothing * skill multiplier.
	float flNothingChance = m_iNothingChance * UH_GetItemRandomSkillMultiplier();
	if ( random->RandomInt( 0, 99 ) + 1 > flNothingChance )
	{
		SpawnRandomItem();
	}
	else if ( !m_bRespawn )
	{
		UTIL_Remove( this );
	}
}

void CItemRandom::SpawnRandomItem( void )
{
	// Build the pool from the enabled entries.
	CUtlVector<const UHRandomPoolEntry_t *> pool;
	for ( int i = 0; i < ARRAYSIZE( s_ItemRandomPool ); ++i )
	{
		const UHRandomPoolEntry_t *pEntry = &s_ItemRandomPool[i];
		bool bEnabled = *(bool *)( (char *)this + pEntry->m_nFlagOffset );
		if ( bEnabled )
		{
			pool.AddToTail( pEntry );
		}
	}

	if ( pool.Count() == 0 )
	{
		// Original message, verbatim.
		Msg( "item_random item possibilites count is 0\n" );
		return;
	}

	const UHRandomPoolEntry_t *pEntry = pool[random->RandomInt( 0, pool.Count() - 1 )];

	CBaseEntity *pItem = CreateEntityByName( pEntry->m_pszClass );
	if ( !pItem )
		return;

	// Original copies the spawnflags onto the spawned item and carries
	// EF_NOSHADOW ("disableshadows") over.
	pItem->AddSpawnFlags( m_spawnflags );
	if ( m_bDisableShadows )
	{
		pItem->AddEffects( EF_NOSHADOW );
	}

	pItem->SetAbsOrigin( GetAbsOrigin() );
	pItem->SetAbsAngles( GetAbsAngles() );
	pItem->Spawn();

	m_hOldItem = pItem;

	if ( !m_bRespawn )
	{
		UTIL_Remove( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Respawn input — remove the previously spawned item (if any) and
// re-roll the pool.
// TODO: verify the original removes the old item here.
//-----------------------------------------------------------------------------
void CItemRandom::InputRespawn( inputdata_t &inputdata )
{
	if ( m_hOldItem.Get() )
	{
		UTIL_Remove( m_hOldItem.Get() );
		m_hOldItem = NULL;
	}

	SpawnRandomItem();
}
