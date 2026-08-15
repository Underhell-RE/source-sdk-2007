# Underhell — reverse-engineering / port status

Этот файл — краткая сводка того, что мы сделали поверх `source-sdk-2007`
для drop-in `client.dll` + `server.dll` под существующий инсталл Underhell,
и что стоит перепроверить после сборки.

Эталон для сверки: декомпил `klaxons1/underhell-hexrays`
(`Underhell/bin/serveror.dll` / `Cliento.dll`, `sub_*.cpp`, RTTI-дампы).
Он не «единая истина»: где декомпил противоречит работоспособности —
сделан рабочий вариант и помечен TODO в `docs/UNDERHELL.md`.

---

## Что реализовано

### Оружие / бой
- **Кик** (`uh_jake_kick`) — конвары `uh_kick_damage/forcemult/enabled`, инпуты
  `DisableKick/EnableKick`, выход `OnKicked`, сетевое `m_bKickMarker`, вьюмодель 2.
- **Прицел** (ironsight) — `uh_ironsight_zoom` (0.9), `uh_ironsight_zoom_focus` (скорость),
  сетевой `m_bIronSighted` + FOV-зум.
- **Глушитель** (`silencer_toggle`) — гейты на пистолет/винтовку (`m_bHavePistol/RifleSilencer`),
  `SetPistolSilencer`/`SetRifleSilencer`.
- **Лазер** (`laser_toggle`) — состояние `m_bLaserToggleState` (точное имя команды неизвестно).
- **Переключение огня** (`firemode_toggle`) — full-auto ↔ semi, клик `Weapon_Pistol.Empty`.
- **Подбор оружия** — только `+use`; проход рядом снимает патроны из магазина, оружие остаётся;
  одно оружие на бакет, старое выбрасывается вперёд (разлёт).
- **Дробовики** — 7 дробин + конус 10° (игрок и NPC).
- **NPC** — acttables по категориям (стрельба/Т-поза), расчленёнка, каски, подбираемые тряпки,
  кровь, `uh_bodygroup`/`uh_fos`/`uh_viewdistance`.

### Инвентарь / предметы
- Полный набор предметов: еда, сода, глоустики, бинты/обезболивающие/шприц,
  аптечка/флакон, броня/тяжёлая броня, каски/респиратор/противогаз, фонарик,
  ПНВ, рации, аммо-боксы, фальшфейеры, `item_random`.
- Патроны-боксы: точные количества + звуки (Pistol 30 / 357 20 / SMG1 50 / AR2 50 / Buckshot 6).
- Фонарик на батарейках (`m_iUHBatteryCount`).

### Снаряжение
- **ПНВ** (`NightVision_Toggle`/`SetNightVision`) — гейт владения + батареи, оверлей.
- **Противогаз** (`GasMask_Toggle`/`SetGasMask`) — гейт владения, зацикленное дыхание, оверлей.
- Взаимоисключение ПНВ ↔ противогаз; еда/питьё блокируются в маске.

### Вторая рука / левая рука (вьюмодель 1)
- Фонарик в левой руке (одноручное/холодное оружие, либо свободная рука).
- Фальшфейер в руке → бросок горящего `prop_physics`.
- Анимированный бросок гранаты (граната в руку, `ACT_VM_THROW`, вылет через 0.4 с).

### Прочее
- `env_message`/`env_hudhint`/`env_global`, игрок в зеркале (`m_bIsMirrorOnly`).
- `prop_dynamic_override` +use (`OnPlayerUse`), `additionalequipment` — одно случайное оружие.
- `Give`/`GiveInv` инпуты, `impulse 101`, сиденье стрелка на джипе (частично).

---

## Что ещё НЕ сделано (TODO)

- **Bullet time** (`bt_*`) — расшифровано, не портировано.
- **Обратный NPC-кик** (`m_flViewkick` / `m_hLastNPCToKickMe`).
- **Полная вьюмодель-система фонарика** (плечевое крепление, хольстер-анимация,
  свет от `v_flashlight_pg.mdl` вместо ванильного `EF_DIMLIGHT`).
- **Щит** — настоящая механика блока (пока +10 брони заглушкой).
- **Заражённые NPC** — карабканье, спринт, распространение заражения, гибы.
- **Free-aim камера** (за плечо / third-person).
- **Hermit cards** — только чит `uh_give_hermit_card`.
- **Транспорт** — вход NPC в транспорт, трассер AR2 на турели джипа; застревание BFG-джипа колёсами.
- **Точные тайминги** — pump-задержка дробовика, чтение `sk_plr_num_shotgun_pellets` из конвара.
- **4 предмета с неизвестными именами классов**: `item_bandagespack`, `item_syringepack`,
  `item_flags`, `item_health`.

---

## Что перепроверить после сборки

⚠️ **Каждый раз после сетевых/лейаут-правок — ПОЛНЫЙ clean-rebuild
`client.dll` И `server.dll`.** Инкрементальная сборка даёт рассинхрон таблиц и
краш `RecvProp type doesn't match ... /m_hViewModel` (из-за общего
`MAX_VIEWMODELS` = 3 и новых сетевых полей).

### Последние правки (проверить в игре)
1. **impulse 101** — теперь выбрасывает вытесненное оружие на пол (разлёт),
   а не молча удаляет.
2. **Глоустик в руке** — светит, но НЕ должен сталкиваться (физика уничтожена,
   `SOLID_NONE`).
3. **Выброс светящегося глоустика** — должен спавниться светящийся проп,
   а не исчезать.
4. **Фальшфейер** — бросается и светит (`kRenderGlow` + `EF_BRIGHTLIGHT`).
5. **Фонарик** — с двухручным оружием (автомат/дробовик) НЕ отказ, а авто-переключение
   на одноручное (пистолет в приоритете, melee fallback) и подъём фонарика в левой руке
   (`sub_101F0C60` -> `sub_101E60C0`). Портировано в `FlashlightTurnOn` + `UH_FindOneHandedWeapon`.
   `UH_UpdateLeftArm` использует тот же гейт `!pWeapon || OneHanded || MeleeWeapon`,
   что и `FlashlightTurnOn` (раньше без оружия свет горел, а вьюмодель не поднималась).
6. **Радио (useitem)** — починено: ветка FM-radio/radiocracker была мёртвым кодом
   (вложена в блок фальшфейера после `return true`). Теперь `useitem` спавнит
   `uh_radio` и активирует его.
7. **Гранаты** — починено: бросок был завязан на `Weapon_OwnsThisType("weapon_frag")`,
   которого у игрока никогда нет (граната = патроны). Теперь гейт по
   `GetAmmoCount("grenade")` + прямой спавн `Fraggrenade_Create` + `RemoveAmmo`.

### Аудит HUD (клиент) — найдено и починено

Сверил 5 HUD-панелей с декомпилом (`sub_100BDF90`/`sub_100BDC80` батарея,
`sub_100CAC10` стамина, `sub_100C8710` выносливость, `sub_100BE800` кровь,
`sub_100BCFA0`/`sub_100BD080` карты) + recv-таблицей `sub_10043D70` +
HudLayout.res/HudAnimations.txt/ClientScheme.res.

- **Батарея: не хватало триггера ПНВ** (логический баг, починено). Оригинал
  светит шкалу при `m_bFlashlightOn (5286) || m_bNightVisionOn (3449) || count
  (5292) изменился`. У нас был только фонарик+счёт — ПНВ жрёт батареи, шкала
  гасла. Добавлен `m_bNightVisionOn`.
- **Батарея: бар был сплошной, в оригинале чанкованный** (починено). 7 чанков
  2px/1px, bottom-up, `round(charge/100*7)`.
- **Текст счёта** `"x%i"` → `"   x%i"` (починено).
- Стамина/выносливость — проверено, совпадает (пороги 35 / 20/50, FgColor
  анимации, направления заполнения).
- Задокументировано (не менял): карты гермита — бинарный show/hide vs наш
  плавный фейд; пропущен `CHudDotReticle`; позиция текста счёта батареи.
Полная таблица оффсетов + разбор — в `docs/UNDERHELL.md` (§ «HUD audit»).

### Аудит VMF (карты) — нереализованные инпуты игрока

Карт в SDK-репо нет — они в декомпиле (`klaxons1/underhell-hexrays` →
`Underhell/maps/`, 5 шт.). Все 160 classname в картах зарегистрированы в нашей
DLL (нет «unknown entity» при загрузке). Но карты жмут на `!player` инпуты,
которых у нас **нет** (подтверждены в `serveror.dll`):

`SetStatusVisibility` (20), `EnableInventory`/`DisableInventory` (6/5),
`EmptyInventory` (4), `BleedPlayer` (10), `RemoveLitGlowstick` (8),
`removeheldflare` (3), `SetEndurance` (2), `AddEndurance` (1), `SetBatteries` (2),
`GiveShoulderFlashlight`/`RemoveShoulderFlashLight` (2/1), `SetHudVisibility` (3),
`DisplayHermitCards` (1), `DisableBt`/`EnableBt` (1/1 — bullet-time, отдельный TODO).

Плюс: 5 мели-записей пула `item_random` (`weapon_wrench/pipe/axe/hammer/shiv`)
спавнят NULL — не совпадают с `weapon_melee_*`. Это 1:1 с оригиналом
(`sub_101757D0`; сам FGD предупреждает «Melee weapons may not work with
item_randoms»). Полная таблица — в `docs/UNDERHELL.md` (§ «VMF entity audit»).

### Известные места, где возможны баги
- **Гранаты не подбираются** — проверить `sk_max_grenade` в консоли (должно быть `4`
  из `skill.cfg` мода; при `0` — `GiveAmmo` срезает до нуля и гранаты «не берутся»).
- **ПНВ «сплошной градиент»** — это НЕ код DLL: кастомные шейдеры
  (`shader/nightvision`, `shader/gasmask`) реализует `game_shader_generic_eshader_2007.dll`,
  который грузит движок. Проверить, что файл лежит в `<mod>/bin/` и грузится
  (в консоли нет `Cannot load ...`). `shadereditor_2007.dll` — dev-тул, реверсить не нужно.
- **Точные суммы/звуки предметов** — сверены с декомпилом, но стоит пройтись по
  факту в игре (особенно броня, тяжёлая броня, каски).
- **Свет фонарика** — пока ванильный `EF_DIMLIGHT`; вьюмодель `v_flashlight_pg.mdl`
  — только визуальное удержание.

### Полезные команды для проверки
- `sk_max_grenade`, `sk_plr_num_shotgun_pellets` — лимиты из `skill.cfg`.
- `uh_kick_damage`, `uh_ironsight_zoom`, `uh_flashlight_battery_time` — конвары фич.
- `impulse 101` — выдача всего набора.

---

Полная хронология и декод-заметки — в `docs/UNDERHELL.md`.
