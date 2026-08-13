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

- **Use** (`UseItem`, vtable idx 411 — first Underhell virtual): read id from slot. Create world entity from classname table. Fail + id 19–23 → consume slot, remove lit glowstick child (warn if none). Success → place at eye position, spawn, call `Use( player, player, USE_ON, id )`. Glowstick 14–18 → slot becomes `id+5` (lit). Then clear slot, `engine->ClientCommand( edict, "UpdateInventory" )`.
- **Drop** (`dropitem`): spawn entity at eye pos + forward*56 + up*64 from classname by slot. TODO: name array access confirmed, full geometry TBD.
- **Specials 19–23**: equip logic (sub_10416380) — decrement counter at player+848, remove held entity, consume slot, resync. TODO: port.
- **`UpdateInventory`** (server handler sub_102DDDE0): per non-empty slot `StateChanged( 4928+4*i )` (or dirty flag), remove held item, clear handle, `engine->ClientCommand( "UpdateInventory" )`.
- **Client**: `CInventoryPanel` = `vgui::Frame` named `InventoryPanel` (factory class `CInventoryPanel`), 28 slot children, vgui messages `NewSelection` / `NewMouseReleased`. Debug msg on construct: `"InventoryPanel has been constructed"`. `cl_inventoryToggle` → `engine->ClientCmd("UpdateInventory")` + toggle panel. Client `UpdateInventory` cmd sets refresh flag.

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
