# Аудит: что заявлено в md vs. что реально в коде

Сверка `agent.md`, `readme.md`, `docs/ARENA_CURRENT_STATE.md` с фактическим состоянием
дерева на `arena/01a00c4c-source-sdk-2007` (базовый коммит `7fe159d`).

Метод: чтение исходников, проверка регистрации в `.vcproj`, поиск заявленных
классов/конваров/инпутов/пропов, проверка порядка сетевых таблиц.
Сборка под Windows/MSVC в этом окружении невозможна — где написано «не собирается»,
это вывод из статического чтения, а не из компилятора.

---

## 🔴 БЛОКЕР 1 — `hud_dotreticle.cpp` физически не компилируется

`game/client/underhell/hud_dotreticle.cpp`, строка 26:

```cpp
#include "c_basehlplayer.h"}ાજેતળીажәлар to=functions.edit_file  天天中彩票中了和天天中彩票】【。】【”】【av不卡免费播放  手机天天彩票Error? Let's see result.numerusform to=functions.edit_file 񹚒 ฝ่ายขายละคร  天天中彩票无法  彩神争霸的={
```

В исходник попал мусор от сбоя генерации предыдущего агента: закрывающая скобка,
обрывки китайского/тайского/гуджарати спама и текст служебного вызова инструмента
(`to=functions.edit_file`). Это не комментарий — это код вне любого блока.
Файл **не пройдёт препроцессор**, значит `client.dll` не собирается вообще.

Это состояние уже **в HEAD** (`git show HEAD:...` даёт тот же мусор), то есть
попало в `master` через PR #9.

При этом `agent.md` пишет:

> ### Точка-прицел (CHudDotReticle) + фейд батареи — **реализовано**

Заявлено как готовое, по факту — сломанная сборка. Ни один пункт «проверить в игре»
из `agent.md` не мог быть проверен: клиент не собирался.

Файл единственный такой — прогнал всё дерево на не-ASCII и на маркеры
`to=functions` / `numerusform`, других заражённых файлов нет. Остальные не-ASCII —
это легитимные типографские тире в комментариях.

**Вердикт: правка на одну строку, но до неё все остальные утверждения о клиенте
непроверяемы.**

---

## 🔴 БЛОКЕР 2 — дублирующийся ConVar `uh_bodymousedamper` (падение при старте)

Объявлен дважды, оба файла — в одном проекте `server_episodic-2005.vcproj`
(строки 280 и 288):

- `game/server/underhell/uh_ai.cpp:50`
- `game/server/underhell/uh_carry_ragdoll.cpp:6`

В Source SDK 2007 повторная регистрация имени конвара — это `Error: ConVar
uh_bodymousedamper already registered` и, как правило, срыв инициализации
серверной DLL. Остальные дубли в списке (`sk_satchel_radius`, `bot_mimic`,
`g_jeepexitspeed` и т.д.) безопасны — они разведены по разным проектам
(`hl2` vs `hl2mp` vs `sdk`), это ванильное состояние SDK. А `uh_bodymousedamper` —
нет, это внесено в этой сессии.

Дополнительно: в `uh_ai.cpp` конвар объявлен, но **нигде не читается** (только
объявление + комментарий). В `uh_carry_ragdoll.cpp` — тоже объявлен и не читается.
То есть заявленное в `ARENA_CURRENT_STATE.md` честно:

> **Not complete:** client mouse/sensitivity damping from `uh_bodymousedamper` is still absent.

— формулировка верна, но не сказано, что мёртвое объявление ломает старт.

---

## ✅ Что заявлено верно (проверено в коде)

| Заявление | Где | Статус |
|---|---|---|
| Порядок sendtable `CHL2_Player` | `hl2_player.cpp:507-541` | ✅ Совпадает с заявленным в readme (`m_HL2Local, m_fIsSprinting, m_bShoulderFlashlight, m_bFlashlightOn, m_bInventoryEnabled, m_iUHBatteryCount, m_iUHHermitCardsCount, m_iUHHermitCurrentQuestCount, m_iUHHermitTotalQuestCount, m_bDisplayHermitCard, m_iInventory[28]`) |
| Recv-таблица клиента в том же порядке | `c_basehlplayer.cpp:31-63` | ✅ Проверил попарно, все 24 пропа идут индекс-в-индекс. Это главный риск drop-in совместимости — тут чисто |
| `MAX_VIEWMODELS 3` | `shareddefs.h:217` | ✅ Есть, с комментарием про кик-вьюмодель. Предупреждение в `agent.md` о необходимости clean-rebuild обосновано |
| Все UH-файлы в vcproj | оба проекта | ✅ 22 записи, ни один `.cpp` из `underhell/` не забыт — включая `npc_ace`, `npc_butcher`, `npc_infected`, `hud_dotreticle` |
| Батарея: триггер ПНВ добавлен | `hud_uhbattery.cpp:79` | ✅ `m_bFlashlightOn \|\| m_bNightVisionOn \|\| count != m_iBatteryCount` |
| Батарея: чанкованный бар 2px/1px bottom-up | `hud_uhbattery.h:46-47`, `.cpp:110-136` | ✅ `BarChunkHeight 2`, `BarChunkGap 1`, `round(charge/100*chunkCount)`, идёт снизу вверх |
| Батарея: текст `"   x%i"` | `hud_uhbattery.cpp:157` | ✅ Три пробела на месте |
| Батарея: фейд `-0.1` float | `hud_uhbattery.cpp:88`, поле `m_flAlpha` | ✅ Поле переименовано в float, шаг 0.1 |
| Карты гермита: бинарный 255/0 | `hud_hermitcards.cpp:108` | ✅ Не плавный фейд, ровно как заявлено в `ARENA_CURRENT_STATE.md` |
| `bt_*` конвары и значения | `uh_bullettime.cpp:10-14` | ✅ `bt_enabled 0`, `bt_timescale 0.3`, `bt_enemybulletspeed 500`, `bt_playerbulletspeed 2000`, `bt_plr_speed 250` — все на месте |
| `impulse 110` → BT | `hl2_player.cpp:1926` | ✅ |
| `EnableBt`/`DisableBt` инпуты | `hl2_player.cpp:429-430` | ✅ Единственные два UH-инпута из «нереализованного» списка, которые реально есть |
| `VPhysicsIsFlesh() == false` для рагдоллов | `physics_prop_ragdoll.h:80` | ✅ |
| `item_gasmask_prison` | `uh_items.cpp:836` | ✅ Зарегистрирован |
| Ace наследуется от `CNPC_CombineS` (структурная ошибка) | `npc_ace.cpp:22` | ✅ Ошибка признана честно и она действительно там: `class CNPC_Ace : public CNPC_CombineS`, тогда как оригинал — `CAI_PlayerAlly` |
| BT-пули = `prop_physics`, не `CBullet` | `uh_bullettime.cpp:118` | ✅ Признано в non-claims, соответствует коду |

Отдельно: раздел «Explicit non-claims» и «MANDATORY VERIFICATION WARNING»
в `ARENA_CURRENT_STATE.md` написаны корректно и без приукрашивания. Список
«Still missing / high-risk» по каждой подсистеме тоже совпадает с кодом.
Это добросовестный документ — основные претензии не к нему, а к `agent.md`.

---

## 🟡 Заявлено неточно / вводит в заблуждение

### 1. `agent.md`: «Аудит VMF — инпуты, которых у нас нет» — список актуален на 100%

Перепроверил все 16 инпутов. Ни один не реализован, кроме `EnableBt`/`DisableBt`:

```
SetStatusVisibility      0        SetEndurance                 0
EnableInventory          0        AddEndurance                 0
DisableInventory         0        SetBatteries                 0
EmptyInventory           0        GiveShoulderFlashlight       0
BleedPlayer              0        RemoveShoulderFlashLight     0
RemoveLitGlowstick       0        SetHudVisibility             0
removeheldflare          0        DisplayHermitCards           0
```

Это не ошибка документа — но стоит понимать масштаб: **14 инпутов**, на которые
жмут реальные карты. Каждый такой вызов — тихий no-op, скриптовые события карты
просто не сработают, без всякой диагностики в консоли.

### 2. `hud_dotreticle`: `SetHiddenBits(1 << 12)` работать не может

`agent.md` пишет:

> `SetHiddenBits(4096)` = 1<<12 — кастомный бит Underhell (за пределами
> ванильного HIDEHUD_BITCOUNT), управляется SetStatusVisibility/SetHudVisibility.

Две проблемы:

1. `m_iHideHUD` отправляется по сети как `SendPropInt(SENDINFO(m_iHideHUD), HIDEHUD_BITCOUNT, SPROP_UNSIGNED)`
   (`playerlocaldata.cpp:28`), где `HIDEHUD_BITCOUNT = 12` (`shareddefs.h:141`).
   12 бит — это индексы 0..11. **Бит 12 физически не влезает в сетевой проп** и
   до клиента не доедет никогда.
2. Управлять им должны `SetStatusVisibility`/`SetHudVisibility`, которых, как
   показано выше, **не существует**.

Итог: бит захардкожен, но и не передаётся, и не выставляется. Панель просто
никогда не скрывается этим механизмом. Комментарий описывает намерение оригинала,
а не поведение порта — и это нигде не помечено как расхождение.

### 3. `hud_dotreticle`: `m_nButtons` на клиенте — не то, чем кажется

`agent.md`:

> Триггер — фронт +use в OnThink

`m_nButtons` **не входит в recv-таблицу** `C_BasePlayer` — это чисто локальное
поле, которое заполняется только в `prediction.cpp:868` через `UpdateButtonState`,
т.е. **только для локального игрока и только когда работает prediction**.
Для сингла это, скорее всего, сработает, но это не «фронт +use», а «фронт +use
в предсказанном кадре», который может отрабатывать несколько раз за тик при
перепредсказании. Детекция фронта по `m_bUseHeld` в `OnThink` при повторном
прогоне prediction может дать ложные срабатывания.

Оригинальный механизм (timestamp на игроке по офсету 3456) от этого свободен.
Расхождение задокументировано в комментарии, но его последствия — нет.

### 4. `agent.md`: «Щит — настоящая механика блока (пока +10 брони заглушкой)»

Формально верно (`uh_items.cpp:638` → `UH_GiveArmorPickup(..., 10, 100)`), но
той же заглушкой сидят `CItemCapPMC` и `CItemHeadsetPMC` — тоже +10/100. В TODO
упомянут только щит. Три разных предмета дают одинаковый эффект-плейсхолдер.

### 5. `readme.md` заявляет «1:1» в заголовке

> Rebuilt 1:1 from decompiled `serveror.dll` / `Cliento.dll`

При том, что `ARENA_CURRENT_STATE.md` в том же репозитории явно перечисляет
десяток подсистем как «not complete or certified 1:1», а клиент вообще не
собирается. Заголовок readme нужно смягчить до «reimplementation, WIP» —
иначе документы противоречат друг другу.

### 6. `agent.md`: «`sk_itemrandom`» в списке конваров

Конвара `sk_itemrandom` не существует. Есть `sk_itemrandom1/2/3`
(`uh_items.cpp:849-851`). Мелочь, но в «полезных командах для проверки» это
приведёт к `unknown command`.

---

## 🟡 Логические замечания к BT (не заявлено, но стоит знать)

`uh_bullettime.cpp` — заявлено честно как «не оригинальный CBullet», но есть
конкретные проблемы, не отмеченные нигде:

1. **`CUHBullet` — мёртвый код.** Класс объявлен, `LINK_ENTITY_TO_CLASS(uh_bullet, ...)`
   есть, датадеск есть — но `uh_bullet` не создаётся нигде в дереве. Реально
   используется `prop_physics` через `UH_BulletTimeSpawnTracer`. Мёртвый класс
   тянет за собой регистрацию энтити и датадеск.

2. **Утечка в `CUHBulletMotionSystem`.** Комментарий гордо заявляет
   «Artificial 120-second visual-bullet removal was removed» / «No fabricated fixed
   lifetime». Но у `prop_physics` **нет собственного механизма удаления** — пуля-проп
   живёт вечно, а её запись в `m_Bullets` удаляется только когда `EHANDLE`
   протухнет, чего не произойдёт. При активном BT каждый выстрел добавляет
   бессмертный проп + запись в вектор, который обходится 20 раз в секунду.
   Оригинальный `CBullet` имел настоящий lifecycle (столкновение → урон → удаление),
   которого здесь нет. Убрать таймаут, не добавив коллизию — это не «ближе к
   оригиналу», это утечка. Пункт «BT duration/behavior too short» от пользователя,
   вероятно, про это же.

3. **`UH_BulletTimePlayerDied` не вызывается.** Функция определена, объявлена в
   заголовке, но `grep` по всему дереву даёт единственное вхождение — само
   определение. BT не выключится по смерти игрока, а `host_timescale` останется 0.3.

4. **`host_timescale` — чит-конвар.** Установка через `pScale->SetValue()` из кода
   пройдёт, но конвар не восстанавливается при смене уровня/загрузке сейва — если
   игрок сохранится в BT, он загрузится в замедленном мире навсегда.

5. **`UH_BulletTimeSpawnTracer` вызывается ровно в одном месте** —
   `uh_weapons.cpp:187`, только для игрока (`bEnemyBullet = false`).
   `bt_enemybulletspeed` не используется ни разу: пуль врагов нет.
   Соответствует признанию «enemy weapon fire not integrated», но конвар
   выглядит рабочим, а он декоративный.

---

## Приоритеты на исправление

| # | Что | Почему |
|---|---|---|
| 1 | Мусор в `hud_dotreticle.cpp:26` | Клиент не собирается. Одна строка |
| 2 | Дубль `uh_bodymousedamper` | Сервер падает/ругается на старте. Удалить одно из двух объявлений |
| 3 | `UH_BulletTimePlayerDied` не вызван + бессмертные BT-пули | Утечка энтить и залипший `host_timescale` |
| 4 | 14 отсутствующих player-инпутов | Скрипты карт молча не работают. Самый большой объём реальной работы |
| 5 | Смягчить «1:1» в `readme.md`, поправить `sk_itemrandom` | Документы противоречат друг другу |
| 6 | Пометить `SetHiddenBits(1<<12)` как нерабочий | Чтобы следующий агент не искал баг там, где его нет |

Пункты 1-3 — это правки на несколько строк, после них есть смысл впервые
попробовать реальную сборку. До этого любые утверждения «проверено в игре»
из `agent.md` следует считать непроверенными.
