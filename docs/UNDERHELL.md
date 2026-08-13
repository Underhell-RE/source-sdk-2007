# UNDERHELL — Inventory Reimplementation

Reimplement Underhell inventory system 1:1. Original: closed-source mod, Source Engine 2007 (Orange Box, episodic codebase). Reference: decompiled Hex-Rays output in [`klaxons1/underhell-hexrays`](https://github.com/klaxons1/underhell-hexrays).

This file: devlog. Written per caveman guidelines — terse, no fluff, substance only.

## Goal

- Build drop-in `client.dll` + `server.dll` for existing Underhell install.
- Preserve engine-visible surface: classnames, datamap field names, sendtable prop order, console commands, exported interfaces.
- Code must stay clean. Behavior unverifiable this stage = stub + `// TODO`.

## Compatibility rules (why each decision matters)

| Surface | Constraint |
|---|---|
| Exported symbols | `CreateInterface`, `ServerGameDLL003` / `ClientGameDLL003` factories — same as SDK. No changes needed. |
| Entity classnames | `item_apple` … from FGD Item List. `LINK_ENTITY_TO_CLASS` per class. |
| Sendtable prop order | Engine matches server↔client by index inside table, not name. Order must match original exactly: `m_HL2Local, m_fIsSprinting, m_bShoulderFlashlight, m_bFlashlightOn, m_bInventoryEnabled, m_iUHBatteryCount, m_iUHHermitCardsCount, m_iUHHermitCurrentQuestCount, m_iUHHermitTotalQuestCount, m_bDisplayHermitCard, m_iInventory[28]`. |
| Datamap field names | Save/restore is name-based. Original `CHL2_Player` datamap: `m_sndWaterSplashes`, `m_fSavedSensitivity`, `m_flArmorReductionTime`, `m_flTimeNextLadderHint`, `m_flTimeUseSuspended`, `m_hHermitCardCurrentQuestCounter`, `m_hHermitCardTotalCounter`, `m_hHermitCardTotalQuestCounter`, `m_hLocatorTargetEntity`, `m_iArmorReductionFrom`, `m_iInventory`, `m_pCarryingRagdoll`, `m_bShoulderFlashlight`. |
| Console commands | `switch <a> <b>`, `dropitem <slot>`, `useitem <slot>`, `emit`, `UpdateInventory` (server, client-executable), `cl_inventoryToggle` (client). |
| Class names | Underhell modified `CHL2_Player` / `C_BaseHLPlayer` directly. No new player class. Items derive from `CItem`. |

## Reverse-engineered facts (hexrays reference)

### Player inventory state — server (`CHL2_Player`)

Original byte offsets (sendtable sub_102DD5E0):

| Member | Offset | Type |
|---|---|---|
| `m_iInventory[28]` | 4928 | int array, 4-byte stride (slot value = item id, 0 = empty) |
| `m_bShoulderFlashlight` | 5040 | bool |
| `m_iUHBatteryCount` | 5044 | int |
| `m_iUHHermitCardsCount` | 5048 | int |
| `m_iUHHermitCurrentQuestCount` | 5052 | int |
| `m_iUHHermitTotalQuestCount` | 5056 | int |
| `m_bDisplayHermitCard` | 5060 | bool |
| `m_bFlashlightOn` | 5061 | bool |
| `m_bInventoryEnabled` | 5062 | bool |
| `m_HL2Local` | 5080 | embedded datatable, `SendProxy_SendLocalDataTable` |
| `m_fIsSprinting` | 5216 | bool (1 bit) |
| item classname table | 5236 | `const char*[34]`, indexed by item id |

Also: held-item handle at byte 2164 (right after `m_hActiveWeapon`), per-slot float array at 800 (HUD weights), per-slot sync array at 1020. TODO: port held-item handle + CBasePlayer additions (`m_UHObjectives` @2676, `m_bIronSighted` @2137, `m_StuckLast` @2176).

Client (`C_BaseHLPlayer`, sub_1018E280): same prop order, own offsets (`m_iInventory` @5052, `m_HL2Local` @5164, bools @5284-5288, ints @5292+).

### Item id table (34 entries, sub_102E27A0)

| id | Print name | Classname |
|---|---|---|
| 0 | none | — |
| 1 | Apple (Red) | item_apple |
| 2 | Apple (Green) | item_apple |
| 3 | Banana | item_banana |
| 4 | Burrito | item_burrito |
| 5 | Sandwich | item_sandwich |
| 6 | Banana Bunch | item_bananabunch |
| 7–12 | Soda (6 kinds, one "Mega Soda") | item_uhsoda |
| 13 | Flare | item_flarepack |
| 14–18 | GlowStick Red/Yellow/Green/Blue/Purple | item_glowstick |
| 19–23 | Lit GlowStick (same colors) | `"nothing"` — no world entity |
| 24 | PainKillers | item_painkillers |
| 25 | Syringe | item_syringe |
| 26 | Bandages | item_bandages |
| 27 | Healthkit | item_healthkit |
| 28 | Health Vial | item_healthvial |
| 29 | Chocolate Bar | item_chocobar |
| 30 | Orange | item_orange |
| 31 | FM Radio | item_fmradio |
| 32 | Radio Cracker | item_radiocracker |

TODO: verify exact soda/health print names from hexrays string dump.

### Mechanics (decoded)

- **Use** (`UH_ItemAction`, vtable idx 411 — first Underhell virtual): read id from slot. Create world entity from classname table. Fail + id 19–23 → consume slot, remove lit glowstick child (warn if none). Success → place at eye position, spawn, call `Use( player, player, USE_ON, id )`. Glowstick 14–18 → slot becomes `id+5` (lit). Then clear slot, `engine->ClientCommand( edict, "UpdateInventory" )`.
- **Drop** (`dropitem`, same virtual with bUse=false): style + drop world item at eye position (body groups per id; flare → flare prop; glowstick → coloured glowstick prop). TODO: second drop path (sub_102DE310, eye + forward*56 + up*64, classname indexed by slot) still to reconcile.
- **Specials 19–23**: equip logic (sub_10416380) — decrement counter at player+848, remove held entity, consume slot, resync. TODO: port.
- **`UpdateInventory`** (server handler sub_102DDDE0): per non-empty slot `StateChanged( 4928+4*i )` (or dirty flag), remove held item, clear handle, `engine->ClientCommand( "UpdateInventory" )`.
- **Console commands use the client-command route**: `emit`/`switch`/`dropitem`/`useitem` are NOT registered ConCommands — the engine forwards them as client commands into `CHL2_Player::ClientCommand` (the Underhell override = hexrays sub_102DDBF0, falls through to the vanilla handler which owns `emit`). Client panel sends them via `engine->ClientCmd`. TODO: verify 2007 engine forwards `UpdateInventory` when the client dll also registers it (resync is belt-and-braces in our port since CNetworkArray marks changes itself).
- **Client**: `CInventoryPanel` = `vgui::Frame` named `InventoryPanel` (factory class `CInventoryPanel`), 28 slot children, vgui messages `NewSelection` / `NewMouseReleased`. Debug msg on construct: `"InventoryPanel has been constructed"`. `cl_inventoryToggle` → `engine->ClientCmd("UpdateInventory")` + toggle panel. Client `UpdateInventory` cmd sets refresh flag. OnThink gates on player alive + `m_bInventoryEnabled` (+ an unidentified bool at client player offset 3681 — TODO).
- **Slot visuals** (client, sub_1012E6C0): per-id sprite + localization token, replicated 1:1 — ids 24/25 (painkillers/syringe) have text only; id 12 = SodaPowerPunch / `#UnderHell_Inventory_MegaSoda`; **glowstick icon colours do not match the print names** (id 14 "Red" uses GlowstickGreen icon) — quirk preserved.

### Item entities

Full classname list: `FGD/Item List.txt` (food, ammo, equipment, health) + `Weapon List.txt` (weapons, later stage). Plus `item_random` (keyvalues `disableshadows`, `respawn`, `nothing`; input `Respawn`). Precache list (sub_101753E0) also has `item_painkillers`, `item_syringe`, `item_battery_pack`, `item_ammo_ar2_altfire`.

## Stage log

| # | Commit | Stage |
|---|---|---|
| 1 | docs | devlog + findings |
| 2 | feat(shared) | item id registry + name table |
| 3 | feat(server) | item entity classes (stubs) |
| 4 | feat(server) | CHL2_Player inventory state + use/drop + commands |
| 5 | feat(client) | CInventoryPanel + network props + client commands |
| 6 | build | vcproj registration |

## Progress

Stage 1–6 done (first batch):

- Shared: item id enum + id→classname/print-name table (exact original strings).
- Server: 40 item entity classes registered under original classnames (Spawn/Precache stubbed, models from FGD), `item_random` stub, `CHL2_Player` inventory state (sendtable in original order, datamap per original save format), `UH_ItemAction` use/drop logic ported 1:1, `switch`/`dropitem`/`useitem` via `ClientCommand`, `UpdateInventory` resync command, `uh_give_item` dev cheat.
- Client: `C_BaseHLPlayer` inventory props (recv table matches server order), `CInventoryPanel` loaded from the original `resource/UI/InventoryPanel.res` + 28 slots (4x6 grid + row of 4, per original creation loops) + original sprite/token table, `cl_inventoryToggle` + `UpdateInventory` commands.
- Build: sources registered in both episodic vcproj files.

Next stage: item pickup flow (`CUHItem::MyTouch` → `UH_AddInventoryItem`), per-item use effects, held-item handle, equip counter at player+848.

## Pickup / item_random decode (from RTTI + datamap blob + vtables)

- **Class hierarchy**: `CItemRandom : CWorldItem : CItem : CBaseAnimating`.
  `CWorldItem` adds `m_iType` + a `"type"` keyvalue; its Spawn only handles
  types 44 (item_battery) and 45 (item_suit), warning `"unable to create
  world_item %d"` otherwise — a legacy wrapper.
- **CItemRandom::Spawn** (sub_101757D0): roll 0..99; spawn when
  `roll + 1 > m_inothing * sk_itemrandom{1,2,3}[skill]`. Pool = 77 entries
  (0-based ids from the original switch): 0..25 food/gear, 26..43 ammo,
  44..74 weapons, 75/76 radios. Spawned item inherits spawnflags; the
  `disableshadows` key carries `EF_NOSHADOW` (0x10, matching const.h).
  Empty pool → `Msg("item_random item possibilites count is 0\n")`.
  `m_hOldItem` tracks the spawned item; `InputRespawn` re-rolls.
- **CItemRandom datamap extracted from the original .data blob** (82 fields,
  52-byte typedescription_t): every keyvalue name + offset recovered
  (m_bitem_* / m_bweapon_* at 804..880, m_inothing=884, m_bRespawn=888,
  m_hOldItem=892). Keyvalue names = the pool classnames.
- **Pickup flow** (CItemBanana::MyTouch, sub_101720D0): RTTI-cast to
  CHL2_Player → free-slot gate (sub_10171D30 != 28) → sound
  `HL2Player.PickupItems` (per-item extras like PickupBandages/PickupArmor)
  → SetOwnerEntity → `CHL2_Player` vtable [410] = **UH_GiveItem** →
  UTIL_Remove.
- **[410] UH_GiveItem** (sub_102726310): first free slot, else the item is
  spawned back into the world at eye + forward*56 + up*64 (drop-on-full).
- Item Spawn details: apple skin random 0/1 (red/green) picks the inventory
  id on pickup; soda skin picks flavour; armour gates on armour < 100 and
  grants 10 (TODO-verify); bandages gate on hurt/bleeding.
- Vanilla items stay vanilla (CItemBattery/CHealthKit/CItemSoda/item_sodacan/... untouched). Inventory sodas are item_uhsoda.

## RTTI + vtable validation (from original binaries)

RTTI dumps (clientRTTI.txt / serverRTTI.txt) + PE parsing of Cliento.dll /
serveror.dll let the vtables be read directly. Key results:

- **CHL2_Player vtable** (serveror.dll @ 0x1055069C):
  - `[328]` = sub_102DDBF0 = `ClientCommand` override (switch/dropitem/useitem dispatch) — our client-command routing design matches the original exactly.
  - `[409]` = sub_102E27A0 = item name-table builder (a CHL2_Player method).
  - `[410]` = sub_102DE310 = the world-drop routine (eye + forward*56 + up*64, per-id prop styling) — same switch as [411], used by a different call path (TODO: find caller).
  - `[411]` = sub_102E05F0 = `UH_ItemAction` (use; dropitem calls it with bUse=0) — attribution verified.
  - `[412]..[423]` = further Underhell player virtuals (TODO: decode).
- **Client classes**: `IInventoryPanel` (4 entries, [0]=dtor), `CInventoryPanelInterface` (4 entries: [1]=create panel via `new`, [2]=cleanup+delete, [3]=re-run slot creation), `CInventoryPanel` (259 entries; [96]=OnThink confirmed, [30]=dtor with slot loop, [27..29]=settings virtuals), `vgui::DragnDropSlot` = the 28 slot panels (a modded-vgui class, NOT in the OB SDK — our CInventorySlotPanel stands in).
- **Item classes** (serveror.dll): per-class Spawn/Precache override CItem slots [24]/[25] (CItemRandom::Precache = the registry list sub_101753E0); shared bool-returning `CItem::Use` at [64] (vanilla physcannon-impulse logic — original modified the base to return bool; our SDK's Use is void, so the failure path stays TODO).
- Original keeps vanilla item classes (CItemBattery, CHealthKit, CHealthVial, CItem_Box*Rounds, CItem_RPG_Round, ...) — our port mirrors that split.

## Save-file validation (inventory_is_full.sav)

Original save parsed. Confirmations:
- Datamap field names in the file match the implementation: `m_iInventory`, `m_bInventoryEnabled`, `m_bShoulderFlashlight`, `m_iUHBatteryCount`, `m_HL2Local` all present.
- Player inventory array decoded: 28 x 4-byte ints, empty slot = 0. The "full" save holds 27 items — values all within 1..32 and consistent with the id table (2=apple, 5=sandwich, 10/11=soda, 14/18=glowsticks, 26=bandages, 27=healthkit, 28=healthvial, 29=chocobar, 30=orange). No 19..23 saved — lit glowsticks are runtime state, as expected.
- Field records: `u16 fieldIndex, u16 dataLength, data` — the inventory record's length field = 112 (28 x 4). Matches the network array layout.

## Mod asset findings

- `resource/ui/inventorypanel.res`: panel = `CInventoryPanel`, x=368, y=84, wide=1024, tall=512, titlebar visible, `Texture1 = Sprites/Hud/Inventory/Inventory`, PaintBackgroundType 1. OB-era vgui has no `Texture1` key (newer-vgui feature) — background applied manually in our port.
- Slot structure (sub_1012E360 / sub_101310D0): 28 `DragnDropSlot` (ImageButton/ImagePanel) children, each **28x28 pixels**. 4x6 grid is **column-major** (`index = row + col*6`), origin **(44, 119)**, pitch 37. Extra slots 24..27 at (82,118), (82,81), (343,118), (343,81). The frame is **not** proportional — `Inventory.vtf` / `.res` is 1024x512 native (proportional scale blows it off-screen). Sprites are scheme images at `../Sprites/Hud/...`. Hover tooltip = `#UnderHell_Inventory_*`. **LMB** (code 107) = NewSelection; **RMB** (code 108) = ContextMenu "Use"/"Drop" → `useitem %i` / `dropitem %i` (sub_10130D00 / sub_10130DC0). Double-LMB uses (sub_10130ED0).
- Sprites: `materials/Sprites/Hud/Items/*.vmt` — every name from the id switch exists (128x128). `Inventory.vtf` = 1024x512 background (DXT5). `Blank.vtf` = 1x1 with alpha 0 (empty slots invisible — a "blank" panel is the original behaviour, minus the background texture).
- Localization (`resource/Underhell_english.txt`, UTF-16): all `UnderHell_Inventory_*` tokens exist; no Painkillers/Syringe tokens (quest items, text-only slots). Item descriptions document real effects: food = endurance + health, bandages stop bleeding, healthvial slows bleeding, radio/radiocracker attract enemies after 5s, flare ignites enemies, glowsticks strap to waist.
- Engine: build 4104 (OB Ep2, Feb 2010). `sv_sendtables` / `sv_dump_class_info` are 2013-era commands — unavailable here. Verification instead comes from the hexrays sendtable decode + the save file.

## Command-flow note (important)

`UpdateInventory` must NOT be a server ConCommand. Original message flow: client registers it plain (no FCVAR_CLIENTCMD_CAN_EXECUTE), so `engine->ClientCmd("UpdateInventory")` (from cl_inventoryToggle) is NOT run locally but forwarded to the server → `CHL2_Player::ClientCommand` dispatches it → resync → server sends `engine->ClientCommand(edict, "UpdateInventory")` → client executes its local handler (refresh flag). A server-side ConCommand registration would double-handle the message and risk a client/server ping-pong. Our port mirrors the original flow exactly.

## TODOs (tracked)

- [ ] Full per-item effects (health, armor, food, ammo, batteries, radio) — stubbed.
- [ ] Pickup (touch) flow: item entity → player add.
- [ ] Item stacking/count semantics.
- [ ] CBasePlayer additions port: `m_UHObjectives`, `m_bIronSighted`, `m_StuckLast`, held-item handle at 2164, per-slot arrays 800/1020.
- [ ] `CHL2PlayerLocalData` exact Underhell contents (original trimmed to 136 B).
- [ ] Original prediction data sets (server + client) — verify against hexrays.
- [ ] Verify all datamap field offsets vs original (sub_102E21B0).
- [ ] Weapons as inventory items.
- [ ] item_random pool behavior.
- [ ] Post-build copy path → mod dir (currently copies to `game/hl2/bin`).
- [ ] Client slot panel visuals + icon mapping (vgui materials).
