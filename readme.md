# Underhell — Source SDK 2007 Reimplementation

Drop-in `server.dll` + `client.dll` for Underhell (Orange Box, Episodic, build 4104).
Rebuilt 1:1 from decompiled `serveror.dll` / `Cliento.dll` (see [`klaxons1/underhell-hexrays`](https://github.com/klaxons1/underhell-hexrays)).

Goal: same engine-visible surface — classnames, datamap field names, sendtable prop order, console commands — with clean, portable code.

## What is implemented

### Player / Inventory (CHL2_Player)
- Sendtable order exactly as original: `m_HL2Local, m_fIsSprinting, m_bShoulderFlashlight, m_bFlashlightOn, m_bInventoryEnabled, m_iUHBatteryCount, m_iUHHermitCardsCount, m_iUHHermitCurrentQuestCount, m_iUHHermitTotalQuestCount, m_bDisplayHermitCard, m_iInventory[28]`
- Datamap: `m_iInventory`, `m_bShoulderFlashlight`, `m_iUHBatteryCount`, etc., matches save file `inventory_is_full.sav`
- Commands via `ClientCommand` route (no ConCommand registration, like original): `switch <a> <b>`, `dropitem <slot>`, `useitem <slot>`, `UpdateInventory`, `emit`, `cl_inventoryToggle` (client)
- `UH_ItemAction` vtable [411]: use spawns world entity at eye + forward*56 + up*64, calls `Use(..., id)`, glowstick 14..18 → 19..23 (lit), then `ClientCommand UpdateInventory`
- `UH_GiveItem` vtable [410]: first free slot, else drops to world
- `UpdateInventory` marks non-empty slots dirty, clears held handle (TODO), client refresh

### Items
- ID table 0..32 (sub_102E27A0): `item_apple` (red/green via skin), `item_banana`, `item_bananabunch`, `item_burrito`, `item_sandwich`, `item_uhsoda` x6 (incl Mega Soda), `item_flarepack`, `item_glowstick` x5, lit glowsticks `nothing` (no world entity), `item_painkillers`, `item_syringe`, `item_bandages`, `item_healthkit`, `item_healthvial`, `item_chocobar`, `item_orange`, `item_fmradio`, `item_radiocracker`
- Entity classes: `LINK_ENTITY_TO_CLASS` per original classname, models from FGD
- `item_random`: pool 77 entries (food/gear 0..25, ammo 26..43, weapons 44..74, radios 75/76), `disableshadows`, `respawn`, `nothing`, `Respawn` input, `EF_NOSHADOW` handling

### Objectives / Messaging
- `DispObj` (and `GiveSign` → `GiveSignal`, `SkipScene` → `Relay_SkipScene`) — 1:1 from `sub_101F11D0`: `gEntList.FindEntityByName("Display_Objective")` + `CLogicRelay::InputTrigger`
- VMF `Uh_House_1_d.vmf` / `uh_prologue_2_d.vmf`: `Display_Objective` (logic_relay) → `MainObjective,ShowMessage`, `OnMapSpawn SetMessagePriority1 @titles_*.txt_*`
- `env_message` extended: inputs `InputMessage`, `SetMessage`, `SetMessagePriority1..16`, `RemoveMessagePriority`, array `m_iszMessagesPriority[16]`, parsing `@titles_*.txt_*` → entry name, `GetTitlesEntry()` loads `scripts/titles_House.txt` / `titles_Prologue.txt` / `titles_Chapter1.txt` via `KeyValues` to get `positionx/y`, `effect`, `r1/g1/b1`, `Message` → `UTIL_HudMessage` with correct color/position (chest comments middle-bottom vs objectives middle). Fixes console spam `KeyValues Error: missing { in file scripts/titles.txt` by excluding GoldSrc-style `titles.txt`.

### Client
- `C_BaseHLPlayer` recv table same order as server
- `CInventoryPanel` (`vgui::Frame`, `InventoryPanel`): 28 slots, `resource/UI/InventoryPanel.res` (1024x512 stretched), background `Sprites/Hud/Inventory/Inventory`, 8×4 grid pitch 84, UV-mapped pockets, context menu Use/Drop, `NewSelection` / `NewMouseReleased`, debug msg `InventoryPanel has been constructed`

## Custom entities (Underhell)

### Implemented
| entity | file | notes |
|---|---|---|
| `item_apple`, `item_banana`, `item_bananabunch`, `item_burrito`, `item_sandwich`, `item_chocobar`, `item_orange`, `item_uhsoda` | `uh_items.cpp` | food, skin → id mapping |
| `item_glowstick`, `item_flarepack`, `item_battery_pack`, `item_bandages`, `item_healthkit`, `item_healthvial`, `item_fmradio`, `item_radiocracker`, `item_painkillers`, `item_syringe`, `item_flashlight`, `item_gasmask`, `item_helmet_*`, `item_armor` | `uh_items.cpp` | gear/health, bodygroup styling for drop |
| `item_random` | `uh_items.cpp` | pool, `sk_itemrandom`, `m_hOldItem` |
| `env_message` (extended) | `envmessage.cpp/.h` | `SetMessagePriority1..16`, `InputMessage`, titles file loading, correct color/pos |
| `logic_relay` `Display_Objective`, `MainObjective`, `SecondaryObjective` | map / `uh_player_objectives.cpp` | `DispObj` → `MainObjective,ShowMessage` |

### Stubbed (spawn/precache exists, logic TODO)
- per-item use effects: health/armor/food/endurance/bleeding, battery count, radio attract
- `CUHItem::MyTouch` → `UH_AddInventoryItem` (pickup sound, owner, remove)
- held-item handle at `CBasePlayer` 2164, `m_hActiveGlowStick`, per-slot arrays 800/1020
- `m_UHObjectives` @2676 (0x200 = 8 players × 16 slots), `CObjectivesState` datamap (sub_101F2910), `m_bIronSighted` @2137, `m_StuckLast`, `m_HL2Local` trimmed
- `item_random` respawn logic, `CWorldItem` legacy wrapper

### Not implemented yet
- weapons as inventory items: `weapon_melee_*`, `weapon_pistol_*` (glock/beretta/python/dualberetta/socom), `weapon_shotgun_*` (m3/m5/spas12/xm1014), `weapon_smg_*` (mp5/mp5_eod/mp7), `weapon_rifle_*` (g36k/sniper), `weapon_bfg_*` (mgl/minigun) — currently vanilla SDK weapons
- NPCs: `npc_infected`, `npc_ace`, `npc_butcher`
- full `CHL2PlayerLocalData` (136B), prediction datasets
- `m_HL2Local` `m_bDisplayReticle` etc. already but not all original fields
- post-build copy to mod dir, `vgui::DragnDropSlot` original class

## Build
`game/server/server_episodic-2005.vcproj` includes `underhell/` filter. Output `game/hl2/bin/server.dll` / `client.dll` copied via post-build custom step.

## Reference
- decompiled: `klaxons1/underhell-hexrays` (`serveror.dll`, `Cliento.dll`, RTTI, save files, vmf `Uh_House_1_d.vmf`, `uh_prologue_2_d.vmf`, FGD `underhell_base.fgd`)
- engine 4104, no `sv_sendtables`

## License
Valve SDK + Underhell mod assets remain property of their owners. Code in this repo is reimplementation for preservation, clean-room, no original binaries included.
