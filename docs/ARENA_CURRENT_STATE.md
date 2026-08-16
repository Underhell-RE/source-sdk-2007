# Arena session implementation ledger — `arena/01a007ab-source-sdk-2007`

> ## MANDATORY VERIFICATION WARNING
> **This document and the changes it describes were produced by an automated LLM during an Arena session — in the user's words, "the dumbest LLM". Nothing in this file is proof of correctness, completeness, buildability, or 1:1 Underhell parity. Every entry requires source review, a clean client/server rebuild, and in-game comparison with the original before merge into `master`.**
>
> This warning is intentionally prominent because the session included speculative implementations, several follow-up corrections, and two reverted false interpretations. Treat this document as an audit ledger, not release notes.

## Scope and baseline

- Branch: `arena/01a007ab-source-sdk-2007`
- Baseline: `2c9f546ce0fb42c4b646e10a5ec71d0374c9bb16`
- Original server reference used: `Underhell/bin/diaphora/servero_diaphora.dll.c`
- The Windows Source SDK 2007/Orange Box game DLLs were **not built in this Linux Arena sandbox**.
- The branch requires a clean rebuild of both `client.dll` and `server.dll`.

## Explicit non-claims

The following are **not** complete or certified 1:1:

- `CNPC_CombineS` corpse/dismemberment state machine;
- all custom items and inventory use/drop behavior;
- `npc_infected`, Ace, Butcher, combine/citizen AI;
- SOCOM laser lifecycle;
- bullet time `CBullet` and common `FireBullets` integration;
- NPC-driven jeep entry/seat behavior;
- shader installation/runtime behavior for gasmask/night vision.

Several changes below improve a concrete discrepancy, but an improvement must not be misreported as a finished subsystem.

---

# Implemented/edited subsystems

## 1. Corpse, ragdoll, dismemberment, body dragging

**Primary files**

- `game/server/underhell/uh_ai.cpp`
- `game/server/physics_prop_ragdoll.{h,cpp}`
- `game/server/underhell/uh_carry_ragdoll.cpp`
- `game/server/hl2/hl2_player.{h,cpp}`
- `game/server/hl2/npc_combines.cpp`

### Added and changed

- Corpse `+use` support and pickup through stock `CPlayerPickupController`:
  - `FCAP_IMPULSE_USE` capability;
  - `CRagdollProp::Use` calls `CHL2_Player::PickupObject( body, false )`;
  - normal mass limit is bypassed for corpse dragging.
- Ragdolls are not treated as flesh by physgun logic (`VPhysicsIsFlesh() == false`).
- Drag blood follows decoded `DraggedThink` cadence:
  - update cadence: `0.25 s`;
  - decal only after >4 units of X/Y body motion;
  - stationary held bodies no longer continuously paint blood.
- Carrying body weight state was added:
  - player records `m_hCarryingRagdoll` and saved speed;
  - bodies above mass 10 reduce speed to one third;
  - state is restored when the pickup controller no longer holds the corpse.
  - **Not complete:** client mouse/sensitivity damping from `uh_bodymousedamper` is still absent.
- Ragdoll hitgroup recovery walks ragdoll physics parent elements to map physics bones back to hitgroups.
- Dismemberment tracks head/arm/leg accumulated damage, helmet damage, and prevents duplicate transitions through bodygroup state.
- Corrected bodygroup lookup to tolerate `arms`/`Arms`, `legs`/`Legs`, etc.
- Corrected combine arm loss encoding:
  - regular combine soldier: authored `0/2/4/6` arm states;
  - four-state variants: `0/1/2/3` states.
- Heavy leg bodypart variants use `leftleg2/rightleg2` for combine heavy-leg bodygroup values.
- Added/preloaded severed bodypart models for standard combine, prison guard, PMC and heavy-leg variants.
- Detached pieces now use `CreateServerRagdollSubmodel`, rather than an independently spawned reference-pose `prop_ragdoll`.
- Detached limbs spawn at authored sever attachments (`ForeArm_L/R`, `Calf_L/R`) and retain source skin; arm pieces copy `Glove_L/Glove_R` state.
- Detached limbs inherit source ragdoll linear/angular velocity to reduce unstable first-frame launches.
- Source ragdoll constraints are **not** deliberately broken during limb removal; the source mesh is bodygroup-hidden and a separate sub-ragdoll is created.
- Original-style effects added/adjusted:
  - `Player.Splat` on actual sever;
  - `Player.HeadShot` plus `UTIL_BloodSpray` for head destruction;
  - `blood_zombie_split_spray` for arms;
  - `blood_advisor_puncture_withdraw` for legs;
  - body decals cleared on successful sever.
- Gear/helmet handling was expanded:
  - eye attachment is used for dropped head gear;
  - model-specific helmet item selection: guard, prison, PMC, worker;
  - added missing `item_gasmask_prison` entity registration;
  - prison corpses use `item_gasmask_prison`, others use guard gasmask where applicable.

### Important correction/revert

Commit `7200b4b` incorrectly made `+use` strip a helmet from a corpse. It was explicitly reverted by `0fc4916`. Current intended behavior is: **helmet removal is by shooting/damage, not by `+use`**.

### Still missing / high-risk

- Full `CNPC_CombineS` `sub_10031BF0` behavior is not ported. In particular, original living-NPC weapon/animation/schedule changes after arm/leg loss, all family-specific state transitions, and exact bodypart attachment/constraint behavior still need exact work.
- Current limb physics was adjusted repeatedly and needs in-game testing. The user reported legs flying in a comedic/unphysical manner; the latest velocity inheritance patch has not been runtime verified.
- Infected corpse state and gib transfer are not complete.
- Ragdoll lifetime/LRU and all original client visual paths have not been fully audited.

---

## 2. Bullet time

**Files**

- `game/server/underhell/uh_bullettime.{h,cpp}`
- `game/server/underhell/uh_weapons.cpp`
- `game/server/hl2/hl2_player.{h,cpp}`

### Added

- ConVars added/used:
  - `bt_enabled` default `0`, cheat/replicated;
  - `bt_timescale` default `0.3`;
  - `bt_enemybulletspeed` default `500`;
  - `bt_playerbulletspeed` default `2000`;
  - `bt_plr_speed` default `250`.
- `bt_enabled` callback changes `host_timescale`, player max speed, overlay and start/end sound playback.
- `impulse 110` toggles BT.
- `EnableBt`/`DisableBt` player map inputs were added.
- BT sound scripts are precached (`Player.bullettimestart`, loop/end).
- Visible bullet model selection follows decompiled ammo IDs:
  - IDs 3/4: `bt_9mm`;
  - ID 5: `bt_357`;
  - ID 7: pellet;
  - otherwise `bt_762`.
- Underhell gun fire currently performs normal hit-scan `FireBullets` and then creates visible BT bullet props.
- Current visible props are registered in `CUHBulletMotionSystem`, which reapplies velocity every `0.05` game seconds, matching the cadence of original `CBullet::Think` (`sub_101078D0`):
  - BT speed = enemy/player speed cvar × `bt_timescale`;
  - non-BT speed = `2500`.
- Artificial 120-second visual-bullet removal was removed.

### Still missing / high-risk

- This is **not** original `CBullet`.
- Current visible bullets are stock replicated `prop_physics`, not original networked `CBullet` server/client classes.
- Common original `FireBullets` extension (`sub_100EAFB0`) is not ported: per-pellet post-trace behavior, collision/damage/penetration sequencing and anti-recursion flag remain absent.
- Vanilla weapon fire and enemy weapon fire are not comprehensively integrated.
- The user reported BT duration/behavior as too short. The latest velocity lifecycle change addresses visible bullet freezing, but BT duration and all runtime behavior require testing against original.

---

## 3. NPC-driven jeep and mounted gun

**Files**

- `game/server/hl2/vehicle_jeep.{h,cpp}`
- `game/server/player.cpp`
- `game/server/vehicle_base.cpp`
- `game/server/hl2/npc_playercompanion.{h,cpp}`
- `game/server/npc_vehicledriver.{h,cpp}`
- `game/shared/vehicle_viewblend_shared.cpp`

### Added/changed

- NPC companion driver bridge was added for `EnterVehicleImmediatelyAsDriver` / `EnterVehicleAsDriver` flows using a hidden `npc_vehicledriver`.
- Player vehicle entry gates were loosened for NPC-driven driveable vehicles and gunner use.
- Player-at-mounted-gun state is tracked (`m_bPlayerAtGun`).
- Gunner camera attempts to use `vehicle_gunner_eyes`.
- Mounted jeep fire was recently reworked against server Diaphora:
  - removed custom extra full-distance `UTIL_Tracer`, which created a duplicate through-world beam;
  - restored jeep `m_nAmmoType` instead of forcing AR2 ammo type;
  - restores `sk_jeep_gauss_damage` through SDK 2007 `FireBulletsInfo_t::m_iDamage`;
  - fire interval set to `0.1`;
  - spread set to decoded `0.0087299999` vector;
  - precaches and dispatches `muzzle_star_uh` on attachment `muzzle_uh`.
- A compile correction changed invalid `FireBulletsInfo_t::m_flDamage` to `m_iDamage`.

### Still missing / high-risk

- User reported inability to enter/sit in the jeep. This is not confirmed solved.
- The current jeep changes went through a bad intermediate edit that accidentally removed charged-cannon code; it was restored before the final state, but a clean game DLL build is mandatory.
- Exact original seat role, `Use`, `EnterVehicle`, map lock and NPC driver behavior still need runtime validation on `Uh_Chapter1_16_d`.
- The chapter map’s rail/stuck-path issue is not solved.

---

## 4. SOCOM laser

**Files**

- `game/server/underhell/uh_weapons.{h,cpp}`
- `game/server/hl2/weapon_rpg.{h,cpp}`
- `game/server/hl2/hl2_player.{h,cpp}`
- `game/client/underhell/uh_laser.cpp`

### Added/changed

- SOCOM secondary attack toggles a real `env_laserdot` path rather than a fake client beam.
- Server helper for laser-dot position updates was added to RPG laser-dot support.
- The dot is traced/updated during weapon frame handling; holstering removes it.
- Player laser state uses public `CHL2_Player` accessors after a private-member compile failure.
- Player laser behavior was restricted to SOCOM paths.

### Still missing / high-risk

- User reported bad laser behavior during reload/weapon switching. No runtime verification proves this is solved.
- Original SOCOM lifecycle (`sub_1027B9E0`, cleanup/update/create functions) remains only partially reconstructed.

---

## 5. Weapons, melee, kick and weapon scripts

**Files**

- `game/server/underhell/uh_weapons.{h,cpp}`
- `game/shared/weapon_parse.{h,cpp}`
- `game/server/underhell/uh_kick.cpp`

### Added/changed

- Weapon parse support includes `MeleeDelayedFire`, `MeleeRoF`, `MeleeRange`, `StaminaToDrain`, `PunchPitch`, `PunchYaw`, `UH_Weapon_Special`/fire mode.
- Script fire mode is applied after weapon data initialization.
- Underhell melee primary gate uses 15 stamina; attack drains configured stamina and chooses normal/exhausted voice based on 35 stamina.
- Unsupported vanilla melee secondary swing is disabled for Underhell melee weapons.
- Shotguns got pump scheduling/animation behavior.
- `uh_jake_kick` was changed toward decoded reach/hull fallback, front-facing gate, melee force and rumble behavior.
- Melee voice sound scripts were precached to fix reported runtime precache errors.

### Still missing / high-risk

- Delayed melee hit execution is not exact: base bludgeon swing is still immediate.
- Most per-weapon timing, recoil, reload, damage, penetration and animation data are estimates unless separately verified.

---

## 6. Custom item/inventory work

**Files**

- `game/server/underhell/uh_items.{h,cpp}`
- `game/server/underhell/uh_player_inventory.cpp`
- `game/shared/underhell/uh_inventory.{h,cpp}`
- `game/client/underhell/c_inventory_panel.cpp`

### Added/changed

- Soda map-assigned skin is preserved; random skin is applied only when skin is zero.
- Bandage item use preserves the inventory slot and plays denial when no health/bleed effect is possible.
- Radio/cracker activation was adjusted: delayed initial activation, stable track selection, radio sound insertion, no fake timed cracker explosion, pickup restrictions while active.
- Failed active radio/cracker creation preserves inventory slot.
- Glowstick skin/inventory color and active-prop lifetime were changed toward decoded behavior.
- Added `item_gasmask_prison` due corpse gear drop requirements.
- Heavy armor, apple, banana, soda and basic armor received partial value/skin fixes.

### Still missing / high-risk

- `item_syringe` remains placeholder model/value behavior.
- `item_syringepack` and `item_bandagespack` were identified in original item-random tables but are still not implemented as entities.
- Generic inventory virtual-success/failure/drop/held-object lifecycle is not complete.
- Full item audit remains unfinished.

---

## 7. Free aim, HUD, player presentation and gear overlay

### Free aim

**Files:** `game/client/in_mouse.cpp`, `game/client/underhell/uh_freeaim.h`, `game/client/underhell/hud_dotreticle.*`, `game/shared/baseviewmodel_shared.cpp`, `game/server/hl2/hl2_player.*`

- Free-aim cvars/cursor movement and viewmodel screen-ray tilt were added.
- `update_freeaim x y z` client-to-server route stores a target vector on `CHL2_Player`.
- Dot reticle uses +use edge timing, 3-second visibility, fixed tick geometry and ironsight hide behavior.

**Not complete:** custom weapon shots/muzzle paths do not all consume the server free-aim target, so this is not full original free aim.

### HUD

- Battery disabled-chunk alpha now follows panel fade.
- Hermit cards use binary 255/0 alpha at three seconds instead of gradual fade.
- Battery implementation and reticle still need in-game visual verification.

### Mirror model

- Model swap initializes valid idle sequence/cycle/state to reduce mirror T-pose caused by stale sequence indices.

### Gasmask/night vision

- Client overlay material names follow original `shader/nightvision` and `shader/gasmask` path.
- Likely remaining issue is custom shader DLL/material installation, not confirmed gameplay code.

---

## 8. NPC work

### Infected

- Corrected original internal variant order and FGD-key mapping:
  - inmate 0, worker 1, doctor 2, uniform 3, urban 4, rural 5, guard 6, office 7.
- Corrected some bodygroup branches.
- Removed an incorrect fabricated head-hit call in `OnTakeDamage_Alive`; base `TraceAttack` now remains responsible for actual hitgroup routing.

**Not complete:** schedules, climb, sprint, door/radio interaction, limp behavior, melee, corpse/gib state transfer and original custom conditions/tasks are incomplete.

### Ace

- `npc_ace` base entity, cloak inputs and basic combat/convars were added.
- **Known structural error:** original Ace derives from `CAI_PlayerAlly`; current code derives from `CNPC_CombineS`. This requires rewrite.

### Butcher

- `npc_butcher` base and required abstract zombie virtual methods were added; this fixed a reported abstract-class compiler error.
- Custom full schedule/task/charge/physics behavior is incomplete.

---

# Build/runtime errors fixed during this session

- Butcher abstract `CNPC_BaseZombie` virtual-method compiler errors.
- SOCOM private `m_bLaserToggleState` access compiler error via public accessors.
- Unprecached melee voice script errors.
- Unprecached bullet-time start/end script sound errors.
- Jeep `FireBulletsInfo_t::m_flDamage` compiler error corrected to SDK 2007 `m_iDamage`.

# Reverted mistakes during this session

- `7200b4b` added helmet removal by corpse `+use`; reverted in `0fc4916` because shooting is the intended helmet-removal mechanic.
- Jeep mounted-gun edits briefly removed charged cannon implementation; later commits restored it. The branch must be clean-built and tested, not trusted from history alone.

# Commit index

The full session commit trail is available with:

```bash
git log --oneline 2c9f546..arena/01a007ab-source-sdk-2007
```

Important recent commits:

| Commit | Subject |
|---|---|
| `363eb09` | Restore bullet-time bullet velocity lifecycle |
| `0a982ff` | Fix jeep bullet damage field for SDK 2007 |
| `f96a780` | Implement original mounted jeep gun fire path |
| `7850393` | Inherit corpse motion for detached limbs |
| `f698f4e` | Transfer glove state to detached corpse arms |
| `8922741` | Select and precache combine heavy-leg corpse gibs |
| `e3f1ab9` | Restore prison gasmask corpse drop |
| `0fc4916` | Revert incorrect helmet recovery from corpse use |
| `9b4f1d1` | Jeep gunner seat gate and bullet tracer replication |
| `d4824bb` | Add `update_freeaim` client/server path |
| `e21136b` | Correct infected variant ID mapping |

# Mandatory verification checklist before any real merge

1. Clean rebuild `client.dll` and `server.dll` with the target Source SDK project.
2. Run a compile pass that includes `vehicle_jeep.cpp`, `uh_ai.cpp`, `uh_bullettime.cpp`, all Underhell entities and both game DLL projects.
3. Test corpse shots for each model family: normal combine, prison guard, PMC, worker/infected and heavy-leg variants.
4. Test corpse carry/drop, speed restoration, ragdoll collisions and detached-limb stability.
5. Test `impulse 110`, BT duration, start/end, player/enemy shots, pellet shots, collision behavior and performance.
6. Test `Uh_Chapter1_16_d`: NPC driver starts, player can enter gunner role, camera is correct, gun fires, charged cannon is intact, no duplicate tracer/beam.
7. Test SOCOM laser through reload, holster, weapon switch, death and map transition.
8. Test all item random pool entries and inventory full/denied/use/drop behavior.
9. Compare screenshots/logs against original Underhell, not only decompiler output.
10. Remove or rewrite speculative code before claiming a subsystem is 1:1.
