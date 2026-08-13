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

## Save-file validation (inventory_is_full.sav)

Original save parsed. Confirmations:
- Datamap field names in the file match the implementation: `m_iInventory`, `m_bInventoryEnabled`, `m_bShoulderFlashlight`, `m_iUHBatteryCount`, `m_HL2Local` all present.
- Player inventory array decoded: 28 x 4-byte ints, empty slot = 0. The "full" save holds 27 items — values all within 1..32 and consistent with the id table (2=apple, 5=sandwich, 10/11=soda, 14/18=glowsticks, 26=bandages, 27=healthkit, 28=healthvial, 29=chocobar, 30=orange). No 19..23 saved — lit glowsticks are runtime state, as expected.
- Field records: `u16 fieldIndex, u16 dataLength, data` — the inventory record's length field = 112 (28 x 4). Matches the network array layout.

## Mod asset findings

- `resource/ui/inventorypanel.res`: panel = `CInventoryPanel`, x=368, y=84, wide=1024, tall=512, titlebar visible, `Texture1 = Sprites/Hud/Inventory/Inventory`, PaintBackgroundType 1. OB-era vgui has no `Texture1` key (newer-vgui feature) — background applied manually in our port.
- Slot structure (ctor, sub_1012E360): 28 slot panels = 4x6 grid (indices 0..23) + 4 extras (24..27) at (82,81), (82,118), (343,81), (343,118). Pitch 37, origin (44, 28), icon size 28 — all passed through the vgui scheme's proportional scaling (`dword_1047CA7C` = `VGUI_Scheme010` interface). Our layout uses the same constants through `GetProportionalScaledValueEx`.
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
