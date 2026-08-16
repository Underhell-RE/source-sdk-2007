# Current branch implementation status

This document records the actual state of `arena/01a007ab-source-sdk-2007` after the Arena work series beginning at `2c9f546`. It deliberately distinguishes decoded/implemented behavior from partial reconstructions and remaining work. It is not a claim that every system below is 1:1 verified in-game.

## Build and integration note

The project is an Orange Box-era Windows/MSVC Source SDK project. No Windows client/server DLL build has been run in the Arena sandbox. A **clean rebuild of both `client.dll` and `server.dll`** is required after pulling changes, especially after server entity/datamap/viewmodel changes.

The server project file updated by this branch is `game/server/server_episodic-2005.vcproj`.

---

## Changes by subsystem

### First-person free aim

**Files:** `game/client/in_mouse.cpp`, `game/client/underhell/uh_freeaim.h`, `game/shared/baseviewmodel_shared.cpp`

Implemented from Cliento.dll `sub_100D7980` and `sub_10014D80`:

- Registers original `cam_ots_freeaim_*` convars and defaults.
- Retains normalized free-aim cursor state from scaled mouse input:
  - `yaw * mouse_x / 90`
  - `pitch * mouse_y / 90`
  - capped by `cam_ots_freeaim_move_max` (default `0.1`).
- Normal camera mouse input is still passed to `ApplyMouse`.
- Unsighted viewmodel obtains a screen ray at `(cursor * 0.25 + 0.5) * screen_size`, using `ScreenToWorld`, then uses its angles for the viewmodel.
- Effect is skipped while ironsighted.

**Scope note:** this is the verified first-person viewmodel visual path. The full OTS angle-separation and original `update_freeaim` server target synchronisation are not ported.

---

### Ragdoll/corpse dismemberment and carrying

**Files:** `game/server/underhell/uh_ai.cpp`, `game/server/physics_prop_ragdoll.{h,cpp}`

Implemented or corrected:

- Corpse `+use` has `FCAP_IMPULSE_USE` and routes to the stock `CPlayerPickupController` through `CHL2_Player::PickupObject( body, false )`; the normal prop mass limit is bypassed for ragdolls.
- Ragdolls remain physgun-pickable through `VPhysicsIsFlesh() == false`.
- Drag blood trail is updated every `0.25 s`, only after movement greater than `4` units in X/Y, matching decoded `DraggedThink` conditions.
- Ragdoll hitgroup recovery now checks the struck physics element and its parent chain instead of relying on a one-to-one physicsbone/hitbox-bone match.
- Limb constraint selection walks up same-hitgroup ragdoll parents before severing, avoiding a forearm/calf-only constraint break.
- Repeated bodypart removal is guarded by bodygroup state to prevent duplicate gibs/blood bursts.
- Head destruction does not break a ragdoll constraint; it uses bodygroup/gear removal behavior.
- `combine_soldier_prisonguard` uses destroyed head bodygroup `1`; standard combine handling uses high destroyed-head variants (`10`/`11`) based on current head variant.

**Remaining / requires dedicated final audit:** complete `CNPC_CombineS` original head/helmet/respirator/gasmask state machine from `sub_10031BF0`; exact normal/prison/PMC/heavy-leg bodygroup transitions; exact particle/attachment and corpse state transfer. The current implementation is improved but should not be called fully 1:1 until this audit is completed.

---

### Flashlight / left hand

**Files:** `game/server/underhell/uh_leftarm.cpp`, `game/server/hl2/hl2_player.h`

- Hand flashlight uses `models/weapons/v_flashlight_pg.mdl`.
- The deploy transition sends viewmodel sequence `1`.
- The holster transition sends sequence `2` and keeps the model visible through a delayed `UH_FlashlightViewModelThink` cleanup instead of hiding it immediately.
- `m_bFlashlightHolstered` is now used as transition state.
- The delayed flashlight animation uses a dedicated context name rather than colliding with grenade `FlashLightContext` handling.

**Remaining:** shoulder flashlight and exact light rendering are still separate work; vanilla `EF_DIMLIGHT` remains in the flashlight core path.

---

### Inventory items

#### Bandages

**File:** `game/server/underhell/uh_items.cpp`

- World pickup no longer rejects bandages because player health is full.
- World pickup no longer pre-rejects full inventory; `UH_GiveItem` owns the existing full-inventory fallback.
- The hurt/bleeding check remains in the consumption path so an inventory bandage is not consumed without an effect.

#### Glowsticks

**Files:** `game/server/underhell/uh_items.cpp`, `game/server/underhell/uh_player_inventory.cpp`, `game/shared/underhell/uh_inventory.{h,cpp}`

- Glowstick pack skin follows decoded `sub_101741C0`: half remain skin `0`; the other half select a random `0..4` skin.
- Inventory color ID now derives from the world pickup's `m_nSkin`, rather than a second unrelated random roll.
- Removed the explicit parent-to-player waist/belt render path for the active lit glowstick.
- Lit active prop is owned by the player and gets the decoded `360` second lifetime.

**Remaining:** exact original lit-glowstick physics/light/effect lifecycle needs a dedicated audit. `EF_BRIGHTLIGHT` tint behavior is not a validated substitute for original light creation.

---

### Weapon system

**Files:** `game/server/underhell/uh_weapons.{h,cpp}`

#### Fire modes

- Underhell weapon script `UH_Weapon_Special -> FireMode` is applied on first actual attack after weapon data is available.
- This replaces the former behavior where all thin gun classes were initialized as full-auto, causing script-semi pistols such as SOCOM/Beretta/Glock to fire continuously while holding attack.

#### Shotgun pump

- Shotguns (`m_iShotsPerFire > 1`) now set a pending pump at `0.4 s` after firing.
- `ItemPostFrame()` plays `SPECIAL1` and `ACT_SHOTGUN_PUMP`, including while attack remains held.
- Existing refire value remains `0.8 s`.

#### Melee stamina

- Reconstructed `sub_102B0B50` entry gate: melee attack is denied below `15` suit-power stamina.
- A successful swing drains script `StaminaToDrain`.
- Uses `Player.Voice.Melee` at normal stamina and `Player.Voice.Melee.Exhausted` below `35` stamina.

**Remaining:** exact per-weapon fire/reload/bolt/pump timings, original damage and penetration tables, SOCOM-only laser weapon gating, and complete vanilla weapon integration require further dedicated reverse engineering.

---

### Kick (`uh_jake_kick`)

**File:** `game/server/underhell/uh_kick.cpp`

Compared against `sub_101F0050`, `sub_101E5A60`, `sub_101F11D0`, `sub_101F2990`:

- 72-unit primary trace retained.
- Added fallback hull trace after line miss, using the decoded short reach `72 - 55.424` and front-facing dot gate `0.70721`.
- Force now uses `CalculateMeleeDamageForce` with `uh_kick_forcemult`; removed fixed forward `300` force approximation.
- Rumble behavior is split into strike `(9,0,4)` and actual-hit `(4,0,4)` pulses.
- Existing damage, `DMG_CLUB`, kick viewmodel, 20 stamina gate/cost, windup/recovery and `OnKicked` output remain.

Doors may respond either through damage (if damageable) or their map `OnKicked` output route. A global forced-break rule was intentionally not added because it would diverge from map-controlled behavior.

---

### Mirror-only player model

**File:** `game/server/underhell/uh_player_model.cpp`

`SetPlayerModel` now:

1. assigns the model;
2. resolves a valid `ACT_IDLE` sequence;
3. calls `ResetSequence`, resets cycle, calls `ResetSequenceInfo`;
4. starts `PLAYER_IDLE` state;
5. sets mirror-only render state.

This prevents retaining an invalid sequence index from the prior model, which produced a bind-pose/T-pose in reflective glass.

---

### HUD

**File:** `game/client/underhell/hud_hermitcards.cpp`

- Hermit card HUD visibility now follows decoded binary timing:
  - alpha `255` for three seconds after card/display state change;
  - alpha `0` afterward.
- Removed prior `alpha -= 1` gradual fade behavior.

The existing battery, bleeding, stamina, endurance and dot-reticle implementations have previously been compared in `docs/UNDERHELL.md` against their decoded HUD functions. This branch only changed Hermit-card timing in the HUD layer.

---

### Bullet time

**Files:** `game/server/underhell/uh_bullettime.{h,cpp}`, `game/server/underhell/uh_weapons.cpp`, `game/server/hl2/hl2_player.{h,cpp}`

Implemented core:

- ConVars:
  - `bt_enabled`
  - `bt_timescale` (`0.3`)
  - `bt_enemybulletspeed` (`500`)
  - `bt_playerbulletspeed` (`2000`)
  - `bt_plr_speed` (`250`)
- `bt_enabled` callback applies `host_timescale`, player speed, screen overlay and start/end sound routing.
- `uh_bullet` entity selects decoded bullet models (`bt_9mm`, `bt_357`, `bt_762`, pellet), updates velocity every `0.05 s`, and applies decoded slow-motion speed formula.
- Underhell gun fire calls `UH_BulletTimeSpawnTracer` after standard `FireBullets`, preserving existing hit-scan damage.
- `impulse 110` toggles bullet time.
- Player map inputs `EnableBt` and `DisableBt` are registered and route through the shared helper.
- `UH_BulletTimePlayerDied()` exists for death-hook integration.

**Important current limitation:** this is a tracer/core implementation, not yet the full original modified `sub_100EAFB0` standard FireBullets extension. It is currently connected to Underhell gun classes; vanilla weapons are not covered. Full original bullet-time requires integrating its post-trace/per-pellet/penetration/anti-recursion branch into the common FireBullets path, plus death hook and exact audio pitch handling.

---

### NPC Ace

**Files:** `game/server/underhell/npc_ace.cpp`, `game/server/server_episodic-2005.vcproj`

Added `npc_ace` base implementation:

- derived from `CNPC_CombineS`;
- model default `models/combine_soldier_assassin.mdl`;
- decoded health/kick/speed/landing/cloak ConVars;
- cloak state and cloak inputs;
- `CloakNow`, `UnCloakNow`, `DisableCloak`, `EnableCloak` inputs;
- ACE sentence branches for pain/player death/man down;
- base combat behavior inherited from combine soldier.

**Remaining:** exact full Ace vtable/schedule/assault/grenade/landing/cloak state machine remains to be ported. The current entity is a working base, not a complete 1:1 recreation.

---

### NPC Butcher

**Files:** `game/server/underhell/npc_butcher.cpp`, `game/server/server_episodic-2005.vcproj`

Added `npc_butcher` base implementation:

- default model `models/butcher.mdl`;
- original public butcher ConVars;
- butcher sound precache and sound methods;
- `EnableCharge`, `DisableCharge`, `SetCowerOn`, `SetCowerOff`, `ChargeEntity` inputs;
- charge target range, impulse, club damage, force and cooldown behavior.

**Remaining:** original butcher custom AI schedule/task/condition suite (`TASK_BUTCHER_*`, `SCHED_BUTCHER_*`, physics-object attack, custom charge collision and door handling) is not yet fully ported.

---

## Commit trail

| Commit | Summary |
|---|---|
| `8b711c6` | Free-aim viewmodel tilt |
| `5b19674`, `5ae030f` | Corpse limb/head state fixes |
| `5d54bd3`, `5b35a6c` | Corpse pickup/flashlight transitions and ObjectCaps compile fix |
| `41a53d2`, `59990f8` | Bandage pickup, shotgun pump, parsed weapon fire modes |
| `48417c4` | Glowstick skin/active prop changes |
| `aa2b152` | Ace NPC base |
| `53c6546` | Hermit card HUD visibility timing |
| `42b7d12` | Melee stamina gate |
| `8388e3e` | Kick trace/force behavior |
| `3b7a674` | Mirror player animation initialization |
| `c1b7e2c`, `7e592a1`, `9800ce5` | Bullet-time core, compile fix, impulse/map inputs |
| `5ae0121` | Butcher NPC base |
