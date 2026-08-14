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
| 7 | feat(server) | endurance/hunger + stamina-recharge gating |
| 8 | feat(server) | functional food/health item use effects |
| 9 | feat(client) | CHudEndurance green bar + endurance props |
| 10 | feat(server) | healthkit/healthvial go into inventory + heal on use |
| 11 | fix(client) | endurance/stamina HUD (vertical/horizontal FgColor bars) |
| 12 | fix(server) | +use-only pickup (no auto-pickup / no physcannon grab) |
| 13 | feat(server) | flashlight on batteries (item_battery / item_batterypack) |
| 14 | fix(client) | inventory toggle closes on second "I" |
| 15 | fix(client) | bars hidden without suit; inventory not suit-gated; toggle debounce per-frame |
| 16 | feat(client) | CHudUHBattery (contour + charge + count) + CHudBleeding |
| 17 | feat(server) | bleeding system + passive hunger decay |
| 18 | feat(client) | CHudUHHermitCards (deck icon + count + quest progress) |
| 19 | feat(server) | working glowsticks (lit prop strapped to player) |
| 20 | fix(server) | healthkit/healthvial always stash into inventory (+use) |
| 21 | fix(client) | battery/cards HUD fade rate (icons now persist) |
| 22 | fix(server) | battery item (pg_battery model, +1/+2, max 20, removed on pickup) |
| 23 | fix(client) | HUD panels pinned to original geometry (no proportional blow-up) |
| 24 | feat(client) | flashlight battery charge (m_flUHBatteryCharge) drains smoothly |
| 25 | feat(server) | npc_infected functional port (variants + fast claw AI) |

## Progress

Stage 1–6 done (first batch):

- Shared: item id enum + id→classname/print-name table (exact original strings).
- Server: 40 item entity classes registered under original classnames (Spawn/Precache stubbed, models from FGD), `item_random` stub, `CHL2_Player` inventory state (sendtable in original order, datamap per original save format), `UH_ItemAction` use/drop logic ported 1:1, `switch`/`dropitem`/`useitem` via `ClientCommand`, `UpdateInventory` resync command, `uh_give_item` dev cheat.
- Client: `C_BaseHLPlayer` inventory props (recv table matches server order), `CInventoryPanel` loaded from the original `resource/UI/InventoryPanel.res` + 28 slots (4x6 grid + row of 4, per original creation loops) + original sprite/token table, `cl_inventoryToggle` + `UpdateInventory` commands.
- Build: sources registered in both episodic vcproj files.

Next stage: item pickup flow (`CUHItem::MyTouch` → `UH_AddInventoryItem`), per-item use effects, held-item handle, equip counter at player+848.

## Endurance / hunger system (stage 7–9)

Two HUD bars in the original: **red = suit power** (the vanilla sprint
"stamina", `m_HL2Local.m_flSuitPower`) and **green = endurance** (`m_iEndurance`,
the "hunger" meter). The green bar is restored by eating/drinking and consumed
as stamina recharges — lower endurance = slower stamina recharge.

Decoded from the original (sub_102E0E60 recharge branch, sub_102DF1A0 eat,
sub_102DF2E0 drink):

- `m_iEndurance` @2184 (int, CBasePlayer), `m_iBleedCounter` @2188 (int) —
  both networked (client recv table sub_10043D70 reads them @3432/@3436).
  Server-only accumulators: `m_fEStaminaCount` @2132, `m_flPseudoEndurance`
  @2148, `m_flPseudoHealth` @2144, `m_iEHealthCount` @2160 (runtime only).
- ConVars (original registrations): `uh_player_endurance` 100,
  `uh_player_endurance_rate` 1600, `uh_player_endurance_rate2` 8000,
  `uh_player_endurance_stamina_effect` 100, `uh_player_bleed_rate` 8000,
  `uh_bleeding_chance` 5.
- **Recharge** (`SuitPower_Update` → `UH_UpdateEndurance`): when the suit
  would recharge (devices off, <100 power, 0.5s delay),
  `rate = max(endurance, 25) * 0.01 * 12.5 * frametime`; charge by `rate`;
  `m_fEStaminaCount += rate`; when it reaches 50 → `--endurance` (clamped 0).
- **Eat** (`UH_Eat`, sub_102DF1A0): endurance += gain (clamp 100), health +=
  gain (clamp 200 — food can overheal), play sound. **Drink** (`UH_Drink`,
  sub_102DF2E0): same, but Mega Soda flavour (id 12, flavour 5) multiplies
  endurance gain by 2.5, plays "Player.Drink".
- Food per-item eat gains (decoded + localization): apple 5/1 ("Player.Eat.
  Apple"), banana 5/1, burrito 5/1, chocobar 5/1, orange 5/1, sandwich 10/1,
  banana bunch 15/3, soda drink 10/1 (flavour = id − 7).
- Food `Use()` gate (sub_10171E10 etc.): alive + not gas-masked + CHL2_Player.
  Gas-mask check is a TODO until the gear system is ported.
- Bandages `Use()` (sub_101725C0): usable while hurt/bleeding; stops bleed +
  heals 5 while bleeding, else heals 1.
- Client `CHudEndurance` (sub_100C8680/sub_100C8710): blue bar (HullColor
  "0 0 255 255") driven by `m_iEndurance`; <20 → "EnduranceLow", <50 →
  "EnduranceMedium", ≥50 → "EnduranceHigh" animation sequences. The original
  uses a modded-vgui panel (settings "HullColor"/"HullDisabledAlpha"/
  "BarInsetX"/"icontall") not present in the OB SDK — our `CHudEndurance` is
  a faithful behavioural clone of the vanilla suit-power bar.

### TODOs (endurance scope)

- Melee stamina drain: Underhell melee weapons carry a `StaminaToDrain` stat
  (default 15.0, parsed in sub_10274870) that drains suit power on swing —
  blocked on the Underhell melee-weapon classes (not yet ported).
- Kick: explicit TODO (no kick implementation in this repo yet).
- Full bleeding system: damage→bleed roll (`uh_bleeding_chance`), `BleedThink`
  health drain (`uh_player_bleed_rate`), and the health-vial slow-bleed effect.
  `m_iBleedCounter`/`m_flLastBleedTime` are in place and wired to bandages.
- Painkillers/syringe heal amounts are a reconstruction (no RTTI for the
  original classes; quest items with text-only slots).

## HUD bars (stage 11) — decoded from scripts/HudLayout.res + HudAnimations.txt

The mod ships its own `scripts/HudLayout.res` / `scripts/HudAnimations.txt` /
`resource/ClientScheme.res`, so the panel names, geometry and colours below
are authoritative (not reconstructed):

- **HudStamina** (horizontal bar, xpos 32 ypos 448 wide 240 tall 18): the
  Underhell stamina bar replacing the vanilla CHudSuitPower. Bar geometry:
  `BarInsetX 26, BarInsetY 7, BarWidth 210, BarHeight 4, BarChunkWidth 1,
  BarChunkGap 0`. Reads suit power; `< 35` → "StaminaLow" (red), else
  "StaminaNormal" (scheme FgColor).
- **HudEndurance** (vertical gauge, xpos 10 ypos 332 wide 18 tall 134): the
  hunger bar. `BarInsetX 7, BarInsetY 104, BarWidth 4, BarHeight 84,
  BarChunkHeight 1, BarChunkGap 0` — fills bottom-up. Reads `m_iEndurance`;
  `< 20` → "EnduranceLow", `< 50` → "EnduranceMedium", else "EnduranceHigh".
- **Colour model**: both bars draw with the panel **FgColor**, animated by
  HudAnimations.txt: scheme `FgColor` = `0 128 255` (blue), endurance medium
  = `230 230 50` (yellow), `DamagedFg` = `180 0 0` (red). So blue → yellow →
  red as the meter drains (the user's observation: blue, yellowing/reddening
  toward zero). The stamina bar only goes blue → red (two states).
- The original bars also draw an icon/contour sprite (`sprites/hud/hud_stamina`,
  `sprites/hud/hud_endurance`) and use `PaintBackgroundType 2` (rounded box).
  The port draws the chunked bar only; the icon sprite is a TODO.

## Pickup model (stage 12) — verified: no auto-pickup

Underhell items are **not** auto-picked by touch and are not grabbed as
physics props:

- `CItem::ItemTouch` (sub_10177A20) still exists and calls MyTouch, but
  Underhell items never register it: `CUHItem::Spawn` clears the touch
  handler (`SetTouch(NULL)`) so walking over an item does nothing.
- `+use` picks the item up: `CUHItem::Use` → `MyTouch`. Consumables
  (food/health) only apply their effect when called with `USE_ON` (the
  inventory `useitem` path from UH_ItemAction); any other use type forwards to
  MyTouch, so world `+use` always takes the item instead of eating/healing it.
- The vanilla CHealthKit / CHealthVial follow the same model (Spawn clears
  touch; Use dispatches on USE_ON).
- Weapons are still vanilla `BumpWeapon` (touch) — TODO: verify Underhell also
  requires +use for weapons (likely, same "no auto-pickup" rule).

## Flashlight batteries (stage 13)

Underhell's flashlight runs on discrete batteries (`m_iUHBatteryCount`), not
suit power:

- `item_battery` (vanilla CItemBattery) grants **1** battery on pickup instead
  of charging suit armour.
- `item_batterypack` (CItemBatteryPack) grants **5** batteries (count TODO —
  verify against original).
- `FlashlightTurnOn` requires a battery; `UH_UpdateFlashlightBattery` (hooked
  into PreThink) drains one battery every `uh_flashlight_battery_time` seconds
  (default 60) while the light is on, and switches it off at zero.
- The original is a full viewmodel system (shoulder flashlight, holster
  animation, `FlashlightViewModelThink`, `item_flashlight`/`item_shoulder-
  flashlight` equipment, `m_bShoulderFlashlight`) — still TODO; this ports the
  core battery mechanic on the vanilla EF_DIMLIGHT flashlight.

## Battery + bleeding HUD (stages 16–17)

From scripts/HudLayout.res / decompile:

- **HudUHBattery** (xpos 8 ypos 200 wide 40 tall 64): contour sprite
  `sprites/hud/hud_battery_contour` (contourx 1 contoury 0 contourwide 24
  contourtall 42), a vertical chunked bar (BarInsetX 6 BarInsetY 31 BarWidth
  14 BarHeight 23 BarChunkHeight 2 BarChunkGap 1, HullColor "2 127 252 192"),
  and the discrete count printed as "x<N>" (sub_100BDC80). The original fades
  the whole gauge when stable and brings it back on battery-count / flashlight
  / nightvision changes (sub_100BDF90). The bar fill reads a 0..100 float
  (client offset 5212 — an Underhell-specific field whose identity is not
  recoverable); our port fills one chunk per battery (count-capped).
- **HudBleeding** (xpos 248 ypos 408 wide 24 tall 36): `sprites/hud/
  hud_blooddrop` tinted red, alpha = m_iBleedCounter * 2.55 (sub_100BE800),
  shown only while bleeding.
- Both use HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT (the
  original sub_100B3790(this, 56) = 0x38).

### Bleeding mechanic (server, decoded from sub_101EF960 PostThink)

- `m_iBleedCounter` (@2188) is the wound severity; `m_iEndurance` (@2184) the
  hunger meter. Runtime floats: `m_flPseudoHealth` (@2144), `m_flPseudoEndurance`
  (@2148), `m_flLastBleedTime` (@2152), previous-tick timestamp (@2156),
  `m_iEHealthCount` (@2160).
- Start: taking damage rolls `uh_bleeding_chance` (5%) and adds to
  `m_iBleedCounter` (exact damage→bleed scaling not recoverable — this port
  adds 1 bleed point per damage point, clamped 100).
- Per think while bleeding: rate = bleedCounter * 0.006, halved if it rounds to
  zero, doubled while sprinting, reduced by endurance * 0.00075. Damage
  accumulates in m_flPseudoHealth and is applied in whole HP (spawning a blood
  drip); each point decrements bleedCounter, and at <=10 the wound closes.
  Bleeding cannot take the last HP (original sends "kill" at health 0).
- Passive hunger decay: endurance drains (0.2 - health * 0.000875) per second,
  applied in whole points — so hunger falls faster at lower health.

## Hermit cards HUD (stage 18)

From scripts/HudLayout.res + decompile:

- **HudUHHermitCards** (xpos r124 ypos 16 wide 128 tall 100): contour sprite
  `sprites/hud/hud_hermitcards` (contourx 34 contoury 10 contourwide 78
  contourtall 54), the collected count printed as "<N>/52" (text1x 78 text1y
  30, HudNumbers font), and — while a quest is active — quest progress
  "   <cur>/<total>" (text2x 10 text2y 30). Paint sub_100BD080.
- Visibility (think sub_100BCFA0): appears when `m_iUHHermitCardsCount` or
  `m_bDisplayHermitCard` changes, fades out ~3 s after the last change. The
  deck icon therefore only appears once the first card is collected.
- Server (sub_102E1B60): the counters are copied from game stats
  `GC_HermitCards` / `GC_HermitQuest_Total` / `GC_HermitQuest_Current` into
  the networked fields; `m_bDisplayHermitCard` flips true once cards exist.
  The Underhell game-stats system is not ported, so the port exposes
  `CHL2_Player::UH_SetHermitCards()` + a `uh_give_hermit_card` cheat instead.

## Glowsticks (stage 19)

Decoded from the original `item_glowstick` Use() (sub_101742D0):

- **Use** (unlit 14–18, from the inventory): create a `prop_physics` with
  `models/pg_props/pg_obj/pg_glow_stick.mdl`, set the colour via bodygroup
  (red/yellow/green/blue/purple = 0/2/4/6/8), place at the player's origin
  + 36 z, add EF_BONEMERGE | EF_NOSHADOW, make it non-solid, then parent it to
  the player and remember it in `m_hActiveGlowStick` (@2164). The model is
  self-illuminated so no separate light entity is needed (matches the
  original, which only creates an `env_flare` for the flare item).
- **Use again** (lit 19–23): the slot has no world entity ("nothing"), so the
  handler removes `m_hActiveGlowStick` and clears the slot.
- **Drop** (unlit 14–18): `UH_SpawnItemInWorld` already drops a lit coloured
  `prop_physics` glowstick.

## Health pickup (stage 20)

`item_healthkit` / `item_healthvial` now **always** stash into the inventory
on +use while a free slot exists (no health gate). Only when the inventory is
full does it fall back to healing on touch. Using the kit from the inventory
heals `sk_healthkit` / `sk_healthvial` and stops / slows bleeding
(sub_102F07A0 / sub_102F08D0).

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
- Slot structure: `Inventory.vtf` is 1024x512. `.res` `PaintBackgroundType 1` + `Texture1` = engine `DrawTexturedBox` — the VTF is **stretched to the live Frame rect**, not drawn 1:1 and not centered. `.res` places the Frame at **(368,84)** size 1024×512 (raw pixels; Frame is not proportional). The metal plate sits in the middle of that canvas ~(174,87)–(849,424); pockets are an **8×4 grid, pitch 84**. 28 game slots fill **row-major** (3×8 + 4) by UV-mapping those texels through `GetWide()`/`GetTall()` so a scaled Frame still keeps icons in the pockets. Hexrays 28/37/44/119 are `GetProportionalScaledValue` leftovers and do not land on this VTF. LMB/RMB Use/Drop menu as before.
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

## npc_infected (stage 25)

`CNPC_UH_Infected` (classname `npc_infected`) is a fast feral zombie. This is a
functional port on CAI_BaseNPC + ai_default schedules (the original carries a
16-entry vtable with custom climb/sprint/infection logic that is out of scope
for one pass). Faithfully reproduced from the FGD + string table:

- 8 body variants (inmate/guard/worker/rural/doctor/uniform/office/urban),
  each with a "disable" keyvalue restricting the random pool. Models
  `models/infected/infected_*.mdl`.
- `SpeedModifier` (0..1, blank = random) — parsed; actual run-speed scaling
  waits on the infected's animation ground-speed (TODO).
- `additionalequipment` melee weapon — parsed; the Underhell melee weapons
  (`weapon_melee_pipe` etc.) are not ported yet, so the innate claw attack
  covers melee.
- Random limb loss flag (`m_bInfectedFlag`) — visual limp TODO.
- `OnSpotInfectedBody` output fired on death.
- Intelligence: standard NPC senses + hunt/chase/melee; a custom fast claw
  attack (AE_NPC_ATTACK_BROADCAST -> direct DMG_SLASH + lunge) and a
  climb-touch stub. `SCHED_MELEE_ATTACK1` when in range.

TODO: climb animation, sprint speed, infection spread, gibs, the weapon give.

## Battery items (stage 22)

From the original: `item_battery` uses `models/PG_props/pg_obj/pg_battery.mdl`
and grants **+1** battery; `item_batterypack` grants **+2**; both cap at
**20** (`m_iUHBatteryCount`). Both are +use only (no touch auto-pickup) and
remove themselves on pickup. The flashlight charge is a networked float
`m_flUHBatteryCharge` (0..100) that drains continuously over
`uh_flashlight_battery_time`, giving the HUD a smooth meter (the original
reads the same value from a float at client offset 5212, inside
C_HL2PlayerLocalData — ported as a plain CHL2_Player network var).

## HUD geometry (stage 23)

The Underhell `scripts/HudLayout.res` is a mod asset not present in the SDK,
so the ported HUD panels had no size and their bar/contour offsets were
proportional-scaled (blown up ~2.25x at 1080p). Each panel now pins itself to
the original .res geometry (SetProportional(false) + SetSize + SetPos) and
uses raw-pixel offsets, so the battery/cards/bleeding icons and the
stamina/endurance bars render at their intended size.

## Weapon system (stage 26 — foundation)

Weapon scripts (`scripts/weapon_*.txt`) are complete mod assets carrying all
the Underhell tuning (StaminaToDrain, MeleeRange, PunchPitch/Yaw, ExpOffset
ironsight, ...). The SDK's `weapon_parse` reads them automatically, so the
port only needed to:

- **Extend `FileWeaponInfo_t`** (shared) with the Underhell keys, parsed in
  `FileWeaponInfo_t::Parse` (decode sub_10274870): `OneHanded`,
  `MeleeDelayedFire`/`MeleeRoF`/`MeleeRange`, `StaminaToDrain`,
  `PunchPitch`/`PunchYaw`/`SnapPitch`/`SnapYaw` ("min, max" ranges),
  `CrouchRecoilMult`/`CrouchAccuracyMult`/`RunAccuracyMult`,
  `UH_Weapon_Special`→`Penetration`, and `ExpOffset` (ironsight x/y/z +
  xori/yori/zori + accuracy).
- **21 weapon classes** (classnames from serveror.dll RTTI + Weapon List.txt):
  melee axe/baton/pipe/wrench/cleaver → `CBaseHLBludgeonWeapon`; pistols
  (glock/beretta/socom/python/dualberetta), SMGs (mp5/mp5_eod/mp7), shotguns
  (m3/m5/spas12/xm1014), rifles (g36k/sniper), BFG (mgl/minigun) →
  `CBaseHLCombatWeapon`. Matching thin client stubs in `c_uh_weapons.cpp`.
  Vanilla weapons stay vanilla.
- **impulse 101** gives the full Underhell set (CHL2_Player::CheatImpulseCommands
  case 101 → `UH_GiveAllWeapons`).
- **One weapon per slot**: `Weapon_Equip` ejects the occupied slot's weapon.
- **Melee stamina**: `CUHMeleeWeapon::PrimaryAttack` drains `StaminaToDrain`.

### TODO (weapon stage)

- Per-weapon damage / fire rate / recoil application (currently melee damage
  is a hardcoded per-class value; guns use the base fire path).
- Ironsight (ExpOffset) viewmodel offset + accuracy — client viewmodel work.
- Silencer + laser sight toggles (script data exists; needs networking + viewmodel).
- Free-aim camera (viewmodel lags the crosshair) + dynamic hand switching
  (OneHanded / BuiltRightHanded / cl_righthand flip).
- NPC acttables for the melee weapons (player works, NPCs TODO).

## Weapon system origins (notes/ in the hexrays repo)

The mod author (Mxthe) documented his sources in `notes/`:

- **`ironsight.txt`** — the VDC **"Adding Ironsights"** tutorial (jorg40/Cin).
  The `ExpOffset { x y z xori yori zori accuracy }` block in every weapon
  script is this system verbatim: `CBaseViewModel::CalcViewModelView` slides
  the viewmodel up to the eye by `m_expOffset`/`m_expOriOffset`, interpolated
  by `m_expFactor` over `gMoveTime`. The port parses these into
  `FileWeaponInfo_t::m_expOffset` / `m_expOriOffset` / `m_flAccuracy`.
- **`Over the Shoulder View - Valve Developer Community.html`** — the VDC
  **"Over the Shoulder View"** tutorial. Its "OPTIONAL: Adding free aim"
  section is the "свободная камера" feature: the mouse moves the crosshair on
  screen (deadzone + auto-turn past the edge), decoupling the aim point from
  the view — i.e. the viewmodel no longer tracks the crosshair 1:1.

### One-handed weapons & the flashlight (user-confirmed + decoded)

`OneHanded` exists because Underhell has a **flashlight that is itself a
viewmodel** (`models/weapons/v_flashlight_pg.mdl`, `FlashlightViewModelThink`,
`m_bFlashlightHolstered`, convar `uh_flashlight_anim`). While a one-handed
weapon (pistols, melee) is equipped, the left hand holds the flashlight (or a
**fake flare** — `models/weapons/v_flare_pg.mdl`, `m_bHoldingFlare`). The
player precache (sub_101E25F0) also pulls `v_kick_jake_*.mdl` — the kick
attack viewmodels (distinct from weapons).

### First-person hand/glove skin (map-driven, not a cvar)

The hands on the viewmodel (`models/weapons/v_hands.mdl`) change texture via
**entity inputs fired at `!player`**, not a convar:

- `ViewModelSkin <int>` (input func sub_101EEE40) — sets `m_nSkin` (offset 848)
  on the player's two viewmodels; this is what changes the hand/glove texture.
- `SetPlayerSkin <int>` — sets the player model skin.
- `SetPlayerModel <model>` — swaps the player model (default
  `models/player/jake_casual.mdl`; guard/PMC variants exist).
- `SetPlayerKickModel <model>` — swaps the kick-attack viewmodel
  (`models/weapons/v_kick_jake_*.mdl`).

Example from `uh_prologue_2_d.vmf` (PMC): `OnNewGame` fires
`!player,SetPlayerModel,models/player/jake_pmc.mdl` +
`setplayerkickmodel,models/weapons/v_kick_jake_pmc.mdl` +
`Viewmodelskin,3`. In `Uh_House_1_d.vmf` a trigger fires
`!player,setplayerskin,1` and `!player,ViewmodelSkin,0`. There is **no**
`uh_hand*` cheat convar in the original binaries — the player inputs above are
the whole mechanism (the closest cheat is `ent_fire !player viewmodelskin N`).

### Silencer / laser sight (from the string table)

- **Silencer**: player flags `m_bHavePistolSilencer` @3371 /
  `m_bHaveRifleSilencer` @3372 (set via `SetPistolSilencer` /
  `SetRifleSilencer` inputs), client `silencer_toggle` command, viewmodel
  animations `ACT_VM_ATTACH_SILENCER` / `ACT_VM_DETACH_SILENCER`, and a
  `single_shot_silenced` sound key in the weapon scripts.
- **Laser sight**: `sprites/laserpointer.vmt` / `sprites/laserdot.vmt` /
  `sprites/laserbeam.vmt`, `m_bLaserToggleState`, `Valve_Hint_LaserSight`.

### FireMode

`UH_Weapon_Special` also carries `"FireMode"` (only the G36K sets it, value 1
= full auto; matches FIREMODE_FULLAUTO in basehlcombatweapon.h). Parsed as
`FileWeaponInfo_t::m_iFireMode`.

## Ironsight (stage 27)

Port of the VDC "Adding Ironsights" system (jorg40/Cin) that Underhell uses.
Decoded 1:1 from the original toggle (`server/sub_101ECF40`), the viewmodel
send/recv tables (`sub_100F8EA0` / `sub_10015160`) and the player sendtable
(`sub_101E6C70`).

State (all networked, mirrors the original binary):

- `CBaseViewModel::m_bExpSighted` — networked bool (server @1120, client
  @1960) that the server toggles; the client `CalcViewModelView` reads it to
  slide the viewmodel up to the eye by the weapon's `ExpOffset`
  (`m_expFactor` interpolates 0 hip → 1 sighted over ~0.1 s).
- `CHL2_Player::m_bIronSighted` — authoritative flag (accuracy + HUD).
- `CHL2_Player::m_fIronsightedTime` — last toggle time (0.1 s debounce).

Server toggle (`UH_ToggleIronsight`, decode `sub_101ECF40`):

1. Debounce 0.1 s.
2. Preconditions: active weapon exists and is not a melee weapon (weapon-info
   `MeleeWeapon` flag → `m_bMeleeWeapon`, hexrays `sub_100D0E00` reads info
   offset 1832). TODO: the original also gates on a weapon flag @1144 and
   `m_bHardLowered` (each with an `|| m_bIronSighted` escape) — not ported.
3. Drop sprint (`StopSprinting`).
4. `m_Local.m_iHideHUD ^= HIDEHUD_CROSSHAIR` (original toggles bit 256).
5. Toggle the viewmodel `m_bExpSighted` (networked).
6. Enter: `ClearUseEntity()`, `HL2Player.Ironsighton`, FOV =
   `GetDefaultFOV() * uh_ironsight_zoom` (default 0.9 → mild zoom; the real
   sighting effect is the viewmodel slide), `m_bIronSighted = true`, and
   `m_flMaxspeed = uh_ironsight_zoom_focus` (default 40) if lower.
7. Leave: `HL2Player.Ironsightoff`, `m_bIronSighted = false`, FOV = 0
   (default), restore `m_flMaxspeed = hl2_walkspeed` (150) if higher.
8. `m_fIronsightedTime = curtime`.

Key corrections vs. the earlier (wrong) reading of the hexrays:

- **`uh_ironsight_zoom` (0.9) is a FOV *multiplier*, not a transition rate**:
  `FOV = GetDefaultFOV() * uh_ironsight_zoom`, not a subtraction.
- **`uh_ironsight_zoom_focus` (40) is the aim-walk speed, not a FOV value**:
  despite its help text ("subtracted from the default FOV" — stale), the code
  writes it to `m_flMaxspeed` (`sub_100EA7B0` → player offset 4132) and
  restores `hl2_walkspeed` on leave.
- **`m_bExpSighted` is networked on the viewmodel**, not a client-local flag;
  the client reads it directly (single source of truth, no client
  `ironsight_toggle` CON_COMMAND — the string only appears in the resync path
  `sub_100D8E90`). This also keeps the `CBaseViewModel` layout identical in
  both DLLs (the earlier `#if CLIENT_DLL`-guarded members shifted every
  `CNetworkVar` below them and corrupted memory → the crash).

The FOV is set directly (`m_iFOV`/`m_flFOVTime`/`m_iFOVStart`/`m_flFOVRate`),
not via `CBasePlayer::SetFOV`. The original's custom setter `sub_100F8040`
also claims `m_hZoomOwner` (passing the player itself); going through it makes
`IsZooming()` true while only ironsighted and collides with the vanilla suit
zoom. A direct set gives the same `GetFOV()` result without touching
`m_hZoomOwner` — intentional, documented divergence.

`m_bIronSighted` / `m_fIronsightedTime` are networked (accuracy + FOV follow
the authoritative server state).

### Weapon fire tuning (stage 28)

`CUHGunWeapon` now reads the parsed weapon-script stats in the fire path:

- **Fire rate** (`GetFireRate`): the Underhell scripts carry no cycle-time key,
  so each class gets a value recovered from serveror.dll (see stage 30).
- **Recoil** (`AddViewKick`): `PunchPitch`/`PunchYaw` ranges, scaled by
  `CrouchRecoilMult` while ducked.
- **Spread** (`GetBulletSpread`): a base cone lerped by an accumulated spam
  penalty, scaled by `CrouchAccuracyMult` (duck), `RunAccuracyMult` (moving)
  and the `ExpOffset` accuracy while ironsighted (`m_bIronSighted`).
- **Ironsight desync**: switching weapons while sighted now un-sights
  (`Weapon_Switch` calls `UH_DisableIronsight`).

### Weapon damage + fire rate (stage 30)

The weapon scripts carry no damage / fire-rate keys — those are baked into the
C++ classes. Recovered from the original:

- **Damage** lives in `cfg/skill.cfg` as `sk_plr_dmg_<weapon>` (the original
  registers each as a `ConVar` default "0" and the engine's skill.cfg sets the
  real value; it also reads them through a per-weapon `GetDamage()` vtable
  override — hexrays slot 213). Values (baked into the constructors here):

  | Weapon | Damage | | Weapon | Damage |
  |---|---|---|---|---|
  | axe / baton / pipe / wrench / cleaver | 35 / 13 / 15 / 25 / 50 | | glock / beretta / socom / python / dualies | 10 / 15 / 20 / 120 / 20 |
  | mp5 / mp5_eod / mp7 | 12 / 10 / 8 | | m3 / m5 / spas12 / xm1014 | 12 / 16 / 14 / 12 |
  | g36k / sniper | 20 / 80 | | bfg_mgl / bfg_minigun | 200 / 50 |

- **Fire rate** (extracted from serveror.dll vtable + disassembly):

  | Weapon | Rate | Source |
  |---|---|---|
  | pistols (glock/beretta/socom/python/dualies) | 0.2 s | shared fire routine `sub_1027AEC0` |
  | SMG mp5 / mp5_eod / mp7 | 0.075 s | `GetFireRate` vtable slot 277 → `sub_102801F0` (`flds 0x105300E4`) |
  | BFG minigun | 0.075 s | same `GetFireRate` (`sub_102801F0`) |
  | G36K | 0.1 s | select-fire `GetFireRate` `sub_103F5150` (`flds 0x1048775C`) |
  | shotgun m3/m5/spas12/xm1014, sniper, BFG mgl | ~0.75 / ~1.5 / ~1.0 s | custom pump/delay fire paths — TODO exact |

- **Note**: the vanilla base `CBaseCombatWeapon::PrimaryAttack` never sets
  `info.m_iDamage`, so the previous port fired with the ammo-definition damage
  instead of the Underhell value. `CUHGunWeapon::PrimaryAttack` now builds the
  shot itself with `m_iDamage = GetDamage()`.

### Silencer + laser sight (stage 31)

Decoded from `sub_101E2F50` (toggle) + `sub_101F11D0` (ClientCommand dispatch)
+ the player sendtable (`sub_101F2D30`).

- **Silencer**: `m_bSilenced` on the gun, `m_bHavePistolSilencer` /
  `m_bHaveRifleSilencer` (networked) on the player. `silencer_toggle` gates
  pistols (`GetWeaponType() == 1`) and rifles (`== 4`) on owning the matching
  silencer; other weapons toggle freely. `SetPistolSilencer` /
  `SetRifleSilencer` inputs grant the silencer. The original's SoundData adds
  `single_shot_silenced` at index 2 (`SINGLE_SILENCED`), used when silenced.
- **Laser sight**: `m_bLaserToggleState` (networked) + `UH_ToggleLaser` +
  `laser_toggle` command. Client `uh_laser.cpp` draws a per-frame beam + impact
  dot from the eye along aim (`sprites/laserbeam.vmt` + `sprites/laserdot.vmt`).
- **DropWeapon**: `UH_DropWeapon` (command) — un-sight, refuse in vehicle /
  sprinting / melee, toss the active weapon forward at 300; `m_bDisableWeaponDrop`
  + `DisableDropWeapon` / `EnableDropWeapon` inputs.
- **Throw_Nade**: `UH_ThrowNade` (command) — un-sight then start the
  `weapon_frag` throw (arm-deploy flare branch TODO).

### TODO (ironsight / weapons)

- Free-aim (over-the-shoulder) camera still TODO — first-person only, and the
  original gates it behind the one third-person location.
- Shotgun multi-pellet fire (skill.cfg `sk_plr_num_shotgun_pellets = 12`):
  the port fires a single shot per trigger pull; the original fires 12 pellets.
- Exact fire rate for SMG / shotgun / sniper / BFG (recover from each weapon's
  fire function in serveror.dll).
- Silenced viewmodel activity (`ACT_VM_*_SILENCED`) + ATTACH/DETACH_SILENCER
  animations — the port only switches the sound.
- Client laser beam/dot rendering.

## Remaining original parameters (stage 32 — inventory of what's left)

Full list of original `CHL2_Player` state + commands not yet ported (from the
sendtable `sub_101F2D30` / ClientCommand `sub_101F11D0`):

| Feature | State | Command / input | Status |
|---|---|---|---|
| Night vision | `m_bHaveNightVision` @2138, `m_bNightVisionOn` @3369, `m_bNightVisionEnabled` @2140 | `NightVision_Toggle`, `SetNightVision` | TODO |
| Gas mask | `m_bHaveGasMask` @2139, `m_bGasMaskOn` @3370, `m_bGasMaskEnabled` @2141 | `GasMask_Toggle`, `SetGasMask` | TODO |
| Shoulder flashlight | `m_bShoulderFlashlight` @5040 | — | networked, viewmodel TODO |
| Flashlight viewmodel | `m_bFlashlightHolstered` | `v_flashlight_pg.mdl` | TODO |
| Fake flare | `m_bHoldingFlare` @2122, `m_bFlareHitting` @2124, `m_bFlareMarker` @2125 | `v_flare_pg.mdl` | TODO |
| Kick | `m_bKickMarker` @4185 | `uh_jake_kick`, `v_kick_jake_*.mdl` | TODO |
| Weapon drop | `m_bDisableWeaponDrop` @2136 | `DropWeapon`, `InputDisableDropWeapon` / `InputEnableDropWeapon` | TODO |
| Grenade throw | — | `Throw_Nade` | TODO |
| Fire-mode select | `m_iFireMode` (script `FireMode`) | `firemode_toggle` | TODO (G36K select-fire) |
| Left arm | `m_bLeftArmDeployed` @2121 | — | TODO |
| Hermit cards | `m_iUHHermitCardsCount` @5048, quest counters | — | partial (`uh_give_hermit_card` cheat) |

Also still TODO from earlier stages: gas-mask check in food `Use()`, melee
swing stamina gate, full bleeding→damage scaling, exact held-item handle @2164.

## Message entities + mirror reflection (env_message/env_hudhint/env_global + player-in-mirror)

Decoded from the "New Message Entities Inputs" and "Player Model and Mirror
Reflections" tutorials + the original datamaps (CMessage sub_101387F0,
CEnvHudHint sub_10137AD0) and input handlers (sub_101EEE40 = ViewModelSkin).

- **env_message** (`CMessage`): `InputMessage` shows the parameter directly
  (does NOT store). `SetMessage` is the highest priority (17); `SetMessagePriorityN`
  (1..16) store into a slot, higher N wins; `RemoveMessagePriority` takes the
  priority number (1..16) and falls back to the next-highest; `ShowMessage`
  displays the active one. `GlobalEnvMessageIndex` (0-7) persists the active
  priority between maps via the global-state counter `uh_envmessage_<idx>`
  (TODO: original stores the message "as player state" — verify string survives).
- **env_hudhint** (`CEnvHudHint`): `InputHint` / `InputHintThroughParameter`
  (FIELD_STRING) show the hint from the parameter.
- **env_global** (`CEnvGlobal`): `SetGlobalOn` / `SetGlobalOff` / `SetGlobalDead`
  affect the global NAMED in the parameter (not the entity's own keyvalue), so
  one env_global can drive many shared globals.

### Player in mirrors (mirror/monitor-only rendering)

- `CBaseEntity::m_bIsMirrorOnly` (networked bool, keyvalue `uh_rendermirrorsonly`).
  Server sendtable + client recvtable (after `m_bAlternateSorting`).
- Client convar `cl_player_render_mirror` (FCVAR_CHEAT, default 1).
- Client render hook: `g_bRenderingReflectiveGlass` is set around
  `CReflectiveGlassView::Draw()` / `CRefractiveGlassView::Draw()`;
  `C_BaseEntity::ShouldDraw()` rejects `m_bIsMirrorOnly` entities outside that
  pass; `C_BasePlayer::ShouldDraw()` lets a mirror-only local player draw there
  even in first person.
- Player inputs `SetPlayerModel` (sets model + flags mirror-only),
  `SetPlayerSkin`, `ViewModelSkin` (viewmodel hand/glove skin), and
  `SetPlayerKickModel` (kick viewmodel, TODO full kick) — see
  `game/server/underhell/uh_player_model.cpp`.
