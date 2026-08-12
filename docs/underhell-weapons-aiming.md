# Underhell — оружейная система, арсенал и система прицеливания

Восстановление функционала поверх **Source SDK Orange Box (2005)** на основе декомпиляции
[`klaxons1/underhell-hexrays`](https://github.com/klaxons1/underhell-hexrays).

**Цель сборки:** Underhell собирался как **Episodic** (`client_episodic-2005.vcproj` /
`server_episodic-2005.vcproj`), поэтому весь код ложится в дерево Episodic, а не HL2MP/SDK.

Документ отвечает на главный вопрос: **какой декомпилированный класс/функция отвечал за оружейную
систему, каждое оружие и прицеливание, и куда это ложится в нашем SDK.**

> Конвенция имён в декомпиляции: Hex-Rays не сохранил символьные имена классов, поэтому функции
> названы по адресам (`sub_XXXXXXXX`), а классы восстанавливаются по строкам RTTI
> (`C_WeaponG36K`, `CBaseHLCombatWeapon` и т.д.) и по дататейблам (`DT_BasePlayer`,
> `DT_BaseViewModel`). Ниже — полная карта соответствий.

---

## 1. Источники данных

| Источник | Что даёт |
|---|---|
| `underhell-hexrays/Underhell/bin/client/` | декомпиляция `client.dll` (18 410 функций) |
| `underhell-hexrays/Underhell/bin/server/` | декомпиляция `server.dll` (26 473 функции) |
| `underhell-hexrays/FGD/Weapon List.txt` | полный список classname оружий |
| `underhell-hexrays/FGD/Item List.txt` | список предметов/боеприпасов |
| `underhell-hexrays/Underhell/scripts/weapon_*.txt` | скрипты данных оружия (WeaponData) |
| `underhell-hexrays/notes/ironsight.txt` | исходный туториал прицеливания (база системы) |
| `underhell-hexrays/FGD/Underhell.fgd` | список игровых энтити |

---

## 2. Карта классов: декомпиляция → наш SDK (Episodic)

### 2.1. Базовые классы оружия

| Декомпилированный класс (RTTI) | Файл-функции (client/server) | Класс в нашем SDK | Файл SDK |
|---|---|---|---|
| `CBaseHLCombatWeapon` | — (базовый, методов много) | `CBaseHLCombatWeapon` | `game/shared/hl2/basehlcombatweapon_shared.h/.cpp` |
| `C_BaseHLCombatWeapon` (клиент) | — | `C_BaseHLCombatWeapon` | тот же файл (`CLIENT_DLL`) |
| `C_BaseHLBludgeonWeapon` | — | `CBaseHLBludgeonWeapon` | `game/shared/hl2/weapon_hl2mpbasebasebludgeon.*` (нет в Episodic — писать аналог) |
| базовый | — | `CBaseCombatWeapon` | `game/shared/basecombatweapon_shared.h/.cpp` |

> В Underhell базовый класс оружия — это HL2-класс `CBaseHLCombatWeapon` (в Episodic он уже есть).
> Именно он расширен полями прицеливания, отдачи, проникания и боевого режима.

### 2.2. Парсинг данных оружия (`weapon_*.txt`)

| Декомпилированная функция | Роль | Класс/функция SDK |
|---|---|---|
| `client/sub_1014DC80` | `FileWeaponInfo_t::Parse()` (клиент) | `CUHWeaponInfo::Parse()` |
| `server/sub_10274870` | `FileWeaponInfo_t::Parse()` (сервер) | тот же `CUHWeaponInfo::Parse()` (shared) |
| `CreateWeaponInfo()` | фабрика структуры | `game/shared/episodic/uh_weapon_parse.cpp` |

Функция `sub_1014DC80` (клиент) и `sub_10274870` (сервер) — это **один и тот же shared-метод
`Parse()`, собранный в обе DLL**. Он читает из `KeyValues` все поля:

```
this+4    bParsedScript (bool)
this+8    PunchPitch      {min,max}   // отдача — тангаж
this+16   PunchYaw        {min,max}   // отдача — рыскание
this+24   SnapPitch       {min,max}   // «рывок» прицела
this+32   SnapYaw         {min,max}
this+40   CrouchRecoilMult     (float)  // множитель отдачи в приседе
this+44   CrouchAccuracyMult   (float)  // множитель точности в приседе
this+48   RunAccuracyMult      (float)  // множитель точности на бегу
this+52   Penetration          (int)    // из UH_Weapon_Special
this+56   ExpOffset.x/y/z      (float)  // смещение прицеливания (ironsight)
this+68   xori/yori/zori       (float)  // доворот прицеливания
this+80   OneHanded            (bool)
this+84   accuracy             (float)  // точность в прицеле
this+88   MeleeDelayedFire     (float)
this+92   MeleeRoF             (float)
this+96   MeleeRange           (float)
this+100  StaminaToDrain       (float)
this+104  szClassName[80]      // ← дальше идут стандартные поля FileWeaponInfo_t
...
this+1832 MeleeWeapon, this+1833 BuiltRightHanded, this+1834 AllowFlipping, this+1876 showusagehint
```

Ключи скрипта, которые это читает (см. `notes/ironsight.txt`, блок «RIFLE G36K»):
`printname`, `viewmodel`, `playermodel`, `anim_prefix`, `bucket`, `bucket_position`,
`clip_size`, `default_clip`, `primary_ammo`, `weight`, `item_flags`, `PunchPitch`, `PunchYaw`,
`SnapPitch`, `SnapYaw`, `CrouchRecoilMult`, `CrouchAccuracyMult`, `RunAccuracyMult`, `ExpOffset`
(`x/y/z/xori/yori/zori/accuracy`), `UH_Weapon_Special` (`Penetration`), `SoundData`,
`TextureData`.

> `FireMode`/`WeaponSpec` из примера-скрипта **не читаются кодом** — в бинаре строки `FireMode`
> нет (проверено по декомпиляции), это мёртвый текст в `.txt`. Поэтому мы их не парсим — 1:1.

### 2.3. Стрельба, точность и отдача

| Декомпилированная функция | Роль |
|---|---|
| `client/sub_100D8E90` | `FireBullets()` — расчёт точности/разброса с учётом прицеливания (`m_bIronSighted`), вызов `ironsight_toggle`, выдача muzzle-эффектов |

Именно здесь применяется значение `accuracy` из `ExpOffset` и читается
`*(player + 4140)` = `m_bIronSighted` (клиент), после чего разброс корректируется.

### 2.4. Система прицеливания (ironsight)

| Декомпилированная функция | Роль |
|---|---|
| `client/sub_10015160` | дататейбл `DT_BaseViewModel` — поле `m_bExpSighted` (offset 1960) |
| `client/sub_10043D70` | дататейбл `DT_BasePlayer` (клиент) — `m_bIronSighted`, `m_fIronsightedTime`, FOV-поля |
| `server/sub_101E6C70` | дататейбл `DT_BasePlayer` (сервер) — те же поля |
| `server/sub_101ECF40` | серверный toggle прицеливания: звуки `HL2Player.Ironsighton/off`, запись `m_bIronSighted`/`m_fIronsightedTime`, FOV |
| `server/sub_100D3FC0`, `server/sub_100D4170` | оружейные обработчики: анимации 212/213, `Silencer`, команда `ironsight_toggle` клиенту |

**База системы** — туториал из `notes/ironsight.txt` (автор Cin/jorg40), который Underhell, судя по
всему, взял и расширил. Суть:

- у вьюмодели появляется `m_bExpSighted` (сетевой) и `m_expFactor` (локальный, интерполяция 0→1);
- `CBaseViewModel::CalcViewModelView()` вызывает `CalcExpWpnOffsets()`, который прибавляет
  `ExpOffset`/`ExpOriOffset` текущего оружия к позиции/углам вьюмодели;
- команда `ironsight_toggle` переключает режим и прячет прицел (`HIDEHUD_CROSSHAIR`);
- **расширение Underhell**: состояние живёт на игроке (`m_bIronSighted`, `m_fIronsightedTime`) и
  синхронизируется по сети; прицеливание меняет FOV через штатную HL2-систему зума
  (`m_iFOV`, `m_iFOVStart`, `m_flFOVTime`, `m_iDefaultFOV`, `m_hZoomOwner`) и корректирует
  разброс (`accuracy`).

### 2.7. Быстрые действия игрока (`dropweapon` / `throw_nade` / `uh_jake_kick`)

| Декомпилированная функция | Роль |
|---|---|
| `server/sub_101F11D0` | `CHL2_Player::ClientCommand` — диспетчер команд `uh_jake_kick`, `ironsight_toggle`, `Throw_Nade`, `DropWeapon`, `NightVision_Toggle`, `GasMask_Toggle`, `silencer_toggle`, `update_freeaim` |

Имена в `kb_act.lst` (клиентские) → фактические команды на сервере:

| kb_act.lst (клиент) | Команда в ClientCommand | Реализация |
|---|---|---|
| `throw_nade` | `Throw_Nade` | `CHL2_Player::ThrowGrenadeQuick()` — быстрый бросок `grenade_frag` (1200 юн/с), если есть аммо «grenade» |
| `dropweapon` | `DropWeapon` | `CHL2_Player::DropActiveWeapon()` — выброс активного оружия вперёд (300 юн/с); **melee-оружие выбросить нельзя** (проверка флага `MeleeWeapon`, offset 1832 в оригинале) |
| `uh_jake_kick` | `uh_jake_kick` | `CHL2_Player::PerformKick()` — удар ногой: viewpunch −2°, звуки `HL2Player.kick_fire(_fly)`, melee-урон (`sk_plr_dmg_kick`, по умолч. 20); в оригинале также тратит 20 выносливости (система выносливости — follow-up) |

Плюс `additionalequipment` (NPC-снаряжение) поддерживает **список через запятую** — NPC берёт
случайное оружие из списка (FGD: `weapon_melee_pipe,weapon_melee_baton,weapon_melee_wrench`). Это
чинило «Attempted to create unknown entity type weapon_smg_mp5,weapon_smg_mp5_eod,…» — ванильный
`CAI_BaseNPC` не умел парсить список и пробовал заспавнить всю строку как один classname.

> Вне оружейного скоупа (отдельные фичи Underhell, будут позже): `cl_inventoryToggle`
> (экран инвентаря, клиент), `NightVision_Toggle` / `GasMask_Toggle` / `silencer_toggle`,
> ConVar `hap_HasDevice` (хаптика HUD), `uh_ragdollcollisiontype`. Спам
> «No caption found for 'metal_box.scraperough'» — безобиден (closecaption для звука скребка).

### 2.6. Проникание пуль (`UH_Weapon_Special.Penetration`)

| Декомпилированная функция | Роль |
|---|---|
| `server/sub_100EAFB0` | `CBaseEntity::FireBullets` — цикл выстрела с поддержкой проникания (88-байтовая копия `FireBulletsInfo_t`, флаг-байт `+81`, продолжение трассировки после попадания) |

Ключевая находка из декомпиляции: в Underhell `FireBulletsInfo_t` расширен, а поле на
смещении `+52` (в ванили `m_iDamage`) используется как **бюджет проникания** — цикл сегмента
(`while (BYTE2(flag) && v132 <= *(info+52))`) продолжает трассировку сквозь поверхность, пока
счётчик не превысит значение из конфига. Значения из оригинальных `weapon_*.txt`:

| Оружие | `Penetration` |
|---|---|
| `weapon_pistol_socom` | 2 |
| `weapon_shotgun_m5` / `spas12` | 5 |
| `weapon_rifle_g36k` | 6 |
| `weapon_pistol_python` / `weapon_rifle_sniper` | 10 |

Семантика: **число поверхностей, которые пуля может пройти насквозь** (0 = выключено). Пуля
останавливается на NPC/игроке; на каждой пробитой поверхности урон падает
(`UH_PENETRATION_DAMAGE_FALLOFF`, 0.5 — точную константу Underhell из Hex-Rays не достать,
вынесена в `uh_basefirearm.cpp`). Реализация: `CUhFirearmWeapon::FireBulletsPenetrating()` —
посегментная трассировка, каждый сегмент уходит в `CBasePlayer::FireBullets` с
`VECTOR_CONE_PRECALCULATED`, поэтому декали/эффекты/урон по аммо-типу полностью штатные.
Парсинг значения — из оригинальных конфигов через `CUHWeaponInfo::m_iPenetration`.

### 2.5. Кастомные поля игрока (из `DT_BasePlayer`)

| Поле (send/recv) | Тип | Назначение | В нашем SDK |
|---|---|---|---|
| `m_bIronSighted` | bool | включён ли прицел | добавить в `CHL2_Player` / `C_BaseHLPlayer` |
| `m_fIronsightedTime` | float | время переключения прицела | добавить туда же |
| `m_iFOV`, `m_iFOVStart`, `m_flFOVTime`, `m_iDefaultFOV`, `m_hZoomOwner` | — | зум (стандарт HL2) | уже есть в `CBasePlayer` (`game/server/player.h`) |
| `m_iEndurance`, `m_iBleedCounter` | int | выносливость/кровотечение | добавить (вне оружейного скоупа, но в той же таблице) |
| `m_bNightVisionOn`, `m_bGasMaskOn`, `m_bLeftArmDeployed`, `m_bHoldingFlare` | bool | снаряжение | добавить (вне скоупа) |

---

## 3. Арсенал: карта оружий

Имена классов — это **клиентские** классы (`C_WeaponXxx`); серверные аналоги отличаются только
префиксом (`CWeaponXxx`). Базовый файл для всех — `basehlcombatweapon` + (для ближнего боя)
аналог `basehlcombatweapon_basebludgeon`.

### 3.1. Оригинальные оружия Underhell (нужно писать заново)

| Класс (клиент) | classname (скрипт) | Группа | SDK-база |
|---|---|---|---|
| `C_WeaponBaton` | `weapon_melee_baton` | ближний бой | bludgeon |
| `C_WeaponPipe` | `weapon_melee_pipe` | ближний бой | bludgeon |
| `C_WeaponAxe` | `weapon_melee_axe` | ближний бой | bludgeon |
| `C_WeaponWrench` | `weapon_melee_wrench` | ближний бой | bludgeon |
| `C_WeaponCleaver` | `weapon_cleaver` | ближний бой | bludgeon |
| `C_WeaponKick` | (удар ногой / без оружия) | ближний бой | bludgeon |
| `C_WeaponPistolGlock` | `weapon_pistol_glock` | пистолет | basehlcombatweapon |
| `C_WeaponPistolBeretta` | `weapon_pistol_beretta` | пистолет | basehlcombatweapon |
| `C_WeaponPistolSocom` | `weapon_pistol_socom` | пистолет | basehlcombatweapon |
| `C_WeaponPython` | `weapon_pistol_python` | пистолет (.357) | basehlcombatweapon |
| `C_WeaponPistolDualies` | `weapon_pistol_dualberetta` | пистолет (парный) | basehlcombatweapon |
| `C_WeaponSMGMP5` | `weapon_smg_mp5` | ПП | basehlcombatweapon |
| `C_WeaponSMGMP5EOD` | `weapon_smg_mp5_eod` | ПП | basehlcombatweapon |
| `C_WeaponSMGMP7` | `weapon_smg_mp7` | ПП | basehlcombatweapon |
| `C_WeaponShotgunM3` | `weapon_shotgun_m3` | дробовик | basehlcombatweapon |
| `C_WeaponShotgunM5` | `weapon_shotgun_m5` | дробовик | basehlcombatweapon |
| `C_WeaponShotgunSpas12` | `weapon_shotgun_spas12` | дробовик | basehlcombatweapon |
| `C_WeaponShotgunXM1014` | `weapon_shotgun_xm1014` | дробовик | basehlcombatweapon |
| `C_WeaponSniper` | `weapon_rifle_sniper` | винтовка | basehlcombatweapon |
| `C_WeaponG36K` | `weapon_rifle_g36k` | винтовка | basehlcombatweapon |
| `C_WeaponBfgMgl` | `weapon_bfg_mgl` | гранатомёт MGL | basehlcombatweapon |
| `C_WeaponBfgMinigun` | `weapon_bfg_minigun` | пулемёт | basehlcombatweapon |
| `C_WeaponGaussGun` | (вырезан, есть RTTI) | — | basehlcombatweapon |
| `C_WeaponAnnabelle` | `weapon_annabelle` | револьвер Аликс | basehlcombatweapon |
| `C_WeaponAlyxGun` | `weapon_alyxgun` | пистолет Аликс | basehlcombatweapon |

> `C_WeaponHopwire` в Episodic уже есть как штатный `weapon_hopwire` (`game/server/episodic/`).

### 3.2. Стандартные HL2-оружия (уже есть в SDK — переиспользуются)

`C_Weapon357` (`weapon_357`), `C_WeaponAR2` (`weapon_ar2`), `C_WeaponSMG1` (`weapon_smg1`),
`C_WeaponShotgun` (`weapon_shotgun`), `C_WeaponPistol` (`weapon_pistol`), `C_WeaponFrag`
(`weapon_frag`), `C_WeaponCrowbar` (`weapon_crowbar`), `C_WeaponStunStick` (`weapon_stunstick`),
`C_WeaponSLAM` (`weapon_slam`), `C_WeaponRPG` (`weapon_rpg`), `C_WeaponCrossbow`
(`weapon_crossbow`). Плюс служебные `C_WeaponCycler`, `C_WeaponCubemap`,
`C_WeaponCitizenSuitcase`, `C_WeaponCitizenPackage`, `C_WeaponBugBait`, `C_WeaponBinoculars`.

---

## 4. Поток данных (как всё связано)

```
weapon_*.txt ──(KeyValues)──▶ CUHWeaponInfo::Parse()   [game/shared/episodic/uh_weapon_parse]
                                    │
            GetWpnData() → GetUHWeaponInfo(pWeapon)   (cast к CUHWeaponInfo)
                                    │
   ┌────────────────────────────────┼─────────────────────────────────┐
   ▼                                ▼                                 ▼
 CBaseHLCombatWeapon (база)    FireBullets (точность/разброс)      CBaseViewModel
  - recoil, accuracy           client/sub_100D8E90                 (прицеливание)
  - penetration                читает m_bIronSighted               m_bExpSighted, m_expFactor
  - melee/stamina                                                CalcExpWpnOffsets()
                                                                CalcViewModelView()
   ┌───────────────────────────────┴──────────────────────────────┐
   ▼                                                              ▼
 CHL2_Player / C_BaseHLPlayer                               ironsight_toggle
  m_bIronSighted, m_fIronsightedTime                        (concommand, сервер)
  m_iFOV…m_hZoomOwner (зум)                                  HIDEHUD_CROSSHAIR
        ▲                                                          │
        └────────── server/sub_101ECF40 (toggle, FOV, звуки) ──────┘
```

---

## 5. План реализации (roadmap)

Состояние по файлам (Episodic-сборка):

1. ✅ **`game/shared/episodic/uh_weapon_parse.h/.cpp`** — `CUHWeaponInfo` (все поля из §2.2) +
   `Parse()` + `CreateWeaponInfo()` + `GetUHWeaponInfo()`. Подключено в
   `client_episodic-2005.vcproj` / `server_episodic-2005.vcproj` (заменяет `weapon_parse_default.cpp`).
2. ✅ **`game/shared/baseviewmodel_shared.h/.cpp`** — `m_bExpSighted` (сетевой) +
   `m_expFactor`/`m_bPrevExpSighted`/`m_flIronsightedChangeTime` (локальные), `CalcExpWpnOffsets()`,
   правки `CalcViewModelView()`/`Spawn()`, сетевая таблица + предсказание. Всё под `HL2_EPISODIC`.
3. ✅ **`game/server/hl2/hl2_player.h/.cpp` + `game/client/hl2/c_basehlplayer.h/.cpp`** — сетевые
   поля `m_bIronSighted`/`m_fIronsightedTime`, серверная команда `ironsight_toggle`
   (звук `HL2Player.Ironsighton/off` + FOV-зум через `SetFOV` + синхронизация `m_bExpSighted`
   вьюмодели), скрытие прицела (`HIDEHUD_CROSSHAIR`) на клиенте. Под `HL2_EPISODIC`.
4. **Оружейные классы** (§3.1) — серверные классы в `game/server/episodic/` +
   клиентские стабы в `game/client/episodic/`, регистрация через
   `LINK_ENTITY_TO_CLASS` + `PRECACHE_WEAPON_REGISTER` + `IMPLEMENT_SERVERCLASS_ST`:
   - ✅ Ядро огнестрела: `CUhFirearmWeapon` — `uh_basefirearm.h/.cpp` — переопределяет
     `PrimaryAttack` (маршрут выстрела через свой `FireBullets`), впрыскивает
     урон из `sk_plr_dmg_*`/`sk_npc_dmg_*`, отдачу (`Punch/Snap` + `CrouchRecoilMult`),
     точность (`CrouchAccuracyMult`/`RunAccuracyMult`/`ExpOffset.accuracy`) и проникание
     (`Penetration` → `FireBulletsPenetrating`, §2.6) из скрипта.
   - ✅ Ближний бой: `CWeaponBaton/Pipe/Axe/Wrench/Cleaver` — `uh_weapon_melee.cpp`,
     база `CUHMeleeWeapon : CBaseHLBludgeonWeapon` читает `MeleeRange`/`MeleeRoF` из
     скрипта; урон — ConVar'ы `sk_plr_dmg_*`/`sk_npc_dmg_*` (имена из декомпа).
   - ✅ Пистолеты: glock / beretta / socom / python / dualberetta — `uh_weapon_pistols.cpp`.
   - ✅ ПП: mp5 / mp5_eod / mp7 — `uh_weapon_smg.cpp`.
   - ✅ Дробовики: m3 / m5 / spas12 / xm1014 — `uh_weapon_shotguns.cpp`
     (база `CUhShotgunWeapon` — залп из N дробин, N из `sk_plr_num_shotgun_pellets`).
   - ✅ Винтовки: g36k / sniper — `uh_weapon_rifles.cpp`.
   - ✅ Прочее: bfg_mgl / bfg_minigun — `uh_weapon_bfg.cpp` (у MGL пока hitscan;
     дуговая граната — follow-up).
   - ✅ Проникание `UH_Weapon_Special.Penetration` — `CUhFirearmWeapon::FireBulletsPenetrating`
     (§2.6), значение из оригинальных `weapon_*.txt`.
   - ✅ Быстрые действия: `dropweapon` / `throw_nade` / `uh_jake_kick` (§2.7) — серверные
     `CON_COMMAND` + обработка в `CHL2_Player::ClientCommand`; `additionalequipment` понимает
     список через запятую (случайный выбор, §2.7).
   - ⬜ Система выносливости (kick/mеле расходуют `m_iEndurance`).
5. **Скрипты** `weapon_*.txt` — перенос из `Underhell/scripts/` в `scripts/` мода
   (контент, вне SDK-кода; классы уже читают их данные через `CUHWeaponInfo`).

### Прицеливание — как реализовано (шаги 2–3)

- Вьюмодель: `CNetworkVar(bool, m_bExpSighted)` в `DT_BaseViewModel` (совпадает с
  `client/sub_10015160`, offset 1960 в оригинале). `CalcViewModelView()` детектит смену состояния,
  пересчитывает время и плавно интерполирует позицию вьюмодели к точке `ExpOffset` (0.1 c).
- Игрок: `m_bIronSighted`/`m_fIronsightedTime` в `DT_HL2_Player` (совпадает с
  `client/sub_10043D70` / `server/sub_101E6C70`).
- Серверная команда `ironsight_toggle` повторяет `server/sub_101ECF40`: кулдаун 0.1 c, звук,
  FOV-зум, запись состояния + репликация `m_bExpSighted` на вьюмодель.
- FOV при прицеливании = `GetDefaultFOV() * UH_IRONSIGHT_FOV_SCALE` (0.6), время 0.15 c —
  точные числа Underhell из декомпа не восстанавливаются, константы вынесены в `hl2_player.cpp`
  и легко настраиваются.
- `ironsight_toggle` регистрируется только на сервере (`FCVAR_GAMEDLL`): в SP команда из консоли
  пробрасывается на сервер автоматически. Позже пропишем бинд в `kb_act.lst`.

> Примечание по «1:1»: раскладка полей в декомпиляции показывает, что Underhell добавлял поля
> прямо в начало структуры данных оружия (offsets 8–103 до `szClassName`). Для чистоты мы не
> трогаем валиевскую `FileWeaponInfo_t`, а делаем `CUHWeaponInfo : FileWeaponInfo_t` — поведение
> идентично, а память и совместимость со стандартными оружиями сохранены.

---

## 6. Что ещё добавить в будущем (по коду)

Таблица незакрытых фич Underhell, выявленных при портировании. Каждая — отдельная система,
завязанная на свой декомпилированный код; оружейное ядро от них не зависит.

| # | Фича | Декомп-ориентир | Где добавить | Статус |
|---|---|---|---|---|
| 1 | **Выносливость** (`m_iEndurance`) — меле и удар ногой расходуют 6–20 ед., регенерация | дататейбл `DT_BasePlayer` (`m_iEndurance`), `uh_jake_kick` (drain 20), `StaminaToDrain` из `weapon_*.txt` | `CHL2_Player` + `CUHMeleeWeapon`/`PerformKick` + HUD | ⬜ (парсер `StaminaToDrain` уже готов) |
| 2 | **Кровотечение** (`m_iBleedCounter`) | `DT_BasePlayer` (`m_iBleedCounter`) | `CHL2_Player`, damage-хендлинг | ⬜ |
| 3 | **Инвентарь** (`cl_inventoryToggle`) | `client/sub_102BC5D0` (ConCommand на клиенте) | клиент: VGUI-панель + конкоманда | ⬜ |
| 4 | **ПНВ / противогаз / глушитель** (`NightVision_Toggle` / `GasMask_Toggle` / `silencer_toggle`) | `server/sub_101F11D0` (диспетчер) | `CHL2_Player` + постпроцесс (ПНВ) + смена звука выстрела (глушитель) | ⬜ |
| 5 | **MGL — дуговая граната** (`weapon_bfg_mgl` на `SMG1_Grenade`) | — (класс `CWeaponBfgMgl`) | `uh_weapon_bfg.cpp`: projectile вместо hitscan | ⬜ |
| 6 | **FireMode** (полуавто/авто) | ключ `FireMode` в `weapon_*.txt` (в бинаре не читается) | `CUhFirearmWeapon`/machinegun-база | ⬜ (опционально) |
| 7 | **Хаптика** (`hap_HasDevice` + `hap_*`) | клиент (ConVar/ConCommand HUD) | клиент: forcefeedback | ⬜ |
| 8 | **`uh_ragdollcollisiontype`** | `server/sub_10452C60` | `CHL2_Player` (параметр ragdoll при смерти) | ⬜ |
| 9 | **Контент-конфиги** — `kb_act.lst` (бинды `throw_nade`/`uh_jake_kick`/`dropweapon`/`ironsight_toggle`), `skill.cfg` с дефолтами `sk_plr_dmg_*`, перенос `weapon_*.txt` + `game_sounds_weapons.txt` | `Underhell/scripts/` | `scripts/` мода (вне SDK) | ⬜ |
| 10 | **Полировка** — отдельная анимация удара ногой (сейчас `PLAYER_ATTACK1`), рандомные дефолты `sk_plr_dmg_*` подобрать под оригинал | — | см. `uh_weapon_melee.cpp`, `uh_basefirearm.cpp` | ⬜ |

Все уже перенесённые куски помечены `✅` в roadmap (§5); незакрытое выше — это следующий слой
поверх оружейного ядра (п.1–2 — «живучесть», п.3–4 — снаряжение/UI, п.5–6 — поведение оружия).
