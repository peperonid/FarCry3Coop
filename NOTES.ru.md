# Far Cry 3 — заметки по кооперативу

English version: [NOTES.md](NOTES.md)

Статус: в работе (решение владельца 2026-10-22). План: [PLAN.ru.md](PLAN.ru.md).

## Данные игры (правило 5 AGENTS.md)

Проверено 2026-10-22 на ПК владельца:

- Магазин: Steam, app 220240, `E:\SteamLibrary\steamapps\common\Far Cry 3`; работает через Ubisoft Connect (`bin\upc_r1_loader.dll`), `steam_api.dll` нет
- Номер сборки Steam: 13146413
- SHA-256 `bin\farcry3.exe`: `fd3929f19ff825912f73bd0158604823c46ab9c124d18d57b0fab05a51cda9bb`
- SHA-256 `bin\farcry3_d3d11.exe`: `5437b2e37a54f8a3b28c342f2b2670ca907fd60bc74905b0b7527f69bafc965e`
- SHA-256 `bin\FC3.dll`: `71d58a7a4503c5c67dcd6f4b9445b5e7ac34c12043987f4c7285e13f2ed51c9b`
- SHA-256 `bin\FC3_d3d11.dll`: `e541594e11ac6091dc7d3702f2cd90504280b82cf15ec10b4eb148aae27f0809`
- Движок: Dunia
- Установка проверена на сторонние DLL: в `bin` только файлы игры
- В `data_win32\patch.dat/.fat` был мод владельца на автопропуск катсцен. С разрешения владельца он переименован в `patch.dat.skipcutscenes` / `patch.fat.skipcutscenes`, оригинальная пара (лежала как `.vanilla`) возвращена. SHA-256 оригинального `patch.dat`: `b76a3ae5092bec46e4d000dfebb0dc1fac9d7990d217421528447d59a7661833`, `patch.fat`: `39dc4178723bfd4e2f9f512d42401790b80a3f4ea305d4ab38889f546780b39f`
- Есть также: `bin\FC3Editor.exe` (официальный редактор карт), `data_win32\worlds\fc3_main`, `data_win32\worlds\multicommon`

## Что известно

- [x] Найти реестр игровых режимов и routine заполнения: ЗАКРЫТО 2026-11-06 — контейнер `[0x128048c0]`
  заполняется из `engine\gamemodes\GameModesConfig.xml` (строка `0x11f16940` в `FC3.dll`; загрузчик
  `0x117568a0`, вызываемый из `0x117569f0`, который затем запрашивает `GameModeProperties` `0x11f16968`
  через `mov edx,[eax+0x58]` / `call edx` на `0x117569ba..0x117569c5`); заполнение выполняет
  `0x11709af0 .. 0x11709ca0`: количество потомков `[ecx]+0x4c` (`0x11709b03`), потомок по индексу
  `[eax]+0x50` (`0x11709b1c`), проверка имени `push 0x11f10d78 'GameMode'` + vtable `[edx+0x14]`
  (`0x11709b35..0x11709b3d`), выделение записи `mov edx,0x10; lea ecx,[edx+0x30]; call 0x1177f7c0`
  (`0x11709b5c..0x11709b64`), инициализация записи: vtable `mov dword ptr [eax], 0x11f224b4`
  (`0x11709b86`), `mov dword ptr [eax+0x20], 0xffffffff` (`0x11709ba1`, идентификатор режима, который
  ищет `cmp dword ptr [ebx+0x20], esi` на `0x1181cf13`), `movss [eax+0x28], xmm0` из `[0x11ef3764]`,
  `mov word ptr [eax+0x34], 0x100` (`0x11709bb6`), `mov byte ptr [eax+0x36], 1` (`0x11709bbc`), затем
  `call 0x11969740` (`0x11709bcc`). Остается открытым: какой атрибут XML пишет идентификатор в
  `[eax+0x20]`.
- [x] Проверить, есть ли ко-оп как игровой режим в поставляемых данных 1.06: ЗАКРЫТО 2026-11-06 — да.
  `H:\FarCryCoop\TestFC3\data_win32\worlds\downloadcontent\mp_dlc1\toc.rml` (1989 байт) содержит
  `GameModes Desc FCXCoop`, и обе карты DLC помечены `gamemodemask 128`: `mpdlc1_aegir` (`id 3001`,
  `presenceId 690271114`) и `mpdlc1_idun` (`id 3002`, `presenceId 3534781404`); там же регистрируется
  `ClassService CharacterAppearancesCoopULC` с `yuriy_ulc_2`, `leonard_ulc_2`, `tisha_ulc_2`,
  `callum_ulc2`. `FCXCoop` встречается и внутри пакетов: `multicommon/multicommon.dat` — 1 совпадение
  (`_FCXCoop.207146615258375806` на смещении 407859421), плюс `MZoneEntity_Coop_Island.2070649460439908519`
  (350109489) и `MZoneEntity_Coop_ToRockets.2077132487061948236` (350279600) среди 331 совпадений `Coop`;
  `downloadcontent/mp_dlc1/mp_dlc1.dat` — 5 совпадений (`DynLoad/ … Mode/FCXCoop)` на смещениях 68659542,
  69112499, 120652426, 121131557 и `_FCXCoop.2077257745930653048` на 160905944); `fc3_main/fc3_main.dat`
  (3496061837 байт) — 1 совпадение на смещении 2379019038 в той же форме `DynLoad/ … Mode/FCXCoop)` плюс
  331 совпадений `Coop`, в том числе категории DOMINO `<Display Category="Coop|Group" …` (1304419581,
  1304592795) и функции DOMINO `Coop/Group/AddTo…` (400798582), `Coop/Entity/Is…` (1030651053),
  `Coop/AllP…` (1088269141, 1312711018). Отрицательный результат: `grep -a -c -i gamemodesconfig` по всем
  `data_win32/*.dat` и `*.fat` дал 0 совпадений, то есть сам XML в этом экземпляре в открытом виде не
  хранится.
- [x] Подтвердить, что классы ко-оп обращаются кодом, а не только присутствуют строками: ЗАКРЫТО
  2026-11-06 — `CFCXCoopService` `0x11fc3d3c` передается через `push` на `0x103ca3aa`, `0x11020fe9`,
  `0x11021004`; `CFCXCoopGateService` `0x11fc5384` на `0x103d141a`, `0x11833c19`, `0x11833c34`;
  `CFCXCompetitiveCoop` `0x11ff136c` на `0x10466e1a`, `0x11047bd1`, `0x11047bec`;
  `CFCXCompetitiveCoopNetFunc…` `0x11ff1334` на `0x10466c80`, `0x10467704`; `CoopBarks` `0x11fd9c1c` на
  `0x11120455`; `SupportedGameModes` `0x11fa84bc` на `0x10351eb7` и на
  `0x10351ebe mov dword ptr [esi + 4], 0x11fa84bc`. Имена служб и консольных команд регистрируются парой
  `push <имя>` + `call 0x10a00be0`, а сканирование всего `.text` насчитывает 1817 таких вызовов и 842
  различных имен (`local/scratch/console_cmds.txt`). Следствие для патчинга: идентификаторы режимов живут
  только в `FC3.dll`, а `FC3.exe` — заглушка на 213 КБ, поэтому патч запуска ко-оп нужен в `FC3.dll`, а не
  в `FC3.exe`.
- Штатный кооп: отдельная кампания из 6 миссий, до 4 игроков онлайн https://co-optimus.com/article/8473/far-cry-3-follow-up-questions-plus-new-co-op-campaign-trailer-and-images.html . Мод: Open World Co-op (scubrah), без сюжетных миссий, альфа https://www.nexusmods.com/farcry3/mods/220 (каталог coop-anything).

## Фаза 1: распаковка данных игры (2026-10-24)

- Инструмент: Gibbed.Dunia (https://github.com/Gibbed/Gibbed.Dunia, commit a43405c), собран из исходников портативным .NET SDK 8.0.426 (`Tools\dotnet-sdk-8.0.426`). Клон: `Downloads\gibbed-dunia\src`, результат сборки `bin_fc3\Gibbed.FarCry3.Unpack.exe`.
- В репозитории Gibbed нет распаковщика Far Cry 3, но `BigFileV2_64` уже знает FAT версии 9 (Windows, версия сжатия 3). Локальный патч [docs/gibbed-dunia-fc3.patch](docs/gibbed-dunia-fc3.patch) добавляет: проект `Gibbed.FarCry3.Unpack`, конфиг `configs/Far Cry 3`, соответствие схем сжатия FAT v9 (0 без сжатия, 1 LZO1x, 2 Zlib) и ограничение вывода LZO для определения типа по первым 64 байтам.
- Результат в `Work\fc3`: `patch.fat` даёт 427 файлов (388 Lua, 11 bin, 6 xbg, 5 hkx, 2 seq); `common.fat` даёт 3099 файлов (1984 xbt, 184 feu, 89 xml, 58 fcb). Списков имён для FC3 нет, поэтому файлы названы хешами.
- Скрипты миссий — обычный текст Lua, созданный инструментом «Domino» (`-- {\v/} Domino auto-generated LUA script file`, в заголовке исходный документ, например `domino/library/sp/lib_mission.domino.xml`).
- Дальше: распаковать `worlds\fc3_main` и остальные архивы, сгруппировать скрипты Domino по исходному документу, найти таблицы миссий и коллекционных предметов.

### Результаты фазы 1: `fc3_main` (2026-10-24)

- `worlds\fc3_main\fc3_main.fat` распаковывается в `Work\fc3\fc3_main`: 2,7 ГБ; 17944 xbt, 5247 xbg, 3628 bin, 3397 hkx, 2229 lua, 1069 fcb, 818 seq. Поля заголовка FAT по смещениям 0x0C/0x10 (здесь 117303 и 50) относятся к таблицам локалей после основной таблицы; одна основная таблица (64354 записи) покрывает весь `.dat` (максимальное смещение + размер = размер файла). Локальный патч пропускает таблицы локалей. Архивы локалей и `multicommon` пока не распакованы.
- Индекс скриптов (файл скрипта → исходный документ Domino): `Work\fc3\script-index.tsv`, 2363 строки. Группы: основные миссии 1175 скриптов (`fc3_mainmissions/<id>`, около 80 идентификаторов: `jas_*` Джейсон, `hoy_*` Хойт, `doc_*` Док, `bam_*`, `cit_*`, `den_*`, `hur_*`, `sam_*`, `wil_*`, `bng_*`, `game_chapter02..10`, `game_credits`), побочные задания 155 (`fc3_poisquests`), столкновения 103, социальные испытания 82, раздатчики заданий 82, хаб и цепочки заданий 81, DLC ulc 74, аванпосты 33 (`fc3_outpost/<клетка карты>`), сохранение состояния 21 (`fc3_persistency`), занятия 14 (покер, метание ножей, гонки, тир, магазин), главные скрипты 91 (`fc3_master`: аванпосты, радиовышки, сохранение состояния, телепорт, нарративные сцены, закрытые зоны, побочные задания).
- Lua-файлы — только графы Domino: они создают и соединяют «боксы» (`Domino/System/AI/`, `Player/`, `Narrative/`, `UI/` и т. д.). Код боксов не на Lua, он собственный в `FC3.dll`. Поэтому логику миссий и прогресса можно менять в двух местах: в графах (наш архив) и в родных боксах (перехваты ASI).
- В игре уже есть кооперативные боксы: `Domino/System/Coop/` (AllPlayers: GodMode, UnlimitedAmmo, TeleportPlayer; Group: AddToGroup, GetFromGroup, GroupUseSTP; Misc: GetEntityByName, EndGame) и библиотека `domino/library/coop/helpers`. Их используют 6 штатных кооперативных миссий из `multicommon`, а не одиночная кампания.
- Для выбора варианта (A: сетевой слой движка, B: свой сетевой код в ASI): в графах одиночной кампании нет учёта нескольких игроков, поэтому решение требует разбора родной части (`FC3.dll`) в REA/Ghidra и мода scubrah. REA при старте сессии не подключился (CONNECT_TIMEOUT).

### Результаты фазы 1: `multicommon` и кооперативные боксы (2026-10-24)

- `worlds\multicommon\multicommon.fat` распаковывается в `Work\fc3\multicommon` (2128 Lua-скриптов; их документы добавлены в `script-index.tsv`). Там 6 штатных кооперативных миссий в виде документов Domino `domino/user/coop/coop_{vidar,huginn,skadi,freyja,sleipnir,fenrir}` (около 1650 скриптов), режим гонки `coop_cce_race` и общие библиотеки `domino/library/coop/{mission,pawns,vehicle,mapsetup,ui,helpers}`.
- Кооперативные боксы рассчитаны на небольшие миссии с целями, а не на сюжет: `PlayerCount` / `PlayerCountTest` (3178 использований), `AllPlayers/*` (GodMode, TeleportPlayer, ProximityTrigger, ManageInventoryAllPlayers), `SetCoopCheckPoint`, `Group*`, `StrayPlayerZone`, `CoopGate`, `ForceOutPlayerFromVehicle`. Полезная мысль: движок уже считает игроков, телепортирует всех и хранит общую кооперативную контрольную точку.
- Графы одиночной кампании получают игрока через один бокс: `Domino/System/Player/GetLocalPlayer` (1484 использования в `fc3_main`, 291 в `patch`, всего 10 в `multicommon`). Остальные боксы игрока (`ManageInventory_v4` 197, `SetHudMode` 136, `PostFx` 637, `ForceInVehicle`, `SwitchPawnPlayer`) принимают этого игрока на вход. Кандидат на перехват для варианта B: родной код `GetLocalPlayer` и поиск игрока по слоту `GetPlayerFromSlot`; для варианта A: как `PlayerCountTest` читает список игроков.
- Дальше: найти эти боксы в `FC3.dll` (REA/Ghidra, строки `GetLocalPlayer`, `PlayerCount`), затем попробовать запустить открытый мир внутри кооперативной сессии. (Сделано ниже: боксы оказались Lua, а не родным кодом.)

### Результаты фазы 1: `FC3.dll` и боксы Domino (2026-10-24)

- **Исправление:** код боксов — Lua. Граф подключает боксы через `cbox:RegisterBox("Domino/System/Player/GetLocalPlayer.lua")`; каждый бокс — Lua-файл в тех же архивах с заголовком-описанием (`-- DOMINO REFLECTION BOX START`, `<Display>`, `<ControlIn>`, `<DataOut>`). Прежнее утверждение «код боксов родной в `FC3.dll`» было неверным. Родными являются только Lua-глобалы, которые вызывают боксы (например `GetLocalPlayerId()`).
- **Имена файлов — хеши имён боксов.** Хеш имени = CRC-64/ISO (отражённый, полином 0xD800000000000000, начальное значение 0, без финального xor, таблица = `Gibbed.Dunia.FileFormats.Hashing.CRC64`) от **пути в нижнем регистре с обратными косыми чертами**. Проверка: `domino\system\player\getlocalplayer.lua` даёт `9716CA537E634631`; такой файл есть в `fc3_main\__UNKNOWN\unknown\` и в `multicommon\__UNKNOWN\unknown\` (782 байта, одинаковый, MD5 `5E8EFF68D5AF85A2E7F849D233170134`). Ещё: `domino\system\getscriptentity.lua` даёт `235DF3F86557C6FA`. Файлы без расширения в `unknown\` — это боксы.
- **Списки имён всё-таки есть:** манифесты ресурсов `__UNKNOWN\misc\*.root.xml` (например `fc3_main\__UNKNOWN\misc\7215F999AEACCD55.root.xml`, `multicommon\__UNKNOWN\misc\7FFFAC13D814F0C8.root.xml`) перечисляют `<CDominoBoxResource ID="Domino/...lua">`, текстуры, материалы и другое. Хеширование этих ID даёт имена всех файлов. Пока не сделано.
- **Бокс `GetLocalPlayer` тривиален:** `function GetLocalPlayer:In() self.LocalPlayer = GetLocalPlayerId(); Out(self) end`. Значит, графы одиночной кампании получают локального игрока через Lua-глобал `GetLocalPlayerId`, а файл-замена бокса в нашем архиве может изменить это для 1484 мест вызова в `fc3_main`.
- **`FC3.dll` (x86, Ghidra 12.1.4, через REA):** у строки `GetLocalPlayerId` по адресу `0x11fbdcc4` две ссылки. (1) `0x111ce912` в `FUN_111cdb00` — таблица регистрации **консольных команд** (`GetLocalPlayerId` -> `FUN_111cb980`, она только форматирует строку, то есть это не Lua-функция; в той же таблице `TeleportToCurrentObjective`, `SetHealth`, `HitMe`, `GetCurrentPrimaryBuddy`, `SetNoWeaponMode`, `ActivateChallenge`, `CompleteChallenge`). (2) `0x11b1b5c4` в `FUN_11b1b4e0` (4442 байта) — вероятно, регистрация **Lua-глобала**. Строка `Game:GetLocalPlayerId(%%)` по адресу `0x11fba634` упоминается из `0x111ef365` (очень большая функция). Пока не декомпилированы.
- В `FC3.dll` строками не найдены: `PlayerCount`/`GetPlayerFromSlot` (есть только `m_playerCount: %i` по `0x11fafae8` и `MultiCaptureBarPlayerCount` по `0x12026c84`) и `Domino/System/Player`. Имён боксов в DLL нет; `PlayerCount` — Lua-бокс в `multicommon`.
- REA и `FC3.dll` (40 МБ): первый анализ занимает около 10 минут; лимит старта по умолчанию 330 с не хватает. В записи `rea` уровня пользователя теперь `REA_GHIDRA_STARTUP_TIMEOUT_MS=3600000`. Рабочая копия: `Work\fc3\bin\FC3.dll` (SHA-256 равен оригиналу). Лишняя запись `rea` уровня проекта (`npx -y`) вызывала CONNECT_TIMEOUT и удалена.
- Дальше: декомпилировать `FUN_11b1b4e0` и найти родную функцию за Lua-глобалом `GetLocalPlayerId`; прочитать файлы боксов `PlayerCount` и `Coop/AllPlayers/*`; захешировать ID из манифестов и назвать все файлы; затем выбрать вариант A или B. (Сделано ниже.)

### Результаты фазы 1: имена файлов и родные кооперативные примитивы (2026-10-24)

- **Имена файлов.** Все 22 манифеста `*.root.xml` содержат 65217 уникальных ID ресурсов; хеширование (правило выше) называет 45045 из 162363 файлов с хешем (3555 — Lua; остальное — .mab, .xbt, .xbg, .spk, .bin, .dpax и т. д.). Список: `Work\fc3\names.tsv` (`hash`, `name`, `archives`). Текстовые файлы (.lua, .xml) с настоящими путями: `Work\fc3\named\<архив>\...` (4104 файла). В манифестах нет, найдены хешированием известного пути: Lua-библиотеки в `common` (например `Domino/System/Coop/LuaLibraries/LuaLibCoop.lua` = `common\__UNKNOWN\unknown\D2329431C5AD239C`, `Domino/System/LuaLibraries/LuaLibStateless.lua` = `6FD38B2074BAB4CB`).
- **В одиночной сборке у движка уже есть 4 слота игроков.** Бокс `Domino/System/Player/GetPlayerFromSlot` лежит в `fc3_main` и побайтно совпадает с боксом из `multicommon`. `LuaLibCoop:GetNrOfPlayers()` вызывает родную `GetNumberOfPlayers()` и превращает 0 или 1 в 2; `SetNrOfPlayersOverride` принимает 2..4. Боксы `GetPlayers` регистрируют `OnSpawnPlayer_0..3` через `CScriptCallbackSystem_GetInstance():RegisterGenericCallback`. `AllPlayers/TeleportPlayer` вызывает родную `CoopTeleportPlayer(игрок, цель)`.
- **Идентификаторы сущностей в Lua — десятичные строки** (`sprintf("%llu")` от 64-битного числа); неверный идентификатор даёт `GetInvalidEntityId()`.
- **Родные Lua-функции в `FC3.dll`** (таблица регистрации около `0x11b1d252`..`0x11b1df2e`; колбэк, который кладётся в стек прямо перед `mov edi, <строка имени>`, относится к этому имени; имена с `0x1206ad0c`: `SpawnCoopAI`, `GetSlotForPlayer`, `GetPlayerInSlot`, `CoopGetCharacterName`, `GetNumberOfPlayers`, `CoopDoGetPawnState`):
  - `GetLocalPlayerId()` = `FUN_1065ef50` -> `FUN_10c79cb0`: читает поставщика локального игрока `DAT_1280437c` (по смещению +8 объект, у которого слот vtable +0x10 возвращает указатель на 64-битный id), `-1` если его нет.
  - `GetPlayerInSlot(slot)` = `FUN_10680ef0` -> `FUN_10c73340(out, slot)`: глобальный указатель сессии `DAT_12804acc`; если байт `+0x6c` установлен, объект сессии `S = FUN_1039e1f0(*(DAT_12804acc+0x68))`; таблица слотов = `S+0xb8` (массив 32-битных дескрипторов, `-1` = пусто), размер в `S+0xbc`; дескриптор разворачивает `FUN_10260770` в объект игрока `P`, id сущности = `**(P+0xcc)`. Если слот 0 пуст, берётся `FUN_11305eb0()` (вероятно, локальный игрок одиночной игры). Остальные пустые слоты дают `-1`.
  - `GetNumberOfPlayers()` = `FUN_10681070`: `*(S+0x54)`, или 0 без сессии.
  - `CoopGetCharacterName(entity)` = `FUN_10680f90`; `GetSlotForPlayer` — соседний с `FUN_10680ef0` (не проверена); у `CoopTeleportPlayer` ссылка регистрации `0x11b1d252` (колбэк пока не разобран).
- **Вывод для выбора A/B.** Одиночная игра уже работает с объектом сессии, у которого есть таблица на 4 слота игроков. Если положить второй `P` (пешку-«куклу») в слот 1, то `GetPlayerInSlot(1)`, `GetNumberOfPlayers()` и все кооперативные боксы его увидят. Открытые вопросы: кто создаёт `S` и заполняет слоты в одиночной сборке, равно ли там `S+0x54` единице, как создать удалённую пешку (`SpawnCoopAI`, `OnSpawnPlayer`).
- Проверка правила пар: `GetNumberOfPlayers` -> `FUN_10681070` (без аргумента, возвращает число) и `GetPlayerInSlot` -> `FUN_10680ef0` (слот в строку id) совпадают с Lua-боксами; `FUN_10680f90` (сущность в строку) совпадает с `CoopGetCharacterName`.
- Дальше: декомпилировать `FUN_11305eb0`, `FUN_10260770`, `FUN_1039e1f0` (кто создаёт объект сессии и таблицу слотов); найти код `SpawnCoopAI` и `OnSpawnPlayer_%u` (`0x11fc1c64`); проверить `CoopTeleportPlayer`; затем небольшой тест: ASI, который пишет поддельный дескриптор в слот 1, и чтение `GetNumberOfPlayers` из Lua. (Частично сделано ниже.)

### Результаты фазы 1: сервис игроков — сетевой объект (2026-10-24)

- `S` — компонент **`CFCXPlayerService`**. `FUN_1039e1f0(контейнер)` перебирает массив компонентов (`контейнер+8`, количество `контейнер+0xc`) и возвращает тот, чей тип (слот vtable +4) совпадает со статическим типом из `FUN_1039e0d0` (`&DAT_12860b80`, имя `CFCXPlayerService`); последний индекс кэшируется в `DAT_1217a7d4`. Контейнер — `*(DAT_12804acc+0x68)`; на `DAT_12804acc` 4529 ссылок (центральный объект игры/сервисов).
- Класс **реплицируется по сети**: статический инициализатор по `0x11b6ff80` регистрирует `NetObject.FCXPlayerService` (дескриптор `0x12838424`) и сетевые сообщения `PlayerService_HidePlayerMessage` / `PlayerService_ShowPlayerMessage` (`0x1283842c`, `0x12838434`). Другие строки: `IPlayerService`, `CPlayerService`, `NetDiscoveryGroup.PlayerService`. Код сетевой регистрации: `FUN_11839b60`, `FUN_10468480`.
- Запасной вариант для слота 0, `FUN_11305eb0`, использует того же поставщика локального игрока, что и `GetLocalPlayerId` (`DAT_1280437c`), иначе статический неверный id. Значит, в одиночной игре слот 0 — локальный игрок даже при пустой таблице слотов.
- **Open World Co-op от scubrah (вторичные источники; страницы Nexus без входа отвечают HTTP 403):** онлайн-кооператив на карте одиночной игры с ИИ врагов, транспортом, аванпостами и всей картой, максимум игроков поднят до 8, сюжетных миссий нет, загрузка около 2 ГБ, альфа (2020-07-10). Источники: https://www.dsogaming.com/news/there-is-now-a-co-op-mod-for-far-cry-3-that-you-can-download/ , https://vgtimes.com/games/far-cry-3/files/24390-co-op-mode.html . По размеру похоже на перепакованный архив мира (остров как кооперативная карта), то есть собственная кооперативная сетевая сессия движка, запущенная на острове.
- **Рабочий вывод (пока не решение):** транспорт **A**, вероятно, жизнеспособен: кооперативная сессия движка уже работает на острове (scubrah), а сервис игроков, слоты и кооперативный Lua API есть в той же DLL. Не хватает сюжета: графы Domino одиночной игры используют `GetLocalPlayer` и ничего не знают о других игроках. Риск из PLAN.ru.md остаётся: как сегодня находить кооперативные сессии и подключаться к ним (может требоваться подбор игроков Ubisoft).
- Дальше: владелец скачивает мод 220 с Nexus (нужен вход) в `Downloads\fc3-open-world-coop\`, чтобы распаковать его архивы и сравнить с оригиналом; найти, как кооперативная сессия выбирает мир (строки `multicommon`, `fc3_main`, список кооперативных карт); найти путь подключения (прямой IP или LAN против сервисов Ubisoft). (Путь подключения: см. ниже.)

### Результаты фазы 1: сетевой транспорт и путь подключения (2026-10-24)

- **Сетевая библиотека: Ubisoft Storm** (`storm::echo::session`, `storm::echo::player`, `storm::echo::lan`, `storm::punch`). У сессий есть миграция хоста, билеты, запросы на вход, маршрутизация пиров через хост и обход NAT (`punchclient`, `punchtraversal`, `Storm NAT Client`).
- **Онлайн-сервисы: Ubisoft RendezVous** (`CGameMatchmakingProvider_RdV`, `CRendezVousInitPunchOperation`, `onlineconfigservice.ubi.com`, `/OnlineConfigService.svc/GetOnlineConfig?onlineConfigID=`). Меню игры использует этот путь. Работает ли он сегодня, неизвестно; быстрая проверка в игре — открыть кооперативное меню и создать закрытый матч.
- **LAN в движке есть:** `storm::echo::lan::inEvent::DiscoverSession`, `BroadcastLanDiscovery2`, `lan::outEvent::DiscoveredSession`, `lan::inFunc::GenerateSessionId`; настройки Storm `LanMatchmakingReuseAddress` («SO_REUSEADDR, чтобы несколько приложений хостили сессии на одной машине»), `LanMatchmakingDiscoveryOnLoopbackEnable` («искать сессии только на этой машине»), `LanMaxUploadInbps` / `LanInitialUploadInbps` / `LanMinUploadInbps`, а также `DisableRouting`, `ForceRouting`, `SessionJoinTimeoutInMs` и другие таймауты (строки около `0x11fb6f00`). В меню настроек есть «LAN Match Making Port» (`OPTIONS_NETWORK_LANMATCHMAKINGPORT`), но режима игры по LAN в английских строках интерфейса не найдено. Как игра переключается на LAN, пока не найдено.
- **Конфиги движка:** движок читает пары `engine\settings\Default<X>Config.xml` и `engine\settings\Override<X>Config.xml` для X = Engine, Network, EngineServices, Threading, Render, Sound, Game, Realtree (+ `ProfileSpecific*GameConfig.xml`, `*engineconfig_legacy.xml`). Файлы по умолчанию есть в `common` (по хешу): `DefaultNetworkConfig.xml` = `common\__UNKNOWN\misc\02E1D1848EC84585.Profile.xml`, `DefaultEngineConfig.xml` = `4C5BBCC4AC9D8E00`, `DefaultGameConfig.xml` = `50C61851314582D5`, `DefaultEngineServicesConfig.xml` = `5FDA7AF3F9DB5B99`. **Файлов `Override*` в игре нет**, поэтому наш архив может добавить `OverrideNetworkConfig.xml`, не заменяя файл игры.
- `DefaultNetworkConfig.xml`: `EnableNatTraversal=1`, `SecureSocketEnabled=1`, `MainSocketEncrypted=1`, `MaximumPeerCount=25`, `MigrationEnabled=1`, `OnlineEnginePort=9000`, `OnlineServicePort=9001`, `FileTransferHostPort=9002`, `FileTransferClientPort=9003`, `LanHostBroadcastPort=9004`, `LanClientBroadcastPort=9005`, `ScanFreePorts=1` с 9000 на 1000 портов; фильтры сетевого журнала (в том числе `session`, `lanmanager`) и **профили эмуляции сети** `moderate` / `high` (потеря, задержка, перестановка пакетов), включаются через `EmulatorEnabled` + `EmulatorProfile`. Полезно для проверки задержек на одном ПК.
- **Параметры подключения:** `FUN_11342740` регистрирует поля структуры параметров сессионной операции: `SearchIdAddress` (+0x8), `SearchSessionId` (+0x10), `JoinSessionId` (+0x18), `SessionRefId` (+0x20), `DeleteSessionId` (+0x28), `PlaylistId` (+0x34), `SessionType` (+0x38), `NetEngineType` (+0x3c), `AccountType` (+0x40), `SearchRetries` (+0x44), `SearchSlots` (+0x48). Рядом строки: `JoinGame`, `JoinService`, `MatchMakingEnabled`, `fullNetworkInitialization`, `isInEditor`, `execLines`, `mapIdLow` / `mapIdHigh`, `saveGameFilename`. Состояния сетевого движка: `ENetEngineState_Initialized_Offline` / `_Online`. Операции: `CFCXCreateSessionOperation`, `CFCXJoinSessionOperation`, `CFCXNetEngineStartupOperation`, `CSessionNetEnginePlayerAddOperation`.
- Найден флаг командной строки: `-enablenetobjectstats`.
- **Что это значит для плана.** У транспорта A два возможных пути подключения: (1) LAN-обнаружение Storm, выведенное в интернет так же, как в моде Far Cry (2004) (ретранслятор через localhost плюс приглашение Steam); (2) онлайн-путь Ubisoft, если его сервисы ещё работают. Оба нужно проверить в игре. Неизвестно, что включает LAN (значение `NetEngineType`, опция конфига или ветка кода).
- Дальше (статика): найти код, который читает `NetEngineType` (смещение +0x3c), и создателя LAN-менеджера (`lan::inEvent::SetDiscoveryHandler`); (в игре, ПК владельца): проверить, доходит ли штатное кооперативное меню до сервисов Ubisoft.
- **Обновление 2026-10-24 (владелец):** онлайн-сервисы Ubisoft для Far Cry 3 отключены. Путь через RendezVous отпадает; транспорт A должен идти через LAN Storm (выведенный в интернет нашим ретранслятором), иначе откат на B. Ссылки на `LanMatchmaking*` и типы событий `lan` по `0x1070de..` — только таблицы регистрации типов; код, запускающий LAN-обнаружение, пока не найден.

### Инструменты: упаковщик архивов и исправление распаковщика (2026-10-24)

- **Список архивов, которые грузит движок** (`0x120701c4`): `patch.dat`, `patch_%lang%.dat`, `patch_sound_%lang%.dat`, `common.dat`, `sound.dat`, `sound_%lang%.dat`, `ige.dat`, `igepatch.dat`, `shadersobj.dat`, `worlds/fc3_main/fc3_main{,_%lang%,_vistas}.dat`, `worlds/multicommon/multicommon{,_%lang%,_vistas}.dat`; ещё строки `_2.dat`, `subfat_%s.fat`, `mergedBigFileName` (смысл неизвестен). Значит, мод поставляет пересобранный `patch.dat/fat` = штатное содержимое patch плюс наши файлы (так же был устроен мод владельца на пропуск роликов).
- **Устройство FAT v9 (проверено на штатных `patch.fat` и `common.fat`):** заголовок 24 байта `FAT2`, версия 9, флаги `0x301` (платформа 1 = Windows, версия сжатия 3), `u0C = u10 = 0`, число записей; записи по 20 байт, отсортированы по хешу; затем 8 нулевых байт. У несжатых записей схема 0, `UncompressedSize = 0`, размер в `CompressedSize`. Смещения данных в штатных файлах не выровнены.
- **Упаковщик:** новый проект `Gibbed.FarCry3.Pack` (`bin_fc3\Gibbed.FarCry3.Pack.exe output.fat input_dir`, файлы без сжатия). Изменения в Gibbed: `BigFileV2.Serialize` реализован только для v9; `Pack` принимает версию и версию сжатия по умолчанию (9, 3 для FC3); файлы из `__UNKNOWN` теперь сохраняют 16-значный хеш (старый код обрезал до 8 цифр, что годится только для 32-битных хешей). LZO-сжатие в исходном проекте по-прежнему не реализовано (не нужно).
- **Ошибка распаковщика, исправлена:** распаковщик читал несжатые записи по `UncompressedSize`, а в FC3 там 0, поэтому каждый несжатый файл записывался **пустым** в `__UNKNOWN\null\`. Затронуто в результатах фазы 1: `patch` 4 файла (два видео `.bik` по 136 МБ и 63 МБ, два материала), `common` 429, `multicommon` 17108, `fc3_main` 11799 файлов и 16 файлов в `named\`. Исправлено в `EntryDecompression.cs` и в определении типа в `Unpack.cs`; все четыре архива распакованы заново с исправлением: `patch` 427, `common` 3099, `multicommon` 122255, `fc3_main` 64354 файла, пустых файлов нет; `names.tsv` и `named\` пересобраны (45045 имён, 0 пустых текстовых файлов). Вспомогательные скрипты (хеширование имён, разбор FAT, перераспаковка): `scripts\`.
- **Круговая проверка:** штатный `patch.fat` распакован, упакован снова и снова распакован: 427 из 427 файлов совпали (MD5).
- Для сборки нужен локальный кэш NuGet (про мод scubrah и версию 1.05 см. ниже): `$env:NUGET_PACKAGES='E:\FarCryCoop\Data\nuget'`, `dotnet build ... --source E:\FarCryCoop\Data\nuget` (`env.ps1` его не задаёт). Файл патча: [docs/gibbed-dunia-fc3.patch](docs/gibbed-dunia-fc3.patch).

### Open World Co-op от scubrah, patch 1.5 (2026-10-24)

- Владелец скачал в `Downloads\fc3-open-world-coop\` (Nexus, мод 220). Readme: ставить поверх игры; «создать закрытую кооперативную сессию в главном меню и сменить главу на ROOK ISLANDS / fc3_main»; одиночная игра и штатные кооперативные карты при установленном моде не работают. Закрытая сессия идёт через онлайн-сервисы Ubisoft, а они отключены (владелец, 2026-10-24), поэтому в исходном виде мод не запустится.
- **Его DLL — другая сборка движка:** `FC3.dll` 29994856 байт, время PE 2018-05-02, PDB `E:\Work\FC3_Chinese\sub-pc\bin\FC3.pdb` (у нас: 2022-10-03, `F:\Projects\FC3\bin\FC3.pdb`, 40720424 байта). Вариант `No_Minimap` отличается 6 байтами данных (`A`/`H`/`R` -> `X` по смещениям файла `0x159c8b4`..`0x15cfbf6`). Остальные правки DLL мода точно найти можно только сравнением с чистой DLL той же сборки.
- **Версии:** нынешняя сборка Steam (1.06, 2022) без мультиплеера; 1.05 — последняя версия с ним (гайды сообщества). Манифест депо 220241 для 1.05 по гайду Steam: `7362101836779063707` (ещё не проверен). Мод Nexus 262 «Solo Offline Coop Game Mode» (на базе 1.05) запускает кооперативный режим офлайн и служит образцом запуска сессии без серверов.
- **Архивы:** `patch.fat` мода — 3316 записей (оригинал 427): 2864 мировых сектора `.fcb` (`WorldSector` / `MissionLayer` / `Entity`), 404 Lua (основные миссии 150, `fc3_master` 91, DLC 66, `lib_*`), конфиги. `fc3_main_vistas` мода — 129894 записи, 3 ГБ (оригинал 13261, 150 МБ); около 100000 совпадают по хешу с `multicommon` и 25000 с `fc3_main`, 14309 новых. Распаковано в `Work\fc3\mod\`; штатный vistas в `Work\fc3\fc3_main_vistas\`. Полное сравнение содержимого: `Work\fc3\mod\compare.tsv` (скрипт `scripts\contentcompare.py`).
- **Конфиги мода:** `DefaultNetworkConfig.xml` включает эмулятор сети с новым профилем `upstream32k`, где `upload_bandwidth bits_per_sec="0"` (без потерь и задержек), ставит `ObjectTrafficStatsEnabled=1` и удваивает лимиты отдачи до 20480000. Два `Profile.xml` движка увеличивают кольца стриминга (`RingData="15,16,10,20,1"`, `RingLod="10,12"`) и лимиты физики. Автор пользовался FCBConverter (ArmanIII) для `.fcb` <-> XML.
- **План:** скачать чистые `bin` и `patch` версии 1.05 через DepotDownloader 3.4.0 (`Tools\depotdownloader-3.4.0`, SHA-256 архива `41C9E9F0DF54B3AD02E67A11726756E5C73283BD7C2E1B04ACFA5AE4C2ED3767`, размер сверен с релизом GitHub; exe без подписи) в `Downloads\fc3-1.05\`, список файлов `scripts\fc3-105-filelist.txt`; запускает владелец со своим входом в Steam. Затем сравнить чистую DLL 1.05 с DLL мода и получить точные правки мода.

### Чистая 1.05 и точные правки DLL мода (2026-10-24)

- Владелец скачал DepotDownloader-ом (вход `-qr`) в `Downloads\fc3-1.05\`: `bin\` и `data_win32\patch*`, `igepatch*`. `FC3.dll` 1.05: 29994856 байт, SHA-256 `98e34155266de3627202e520bff92b2be17a21ee3f89d9a6213915b83e9e1166`, время PE 2018-05-02, GUID PDB `b0ba4c20...` = та же сборка, что DLL мода. Значит, манифест `7362101836779063707` — это 1.05 и основа мода scubrah. В `bin` версии 1.05 ещё есть `uplay_r1_loader.dll` и `ubiorbitapi_r2_loader.dll`.
- **Решение владельца:** поддерживать и 1.05, и 1.06 (PLAN.ru.md).
- **Правки DLL от scubrah = 90 байт в 45 местах**, одинаковые в `FC3.dll` и `FC3_d3d11.dll` (скрипт `scripts\patchctx.py`; адреса для `FC3.dll` 1.05):
  - `.rdata` `0x11535dd0`: строка `mpdlc1_aegir` -> `fc3_main`. Имя карты мультиплеерного DLC заменено на мир одиночной игры — так остров появляется как глава кооператива.
  - границы мира для сетевого квантования: блок по `0x11535bb0` и отдельные float: +/-1536 -> +/-5120, +/-1029 -> +/-5120, 1535.99/2047.99/1279.99 -> 5119.99, 127.99/255.99 -> 511.99/512, 129 -> 512, 153600 -> 512000, 12900 -> 51200, 1.536e6 -> 5.12e6. Кооперативные карты примерно 3 км в ширину, остров около 10 км.
  - код: три помощника сериализации `push 8` -> `push 9` (`0x1027b5da`, `0x1027b5fb`), `push 6` -> `push 7` (`0x1027b627`, `0x1027b648`), `push 7` -> `push 8` (`0x10592f7a`, `0x10592f9b`): по одному биту больше в парах (запись и чтение), под больший диапазон координат.
  - имена, переименованные, чтобы выключить функции (`F`/`C` -> `X`): свойства смены FOV камеры (`fBlendOutFOVTime` и другие), `CRadioTowerPanelComponent`, `CoopNameplatesUI` / `CFCXCoopNameplatesUI` (readme: «Removed NPC tag icon»). Вариант `No_Minimap` добавляет ещё 6 таких байт.
- **Переносимость на 1.06:** якоря данных в DLL 1.06 есть (`mpdlc1_aegir` по смещению файла `0x1f893b4`, блок границ по `0x207b8e0`, `CoopNameplatesUI`, `fBlendOutFOVTime`), но три шаблона кода сериализации побайтно не совпадают; нужно сопоставление функций.
- **Архивы мода по содержимому** (`Work\fc3\mod\compare.tsv`, сравнение с данными 1.06): `fc3_main_vistas`: 115585 файлов побайтно совпадают со штатными (`multicommon`, `fc3_main`, vistas) плюс 14309 новых; `patch`: 396 совпадают, 64 изменены, 2856 новых (2827 мировых секторов `.fcb`). Являются ли новые файлы штатными под новыми именами — проверяется (`dupcheck.py`).
- **Скрипт карты адресов** `scripts\fc3_addrmap.py` (без Ghidra, секунды): находит строки-якоря, ссылки из кода, зарегистрированный колбэк и глобалы, которые колбэк читает. Понимает оба вида регистрации: 1.06 `push 0; push CB; call; mov edi, имя` (колбэк перед именем) и 1.05 `push имя; push esi; push edi; call; push ebx; push CB; push edi; call` (колбэк после имени). Самопроверка по вручную подтверждённым значениям 1.06 (`--selftest`): проходит. Результат: `Work\fc3\addrmap-105.tsv`, `addrmap-106.tsv`.
- Колбэки 1.05, **подтверждённые декомпиляцией** (снимок REA `E:\FarCryCoop\Data\rea-snapshots\fc3-105-FC3.dll.snapshot`): `GetPlayerInSlot` `0x10bad843` (целый слот -> 64-битный id), `GetNumberOfPlayers` `0x10baba48` (без аргумента -> число, через `FUN_10bca625`), `CoopGetCharacterName` `0x10badb62` (аргумент `EntityId`), `CoopDoGetPawnState` `0x10bad8b5` (целый аргумент -> число); из скрипта, ещё не декомпилированы: `GetLocalPlayerId` `0x10bb3f60`, `GetSlotForPlayer` `0x10bad7cb`, `CoopTeleportPlayer` `0x10bac7d0`, `SpawnCoopAI` `0x10bae756`, `GetInvalidEntityId` `0x10bb7709`. В обеих сборках одинаковая раскладка: колбэк кладётся в стек прямо перед именем (прежнее правило «колбэк после имени» для 1.05 было неверным и убрано из скрипта; `--selftest` теперь проверяет обе сборки).

### Мод Nexus 262 «Solo Offline Coop Game Mode» (2026-10-24)

- Владелец скачал в `Downloads\fc3-solo-offline-coop\`: `FC3.dll`, `FC3_d3d11.dll`, `patch.dat/fat` (дата 2022-07-20), сделан на 1.05.
- **DLL: всего две правки**, обе заменяют `call 0x108c4b09` пятью NOP (в `FC3.dll` 1.05 по `0x108c4c6b` и `0x108c5c36`; в `FC3_d3d11.dll` смещения файла `0xa1e1e2`, `0xa1f1ad`). Контекст: `74 07 8b ce e8 ..` и `80 7d ff 00 74 07 8b cb e8 ..` (вызов только при установленном флаге). Гипотеза: проверка, которая выводит из сессии при отсутствии онлайн-сервисов. Подтвердить декомпиляцией (анализ 1.05 в REA идёт).
- **patch: штатный patch 1.05 (427 файлов, без изменений) + 4 файла:** переопределения из `common` `9B788D77FCFA35BB.fcb` (игровая база, 1040772 -> 2617955 байт; новых строк всего 12, значит в основном числовые и структурные изменения) и `E88B7431DC8E7DBF.feu` (интерфейс главного меню в контейнере Ubisoft `UEF`, +3 КБ); `36A67CE0D3C22326.xml` (раскладка управления, тот же файл есть у scubrah); один Lua `coop_vidar_defend` «AllInRoom» с закомментированным роликом закрытия двери.
- Дальше: декомпилировать `0x108c4b09` и два места вызова в 1.05; распаковать меню `UEF` (JPEXS или свой разборщик), чтобы увидеть изменение меню; перевести `.fcb` через FCBConverter, чтобы увидеть изменение базы.
- **FCBConverter** (Jakub Marecek, ник ArmanIII; исходники https://github.com/JakubMarecek/FCBConverter, сборка 20230711-1830 с https://downloads.fcmodding.com/files/FCBConverter.zip, хеш автор не публикует; SHA-256 архива `803B04CC9DA0D40BF2CB390C1D1C0EB26D6AFFDAD1E6D4E0360EED8E63B3BDCE`) в `Tools\fcbconverter-20230711\FCBConverter\`. Поддерживает FC3 (FAT v9). Нужен .NET 7; работает на нашем переносимом .NET 8 с `DOTNET_ROOT=E:\FarCryCoop\Tools\dotnet-sdk-8.0.426` и `DOTNET_ROLL_FORWARD=Major` (без установки на C:). Запуск: `FCBConverter.exe -source=<file.fcb>` пишет `<file>.converted.xml`. В комплекте `FCBConverterFileNames.list` (4189677 имён файлов) и `FCBConverterStrings.list` (483419 строк), по ним можно назвать больше наших хешей.
- **Изменение базы в моде 262** (`9B788D77FCFA35BB.fcb`, переведено в `Work\fc3-105\fcb\`): всего два поля в плейлисте **«Local Coop»** (`id` 0xFFFFFFFC, `sessionType` 3, `maxPlayersInRound` 2): `minPlayersInRound` 2 -> 1 и `matchCountdownInSecs` 0 -> 10. Бинарник больше только из-за способа записи конвертером. Плейлисты в штатной базе: `sessionType` 2 = User (12), Beta Test (14), Map Archive (14), Private Coop (4); `sessionType` 3 = Local Coop (2). Значит, мод 262 запускает штатный тип сессии «Local Coop» (вероятно, консольный split-screen) с одним игроком, без серверов. Это офлайн, а не сетевая игра, но видно, что кооперативный мир стартует без сервисов Ubisoft.
- **Что за новые файлы мода scubrah** (`dupcheck.py`: из 17165 «новых» только 6 — штатные файлы под другим именем): 2827 новых `.fcb` в `patch` — мировые секторы с собственными сущностями автора `CoopAI_EncounterPawn_<случайное>` (кооперативные ИИ-противники, с точками укрытия) и `CoopVehicle_<случайное>` (транспорт с вариантами разрушения); случайные суффиксы значат, что их сгенерировал инструмент. То есть остров заселён кооперативными спавнерами ИИ и машинами вместо скриптов одиночной игры. 14309 новых файлов vistas начинаются с `04 4B 50 53` (вероятно, звуковые пакеты `.spk`). Образцы: `Work\fc3\mod\sample\`.
- **Больше имён файлов:** `scripts\names_ext.py` хеширует 4189677 путей из `FCBConverterFileNames.list`: +84934 имени (из 192788 файлов с хешем в штатных архивах и архивах мода, без имени осталось 62809). Результат `Work\fc3\names_ext.tsv` (столбец `source` = manifest или fcbconverter); ещё 1395 Lua/XML скопированы в `named\`. Ключевые имена: `9B788D77FCFA35BB` = `generated\nomadobjecttemplates.fcb` (шаблоны объектов, включая плейлисты), `E88B7431DC8E7DBF` = `ui\multiplayer\menu\multimain.feu` (главное меню мультиплеера, его меняет мод 262), `8BDCD85D74264EB2` = `engine\settings\pc\defaultengineconfig.xml`, `36A67CE0D3C22326` = `actionmaps\inputactionmapcommon.xml`; секторы scubrah — `worlds\fc3_main\levels\<клетка>\generated\worldsectors\worldsector_<n>.data.fcb` (2834 файла, в штатных архивах их нет).
- **Правки DLL мода 262 выключают «сессия завершена: мало игроков» (декомпилировано, 1.05):** `FUN_108c4b09` сбрасывает `[this+0x180]`, создаёт окно-сообщение на 0xE4 байта (vtable `0x1156d990`) с текстом из таблицы id строк по `0x115ac030` = {167676, 167677, 167678} = `*_END_OF_GAME_INSUFFICIENT_PLAYERS` («The game session ended prematurely because of an insufficient number of players.») и заголовком 160927 («Alert»; его enum — `MP_LinkApp_Passport_Locked_Alert`, но строка просто используется как общий заголовок), затем отправляет событие типа `DAT_118644bc` (завершение сессии). Вызовы в 1.05: `FUN_108c4c48` по `0x108c4c6b` (`if (тип события == DAT_118644bc && [this+0x180]) ...`) и `0x108c5c36` (после цикла по игрокам, если флаг остался 0). Это ровно те два вызова, которые мод 262 заменяет NOP. **Исправление:** раньше здесь было написано, что это проверка Uplay Passport — по имени enum заголовка; это было неверно, проверкой лицензии это не является.
- **Та же функция в 1.06:** таблица id строк по `0x11ffac34`, ссылка из `0x10ff276f`, начало функции `0x10ff2750`; три вызова: `0x10fddbc8` и `0x10ff3be4` (`cmp eax,[0x1283a184]; jne; cmp byte [reg+0x180],0; je; push reg; call`, проверка типа события) и `0x10ff2adc` (`cmp byte [esp+0x13],0; je; push edi; call`, после цикла по игрокам). Подтверждено декомпиляцией 1.06: `FUN_10ff2750(param)` сбрасывает `[param+0x180]`, берёт текст из `0x11ffac34`, заголовок `0x2749F`, отправляет событие типа `DAT_1283a184`; объект передаётся через стек (отсюда `push reg` в местах вызова). Снимки REA: `E:\FarCryCoop\Data\rea-snapshots\fc3-105-FC3.dll.snapshot` (38 КБ) и `fc3-106-FC3.dll.snapshot` (11 КБ); они маленькие, поэтому, вероятно, хранят только доказательства и пометки, а не базу Ghidra; избавляет ли открытие с ними от повторного анализа, пока не проверено.
- **Решение владельца 2026-10-24: завершение «сессия окончена: мало игроков» в нашем моде выключается** (как в моде 262), чтобы сессия шла и с одним игроком. Владелец одобрил это, когда считалось, что это проверка Passport; после декомпиляции оказалось, что это правило числа игроков, а не проверка лицензии. Собственная защита игры (запуск через Ubisoft Connect с лицензией на аккаунте) не трогается; каждому игроку нужна своя копия.

### Прототип v0: одиночная кооп-сессия без серверов, собранная нашими инструментами (2026-10-24)

- **Тестовые копии** (`scripts\build_testgame.ps1 -Version 1.05|1.06`): `E:\FarCryCoop\Work\game-105` и `game-106`. `bin\` — настоящая копия (1.05 из `Downloads\fc3-1.05`, 1.06 из установки); в `data_win32\` 94 жёсткие ссылки на архивы установки (тот же диск, места не занимают) и настоящие копии `patch*` / `igepatch*`. Файл-ссылку нельзя менять на месте, только заменять новым файлом. `Support\` не копируется. Установка проверена после: хеши `FC3.dll` и `patch.fat` не изменились. Штатный `patch.fat` одинаков в 1.05 и 1.06.
- **Патчер DLL** `scripts\patch_dll.py`: находит функцию «мало игроков» по её таблице id строк и заменяет каждый её вызов NOP; работает на всех четырёх DLL; отказывается работать с путями внутри установки Steam; `--selftest`: пропатченные `FC3.dll` и `FC3_d3d11.dll` 1.05 побайтно равны файлам мода 262, повторный запуск ничего не меняет. Применён к тестовым копиям: 1.05 `FC3.dll` функция `0x108c4b09` (вызовы `0x108c4c6b`, `0x108c5c36`), `FC3_d3d11.dll` `0x10a1ec80` (`0x10a1ede2`, `0x10a1fdad`); 1.06 `FC3.dll` `0x10ff2750` (`0x10fddbc8`, `0x10ff2adc`, `0x10ff3be4`), `FC3_d3d11.dll` `0x10ff7f90` (`0x10fe33f8`, `0x10ff831c`, `0x10ff9424`).
- **Правка базы** `scripts\edit_playlist.py` по XML FCBConverter для `generated\nomadobjecttemplates.fcb`: плейлист «Local Coop» `minPlayersInRound=1`, `matchCountdownInSecs=10`; обратно через FCBConverter: побайтно совпадает с файлом мода 262 (SHA-256 `9A3C6D8D...`).
- **Архив patch** `scripts\build_patch.ps1` -> `Work\mod-build\patch.dat/fat`: штатный patch (427 файлов) + наш `generated\nomadobjecttemplates.fcb` + **только для теста, чужой** `ui\multiplayer\menu\multimain.feu` из мода 262 (не наш, в релиз не идёт; заменить своей правкой меню). Проверка распаковкой: 429 файлов. Установлен в обе тестовые копии.
- Дальше: владелец запускает `Work\game-105\bin\farcry3.exe` (потом 1.06) и пробует меню мультиплеера -> Local Coop одним игроком. Ожидается: кооперативная карта (пока без острова) грузится в одиночку без окончания «insufficient number of players». Тестовая копия 1.05 использует архивы данных 1.06 (из 1.05 только `bin` и `patch`); если не загрузится — скачать депо 1.05 целиком.

### Тесты прототипа v0 на ПК владельца (2026-10-24)

- **1.06** (`Work\game-106`, наш мод): запускается; кнопка «Мультиплеер» сразу показывает «The online multiplayer service is not available anymore.» Откуда: `FUN_11751440` (получение строки по id, 1.06) возвращает зашитый текст UTF-16 (английский по `0x11f17378`, переводы рядом, выбор в `FUN_11750980`), когда id строки = `0x23894` = 145556 = `X360_EU_NETWORK_XLSP_DOWN` («The Far Cry 3 service is not available. Please try again later.»). То есть 1.06 по-прежнему пытается подключиться к онлайн-сервису Ubisoft при входе в мультиплеер; Ubisoft поменяла только текст. Чтобы попасть в кооп-сессию в 1.06, этот путь онлайн-входа нужно обойти в коде.
- **Запуск 1.05:** `farcry3.exe` (DX9) в полноэкранном режиме даёт чёрный экран (при `UseD3D11` 1 и 0 в профиле); `farcry3_d3d11.exe` работает в полноэкранном; `farcry3.exe` работает в окне (`Fullscreen="0"`). Профиль `C:\Users\<user>\Documents\My Games\Far Cry 3\GamerProfile.xml` общий для 1.05 и 1.06. С разрешения владельца для тестов изменён: `UseD3D11="0"`, `Fullscreen="0"`; копия оригинала: `E:\FarCryCoop\Work\fc3-profile\GamerProfile.xml.bak` (вернуть после тестов). Логов игра туда не пишет.
- **Данные 1.05:** манифест депо 1.05 (`Downloads\fc3-1.05-manifest\`, 326 файлов, 2018-05-21) совпадает с установкой 1.06, кроме `bin\` и двух лишних файлов 1.05 `worlds\multicommon\multicommon_japanese.dat/fat` (скачаны, теперь в `game-105`; `build_testgame.ps1` их копирует). 8 отличий `sound_*.dat` — ложное срабатывание (пустые файлы; манифест пишет для них нулевой SHA-1).
- **1.05 с модом:** чистая 1.05 показывает только «online co-op»; с нашим patch (и с точным patch мода 262) в меню Coop появляется **«Offline coop»**, но его запуск **вылетает**: нарушение доступа в одной и той же функции в обеих сборках: `FC3.dll` `FUN_10622c58`+0x27 (`0x10622c7f`, DX9) и `FC3_d3d11.dll` `0x1077e1bd`+0x27 (`0x1077e1e4`). Код обходит массив: `mov esi, <результат вызова>; cmp [esi], 0; jbe ...; mov eax, [esi+4]; mov eax, [eax+ebx*4]` -> число элементов > 0, а указатель на данные битый. Причина не в наших инструментах (точный patch мода 262 падает так же), не в японском архиве и не в рендере. Декомпилирую `FUN_10622c58` (анализ 1.05 в REA идёт).
- **Вылет Offline coop: причина и исправление (2026-10-25).** `FUN_10622c58` (1.05) обходит список, который возвращает `FUN_10622144`, а та вызывает `UPLAY_PARTY_GetFullMemberList(this+0xc0)`; нынешний Ubisoft Connect больше не обслуживает API групп Uplay, вызов падает с ошибкой (`UPLAY_GetLastError`), в списке остаётся мусор (число > 0, битый указатель) и цикл падает. Прямых вызовов `FUN_10622c58` нет (вызывается через таблицу). Исправление в `patch_dll.py`: `jbe`, который пропускает пустой список, становится `jmp` (один байт, `76 3C` -> `EB 3C`), ищется по шаблону `call; mov esi,eax; xor ebx,ebx; cmp [esi],ebx; jbe +3c; mov eax,[esi+4]; mov eax,[eax+ebx*4]`: 1.05 `FC3.dll` `0x10622c7a`, `FC3_d3d11.dll` `0x1077e1df`; в 1.06 шаблона нет (новый API UPC). Самопроверка: наши DLL 1.05 = DLL мода 262 + ровно этот байт.
- **Прототип v0 работает на 1.05 (тест владельца, 2026-10-25):** `Work\game-105` с нашим архивом patch и пропатченными DLL: меню Coop -> «Offline coop» -> кооперативная карта загружается с одним игроком, без вылета и без «insufficient number of players». Тестовый профиль на C: всё ещё в оконном DX9 (копия в `Work\fc3-profile\`).

### Сессионный слой PC 1.05: LAN-провайдера нет (2026-10-25)

- Флаги аккаунта (`FUN_110d39c8`): `Offline` 1, `LAN` 2, `Live` 4, `Gator` 0x20, `Rendezvous` 0x40. Поля операции входа (`FUN_104c0f03`): `login` (+4), `password` (+8), `onlineKey` (+0xc), `guestlogin` (+0x10), `primaryAccount` (+0x14), `accountFlags` (+0x18), `controllerIndex` (+0x1c), `silentLogin` (+0x1d), `isPrimaryIndex` (+0x1e), `platformSpecificId` (+0x20).
- В сетевом конфиге (`FUN_1024dbef`, профиль `NetworkProfile` / `engine\settings\*NetworkConfig.xml`) есть список `Accounts` из `Account` (`CAccountConfig`, смещение 0x6c) и строка `SessionProvider` (0x64). Тест с разрешения владельца: в `GamerProfile.xml` добавлено `<Accounts><Account login="Host" accountFlags="2" .../></Accounts>` (копия `Work\fc3-profile\GamerProfile.xml.before-lan`): игра запускается, меню не меняется.
- Причина: все игровые онлайн-провайдеры в PC-сборке есть только в вариантах `_RdV` (Ubisoft RendezVous), `_UplayPC` или `_Win`: `CSessionLoginOperationProvider_RdV`, `CGameMatchmakingProvider_RdV`, `CGameInviteProvider_RdV`, `CGameFriendProvider_RdV` / `_UPlayPC`, `CGamePartyProvider_UplayPC`, `CGameFrontendServiceProvider_RdV` / `_Win` и другие. LAN-провайдера нет; `storm::echo::lan` в Storm есть ниже, но игровой слой PC его не использует.
- Другое найденное: ключ командной строки `-offline` (`FUN_10680f27` выбирает режим сообщений; `FUN_10922ac6`, отладочное главное меню с «COOP Story» / «Adversarial» / «UPlay» / «!What Sebra did before this ;)!», скрывает онлайн-пункты), `-splitscreen`, `-splitscreen4`, `CFCXSplitScreenService`, `CGRQueryIsLocalCoopGame`, `OfflinePlaylist`.
- **Вывод:** без сервисов Ubisoft сетевая кооп-сессия на PC требует одного из: (A1) свои замены провайдеров `_RdV` входа / подбора / приглашений в ASI (отвечать локально и создавать или подключаться к сессии Storm по адресу); (A2) локальный эмулятор сервиса RendezVous (перенаправить хосты сервиса на него); (B) свой сетевой код поверх работающей локальной сессии «Offline coop». Решить дальше.

### Быстрый запуск разработчиков: прямой хост / клиент без подбора игроков (2026-10-25)

- `FUN_10652061` (1.05, один вызов `0x10652215`) проверяет командную строку: `-host` -> `FUN_106518ba`, `-client` -> `FUN_10651d10`, `-wait`, `-quickmatch`.
- **Хост** (`FUN_106518ba`): нужен `-login <имя>`; читает `-host <значение>` (карта), `-bfname`, `-dedicated`, `-sessionuid <u64>`, режим `-deathmatch` / `-coop` / `-domination` / `-headquarters` / `-firestorm` / `-dash`, `-class`, `-spawn`, `-spawnpos x,y,z`, `-spawnangle`, `-netemulator <профиль>`, `-ranked`. Тип сессии = **3 без `-online`** (тот же локальный тип, что у плейлиста «Local Coop»), 2 с `-online`.
- **Клиент** (`FUN_10651d10`): нужны `-login <имя>` и `-ip <адрес>` («Missing -login or -ip for -client.»); `-online` дополнительно требует `-password`; для `localhost` своя ветка; те же режимы, `-sessionuid`, `-class`, `-ranked`.
- Другие найденные ключи: `-xpos -ypos -xres -yres` (окно), `-numclients`, `-offline`, `-quickstartup`, `-skipintro`, `-world`, `-exec`, `-execline`, `-login2..4`, `-password2..4`, `-splitscreen`, `-splitscreen4`, `-netemulator`, `-cmdport`, `-logFile`, `-steam_mode`, `-fromuplay`.
- Имена кооперативных карт: `coop_vidar`, `coop_skadi`, `coop_freyja`, `coop_huginn`, `coop_sleipnir`, `coop_fenrir`; ещё `fc3_main`, `mpdlc1_aegir`, `mpdlc1_idun`.
- Доступные сессионные операции: `CSessionCreateOperation`, `CSessionSearchIPOperation`, `CSessionJoinOperation`, `CSessionStartNetOperation`, `CFCXCreateSessionOperation`, `CFCXJoinSessionOperation` и другие. Провайдер входа `CSessionLoginOperationProvider_RdV` (геттер типа `0x110cdef8`, vtable около `0x11696d98`, методы `0x110cd961`, `0x110cd71d`, `0x110ccb46`, `0x110cc8ac`) — машина состояний заданий RendezVous.
- Скрипт теста: `scripts\launch_105.ps1 -Role host|client [-Map coop_vidar] [-Ip 127.0.0.1] [-Dx9]` (два оконных экземпляра рядом; по умолчанию DX11 `farcry3_d3d11.exe`).

### Тест быстрого запуска: хост + клиент на одном ПК (2026-10-25)

- **Проверка второго запуска.** Вторая копия останавливается с «You already have a Far Cry 3 session (game or editor) active or left open after an irregular shutdown» (`ERROR_INSTANCE_MUTEX`). Код при запуске: `push 0x1f0001; call [OpenMutexA]; test eax,eax; jnz <ошибка>`. `patch_dll.py` заменяет `jnz` на 6 NOP: 1.05 `FC3.dll` `0x100041bb`, `FC3_d3d11.dll` `0x10004092`. Теперь две копии работают рядом.
- **Сетевой адрес.** Хост открывает UDP 9000 на первом адаптере, здесь это адаптер VirtualBox `192.168.56.1`, а не `127.0.0.1`; клиент открывает 9001. Клиент запускался с `-ip 192.168.56.1`.
- **Результат:** хост загружает `coop_vidar` (мир виден за меню); клиент, похоже, подключается и показывает тот же экран. Затем у обоих пустая тёмная панель без текста и кнопок; картинка не обновляется (следы курсора), ввод не работает. После нажатия кнопки «ОК» на хосте то же самое. Одинаково в DX9 и DX11, с оверлеем RivaTuner (RTSS) и без него.
- **Это не зависание:** оба процесса отвечают Windows; главный поток занимает одно ядро и выполняет разный код игры и вызовы драйвера (`scripts\thread_eips.ps1` снимает EIP потоков и ищет в стеке адреса `FC3*.dll`).
- **Подозрения / дальше:** (1) чужой тестовый `multimain.feu` из мода 262 в нашем `patch.dat` ломает интерфейс сетевого меню: повторить тест с пропатченной DLL и штатным `patch.dat`; (2) экран лобби (выбор персонажа / готовность), который ждёт ответа от отключённых онлайн-сервисов; (3) проанализировать `FC3_d3d11.dll` в REA и разобрать адреса из стека главного потока (например `0x10338ad4`, `0x102f1b1d`, `0x103e07db`, `0x1040aaa3`, `0x10327bbd`).


---

## Дозаписанные сессионные заметки, 2026-11-02 (агентская сессия на этой машине)

Эти разделы написаны в агентской сессии. Материал Phase 1 здесь не повторяется:
первоисточником по Phase 1 остаются разделы владельца выше («Фаза 1: … и т.д.).
Новые находки переносятся и в английскую, и в русскую версию — правило двуязычия
записано ниже в AGENTS.md.
### Ghidra-адреса из 1.05 `FC3.dll` (заметки Phase 1; против 1.06 не перепроверены)

Это имена, которые Ghidra присвоил функциям 1.05-исполняника. Это единственное место, где
записаны хендлеры загрузчика и точки входа сервисов игроков, поэтому они сохранены даже тогда,
когда файла 1.05 на машине нет. Все адреса — в `FC3.dll` (1.05).

| Элемент | 1.05 адрес | Роль |
| --- | --- | --- |
| хендлер `-host` | `FUN_106518ba` | запуск host-сессии из командной строки |
| хендлер `-client` | `FUN_10651d10` | запуск client-сессии из командной строки |
| хендлер `-offline` | `FUN_10680f27` | включает отладочное меню (пункт COOP Story) |
| строка `GetLocalPlayerId` | `0x11fbdcc4` (xref `0x111ce912`) | имя net-поля сервиса игроков |
| `GetNumberOfPlayers` | `FUN_10681070` | количество игроков |
| `GetPlayerInSlot` | `FUN_10680ef0` | слот → id игрока |
| `CoopGetCharacterName` | `FUN_10680f90` | имя игрока |
| xref `CoopTeleportPlayer` | `0x11b1d252` | место вызова телепорта |
| поиск компонента | `FUN_1039e1f0` (тип из `FUN_1039e0d0`) | получить net-компонент по типу |
| фолбэк слота 0 | `FUN_11305eb0` (провайдер `DAT_1280437c`) | путь одиночной игры |
| создание объекта сессии | `FUN_10260770` | ещё не декомпилирован; аналог в 1.06 open |
| `SpawnCoopAI` / `OnSpawnPlayer_%u` | строка `0x11fc1c64` (в 1.06 пара: `0x11fbda7c` push `0x111cdf76`, `0x1206ad0c` push `0x11b1da06`) | запрос спавна ко-опа |
| net-регистрация | `FUN_11839b60` / `FUN_10468480` | регистрация компонента и поля |
| «Not enough players» | место вызова `0x108c4b09`, внутри функций `0x108c4c6b` и `0x108c5c36` | цель 5-NOP патча мода 262 |
| центральный объект игры | `DAT_12804acc` (4 529 ссылок, `+0x68` контейнер компонентов) | объект, через который идёт любой поиск компонента |

Локальные измерения 1.06 (см. раздел про командную строку в англиском файле): функция загрузчика
`0x11aa2fe0`, экстрактор значений `0x11a1b940` (места вызова `-host` `0x11aa12a6`, `-client`
`0x11aa1362`, `-load` `0x11aa2b31`, `-wait` `0x11aa2680`), перезапись командной строки
`0x11aa39d0`..`0x11aa3b60` с хелпом добавления `0x100041b0` и хардкодными фрагментами
`' -world=fc3_main'` `0x11fdbdb1`, `' -spawnpos=333.63,964.5,124.465'` `0x11fdbdc4`,
`' -spawnangle=0.0,0.0,0.0'` `0x11fdbde4`, `' -spawnpos=-857.626,2317.1,119.752'` `0x11fdbe0c`,
`' -spawnangle=0.0,0.0,-20.0'` `0x11fdbe30`; блоки строк флагов в `FC3_d3d11.dll` `0x11fdbc*` и
в `FC3.dll` `0x11fb7*` (`0x11fb7d90`..`0x11fb7e27` — набор читов и графики).
## Открытые вопросы

- [ ] Какая версия установлена у владельца? 1.05 или 1.06?
- [ ] Где путь к установке? (сохранить в local/)
- [ ] У друзей тоже есть FC3, или одна копия + ко-оп?
- [ ] Сколько всего игроков (2? 3? 4?)?
- [ ] Все друзья имеют Windows-ПК с Steam?

## Запуск и командная строка в Uplay-сборке 1.06 (2026-11-01)

Проверено на машине владельца: `H:/Uplay/Far Cry 3` = Uplay-сборка **1.06**
(FC3_d3d11.dll 40869416 байт на диске, PE compile time 2022-05-04, версия файла 1.06.0.90512,
product 1089053420; MD5 FC3_d3d11 `916c6d4b876d8be5f0ad68f50719a838`, FC3
`5742de2aaa76935d46947bf89e2e2eb7`, farcry3_d3d11.exe `47d4c992b0e82b7d52cc48e8d1bd9119`).
Сборки 1.05 на дисках нет нигде (C:/Users/USER/Downloads, Desktop, D:/, E:/, H:/).

- **exe — DRM-загрузчик.** `bin/farcry3_d3d11.exe` (60992 байта) стартует и заново запускает exe
  **из канонической установки**, путь к которой берётся из реестра
  (`HKLM\SOFTWARE\WOW6432Node\Ubisoft\Far Cry 3`: `InstallDir=H:/Uplay/Far Cry 3`,
  `ExecutableName=bin/FC3Updater.exe`, SKU=WW, Version=1.05, Language=Russian).
  Дополнительные аргументы стираются перед запуском:
  только `-language=Russian` доходит до игры.

- Следствие для тестовой копии: запуск `H:/FarCryCoop/TestFC3/bin/farcry3_d3d11.exe` с
  `-host fc3_main -login h1 -coop` порождает дочерний процесс из `H:\Uplay\Far Cry 3\bin` только с
  `-language=Russian`. Быстрый запуск из своей копии невозможен.
- Строки быстрого запуска в 1.06 `FC3_d3d11.dll` (замер 2026-11-02; блок `.rdata` `0x11fdbc*`, зеркало
  в `FC3.dll` `0x11fb7d*`): `-exec` `0x11fb7d78`, `-execline` `0x11fb7d6c`, `-quickstartup` `0x11fb7d80`,
  `-perfhud` `0x11fb7d4c`, `-logFile` `0x11fb7d58`, `-bfname` `0x11fb7d64`; блок сетевых параметров
  `0x11fdc02c`..`0x11fdc27c` (`FC3.dll` зеркало `0x11fad15c`..`0x11fad308`): `-login` `0x11fdc040`,
  `-sessionuid` `0x11fdc0b4`, `-coop` `0x11fdc0cc`, `-online` `0x11fdc104`, `-ip` `0x11fdc124`,
  "Invalid parameters for -host, missing (-login)" `0x11fdc084`, "Missing -login or -ip for -client."
  `0x11fdc128`, `-netemulator` `0x11fdc10c`.
- **ОТКАЗ (2026-11-02):** старая таблица `0x1fb6f6c`-`0x1fb74f6` (`-host` @0x1fb73cb, `-client` @0x1fb7472,
  `-offline` @0x1f36444, `-dedicated` @0x1f0d9dc, "Missing -login or -ip" @0x1fb7458) оказалась ложной — эти
  адреса это концы строк в `\0`-наполнении. В `FC3.dll` 1.06 таких строк нет вовсе (`-offline`, `-dedicated`,
  `-dedicator`, `-hostmaster`, `-mapname`, `-stageName`, `-profile`, `-demo`), как и строк отладочного меню 1.05
  ("Not enough players", "Local Coop", "Coop Story"). Урок: поиск `\0<строка>\0` возвращает индекс ведущего
  `\0` — сдвигать +1.
- **Разобрано (2026-11-05, заново проверено скриптом `scripts/fc3_mode_cascade.py`):** каскад flags
  режима. Каскад — цепочка `push <строка>` / `push esi` / `call edi` (`strstr`), `add esp,8`,
  `test eax,eax`, `je <следующий flag>`; совпадение читает **статический id режима** из global-адреса
  и пишет его в `[ebp+8]`.
- **Тело каскада (`FC3.dll`): `0x11a8c440`** (`push ebp` / `mov ebp,esp` / `sub esp,0x2c` / `push ebx`
  / `mov ebx,[ebp+0x14]` / `mov esi,[ebp+0xc]` / `push edi` / `mov edi,[0x11bcb5c0]` =
  `MSVCR100.dll:strstr`). Ранее записанный диапазон `0x11a8c533` .. `0x11a8c567` — фрагмент этого тела.
- **Порядок flags и места сравнения (`FC3.dll`, `0x11a8c440` .. `0x11a8c5b1`):** `-login` `0x11a8c455`,
  `-ip` `0x11a8c464`, `Missing -login or -ip for -client.` `0x11a8c476`, `-online` `0x11a8c49d`,
  `-password` `0x11a8c4ac`, `Trying to launch an online client, but missing password (-password)`
  `0x11a8c4be`, `-ip` `0x11a8c4f0`, `-bfname` `0x11a8c4fe`, `-sessionuid` `0x11a8c50c`,
  `-deathmatch` `0x11a8c51a` (читает `[0x1216aedc]`), `-coop` `0x11a8c533` (читает `[0x1216aee4]`),
  `-domination` `0x11a8c54d` (`[0x1216aeec]`), `-headquarters` `0x11a8c567` (`[0x1216aef4]`),
  `-firestorm` `0x11a8c57d` (`[0x1216aefc]`), `-dash` `0x11a8c597` (`[0x1216af04]`), запасной variant
  `[0x1216af0c]` на `0x11a8c5b1`.
- **Значения id (`FC3.dll` `0x1216aed0` .. `0x1216af10`, блок побайтово идентичен
  `FC3_d3d11.dll` `0x1218ffd0` .. `0x12190010`):** `0x9ba82025` первый entry (`0x1216aed0` и
  `0x1216aed4`), `-deathmatch 0x8b40f692`, `-coop 0x43521e6f`, `-domination 0xab4fc8e2`,
  `-headquarters 0x9991daec`, `-firestorm 0xf66ffb87`, `-dash 0x2100ced7`, запасной `0x6c2b3868`;
  после каждого id идёт пара `0xffffffff`, и ещё один id `0xd2fd0a6b` стоит прямо перед блоком
  (`0x1216aecc` / `0x1218ffcc`). Две поправки к прежней записи: `-deathmatch` читает `0x1216aedc`
  (не `0x1216aed4`), и id, помеченный как "unknown" (`0xf66ffb87`) — это **`-firestorm`**.
- **Каскад читает эти id, а не пишет их:** значения — статические инициализаторы `.data`, присутствующие
  в обоих модулях 1.06, значит `-coop` в 1.06 это живой id режима, а не удалённая строка. `-online`
  выбирается отдельно: `[ebp-0xc]` равняется `3` на `0x11a8c5bf` по умолчанию и `2` на `0x11a8c5cf` при
  совпадении `-online` (сходится с выводом модели хоста: «тип сессии = 3 без `-online`»).
- **Потребитель id каскада (`FC3.dll` `0x11a8c5d6` .. `0x11a8c5f0`):** `mov ecx,[ebp+8]` →
  `mov edx,[ebx]` → `mov eax,[edx+0x14]` → **`mov [ebx+0xa4],ecx`** (id режима попадает в поле объекта
  `+0xa4`) → `mov ecx,ebx` / `call eax`, затем `mov eax,[edx+0x18]`, `mov ecx,ebx`, `call eax`.
  `FC3_d3d11.dll` даёт тот же каскад (`-domination` `0x11aa08ed`, `-headquarters` `0x11aa0907`,
  `-firestorm` `0x11aa091d`, `-dash` `0x11aa0937`, `-online` `0x11aa0959`) с диспатчем по слоту `+0x14`
  на `0x11aa0976`.
- **Что функция каскада делает после записи id (`FC3.dll` `0x11a8c5f0` .. `0x11a8c6b9`, вывод
  `local/scratch/cascade_tail.txt`):** она вызывает `0x119696d0` (`0x11a8c5f7`) и `0x11a8bdc0`
  (`0x11a8c601`), выполняет ref-count блок из `[ebx+0x9c] + 0x4c` и `[ebp+0x14]` (`cmp` на `0x11a8c612`,
  `lock xadd [edx], 1` на `0x11a8c62d`, `lock xadd [edx], -1` на `0x11a8c645`, `call 0x100945e0` на
  `0x11a8c64f`), ставит флаг `[ecx+0xd] = 1` (`0x11a8c668`), читает строковый global `-sessionuid`
  `0x11fb81e4` (`0x11a8c696`) и затем пишет поля записи запуска: `[ebx+0x64] = 0xfffffffd`
  (`0x11a8c69c`), `[ebx+0x40] = eax` (`0x11a8c6a3`), `[ebx+0x44] = edx` (`0x11a8c6a6`) и
  `[ebx+0x74] = 6` (`0x11a8c6b2`). Id режима уходит через объект с vtable-диспатчем; отсюда ни одна
  запись в реестр не регистрируется.
- **Обход `+0xa4` по всему `.text` (`FC3.dll`, скрипт `local/scratch/probe5.py`, вывод
  `local/scratch/a4sites.txt` 93637 байт):** 832 функций работают со слотом `[reg+0xa4]` (835
  обращений), и ровно одна заодно трогает global с id режима — `0x1181c440`; она оказалась UI-кодом
  (следующий пункт). Значит `+0xa4` не является уникальным признаком режима запуска, и потребителя
  надо искать через vtable-диспатч.
- **Тупик: `FC3.dll 0x1181c440` — это FlowGraph UI-обработчик `customMapEvaluation`.** Его `+0xa4` —
  слоты стека (`mov [esp+0xa4],4` на `0x1181d09e`, `mov [esp+0xa4],0x190` на `0x1181d101`); тело грузит
  своё имя `mov esi, 0x1201b27c` (`0x1181c45b`, UTF-16 строка сразу после fourcc `CFCX` на `0x1201b276`);
  `scripts/fc3_callers.py` даёт `0 E8 callers` плюс один абсолютный data-ref на `0x1201b444`, который
  лежит в таблице обработчиков `0x1201b404` .. `0x1201b458` с соседями `OnMapPublish` `0x1201b380`,
  `OnEnterMyMapsOK` `0x1201b39c`, `OnEnterMapArchiveOK` `0x1201b3bc`, `ExitMyMaps` `0x1201b3e4`,
  `updateCustomMapEvaluation` `0x1201b478`, `multiCustomMapEvaluationReportContent` `0x1201b4b0`,
  `multiCustomMapEvaluationFavoriteContent` `0x1201b500`, `multiCustomMapEvaluationLikeContent`
  `0x1201b550`, `customMapEvaluationTagsRequest` `0x1201b598`.
- **Похожий на реестр цикл сравнения внутри этого обработчика (`0x1181ceef` .. `0x1181cf30`), листинг `local/scratch/registry_scan_loop.txt`; оставлен,
  потому что это ровно тот шаблон, который показал бы настоящий реестр сессий:** `mov edi,[edx+0xc4]`
  (`0x1181cef5`), `mov ecx,[0x128048c0]` (`0x1181ceef`), `mov eax,[ecx+8]` (`0x1181cefb`),
  `mov edx,[ecx+4]` (`0x1181cefe`), `lea edx,[edx+eax*4]` (`0x1181cf01`), `mov eax,[ecx+4]` (`0x1181cf04`), `cmp eax,edx` (`0x1181cf07`), `je 0x1181cf21` (`0x1181cf09`), затем скан `mov ebx,[eax]`
  (`0x1181cf11`) → `cmp dword ptr [ebx+0x20], esi` (`0x1181cf13`) → `add eax,4` (`0x1181cf18`) →
  `cmp eax,edx` (`0x1181cf1b`) → `jne 0x1181cf11` (`0x1181cf1d`) при промахе, `je 0x1181cf1f` (`0x1181cf16`) при совпадении; `xor ebx,ebx` (`0x1181cf1f`) достигается и из прыжка совпадения, и из исчерпанного цикла, а `0x1181cf21` (`mov edx,[ecx+8]`, `mov ecx,[ecx+4]`, `lea edx,[ecx+edx*4]`) начинает второй проход по тому же контейнеру; `esi = [0x1216aeec]`
  (`-domination 0xab4fc8e2`, прочитано на `0x1181cf0b`), `edi = [0x1216af0c]` (запасной `0x6c2b3868`,
  прочитано на `0x1181ce50`). Новый контейнер: **`[0x128048c0]`**, `+4` = количество, `+8` = база
  индекса со множителем ×4, элементы — указатели, в поле `+0x20` которых лежит id; это другой
  контейнер, не `execLines` (`[0x12829424]`). Открыто: что такое `[0x128048c0]` и сравнивается ли
  значение `+0xa4` из каскада с полем `+0x20` такого элемента.
- **Найдены потребители launch-mode id** (скан whole-`.text` по инструкциям, читающим mode-id global,
  дамп `local/scratch/modeid_sites.txt`: в `FC3.dll` их читают только 3 функции): `0x1111e390`
  (`cmp eax,[0x1216aee4]` в `0x1111e4e3`), `0x1111e860` (`cmp eax,[0x1216aee4]` в `0x1111e91e`),
  `0x1181c440` (запасной id в `0x1181ce50`, `-domination` в `0x1181cf0b`). В `0x1111e390` id лежит в
  `mov eax,[edi+0x10]` (`0x1111e399`), контейнер берётся из `mov ecx,[ebx+0x154]` (`0x1111e3bc`),
  поиск id→record делает `call 0x10260770` (`0x1111e3c7`: `cmp byte ptr [ecx+0x5c],0` `0x10260773`,
  `cmp dword ptr [ecx+0x54],0` `0x1026077b`, `mov eax,[ecx+0x50]` `0x10260781`, `call 0x1025fee0`
  `0x1026078a`, `cmp dword ptr [eax+4],2` `0x1026079a`, `cmp ecx,[edi+0xc]` `0x102607a3`, возвращает
  record или 0), а поле record `+0xcc` (`0x1111e3d8 mov eax,[eax+0xcc]`) даёт контейнер этой записи.
- **Сама проверка coop идёт через второй синглтон (`FC3.dll 0x1111e4ce .. 0x1111e517`)**:
  `mov edx,[0x12804acc]` (`0x1111e4ce`) → `mov eax,[edx+0x68]` (`0x1111e4d4`) → `mov eax,[eax+0x30]`
  (`0x1111e4db`) → `or eax,0xffffffff` при пустом аксессоре (`0x1111e4e0`) → `cmp eax,[0x1216aee4]`
  (`0x1111e4e3`) → `mov ecx,[ebx+0x154]` (`0x1111e4ef`) → `mov eax,[ecx]` → `mov eax,[eax+0x68]` →
  `call eax` с `[esp+0x18]` (`0x1111e4fe`) → `mov eax,[edx+0x24]` (`0x1111e508`) → `call eax`
  (`0x1111e50b`) → `test al,al` → `cmp byte ptr [esp+0xb],al` → `cmp dword ptr [0x12835c94],0`
  (`0x1111e517`). Новые глобалы: **`[0x12804acc]`** (аксессор `+0x68`, vtable слоты `+0x30` и `+0x24`)
  и **`[0x12835c94]`**.
- **Тот же тест кооп-режима встречается второй раз внутри `0x1111e860` (`FC3.dll 0x1111e860 .. 0x1111ecb0`,
  разбор `local/scratch/r1_out.txt`)**: `mov eax,[0x12804acc]` (`0x1111e902`) → `mov eax,[eax+0x68]`
  (`0x1111e907`) → `mov eax,[eax+0x30]` (`0x1111e916`) / `or eax,0xffffffff` (`0x1111e91b`) →
  `cmp eax,[0x1216aee4]` (`0x1111e91e`) → `sete al` (`0x1111e924`) → `mov byte ptr [esp+0x13],al`
  (`0x1111e927`) → `test al,al` → `jne 0x1111e98e` (`0x1111e92d`); в отрицательной ветке
  `cmp dword ptr [ecx+0xc],0` (`0x1111e932`), `cmp dword ptr [0x12835c94],0` (`0x1111e938`),
  `call 0x10003cd0` (`0x1111e94a`), `call 0x10002aa0` (`0x1111e961`), `call 0x10425d20` (`0x1111e96e`,
  `cmp eax,ebx` → `je 0x1111e98e`), `mov ecx,[0x12805434]` (`0x1111e97f`), `call 0x11363ab0`
  (`0x1111e985`). Общая форма обеих функций: `mov ebx,[ebp+8]` + `add ebx,0x108` (`0x1111e8ad`) либо
  `mov ecx,[ebx+0x154]` (`0x1111e3bc`, `0x1111e437`) как искомый контейнер, `call 0x10425d20`
  (`0x1111e8b8`, `0x1111e96e`) с проверкой конца контейнера `cmp eax,ebx`, чтение поля записи
  `mov eax,[eax+0x20]` (`0x1111e8c5` — тот же `+0x20`, что и у записей `[0x128048c0]`),
  `mov ecx,[0x12805434]` (`0x1111e8c8`), `call 0x11363b10` (`0x1111e8de`),
  `cmp dword ptr [ebp+0x14],2` (`0x1111e98e`) и `cmp dword ptr [ebp+0x14],1` (`0x1111ea44`) как
  переключатель режима, `call 0x104245c0` (`0x1111e9a5`), и итоговая проверка `mov ecx,[0x12805180]`
  (`0x1111ea72`) → `mov eax,[ecx+4]` (`0x1111ea78`) → `mov edx,[eax+0x11c]` (`0x1111ea7b`) →
  `mov ecx,[eax+0x118]` (`0x1111ea81`) → `push edx; push ecx; push eax` → `call 0x102000d0`
  (`0x1111ea8a`) → `test al,al` → `je 0x1111eb48` (`0x1111ea91`). Открыто: чем являются
  `[0x12805180]`, `[0x12805434]`, `0x102000d0`, `0x10425d20` и кто вызывает `0x1111e390`/`0x1111e860`.
- **Владелец контейнера `[0x128048c0]` (`local/scratch/container_sites.txt`, 13 функций)**: создаёт его
  `0x10048230`, публикует дважды (`0x10048289`, `0x10048297 mov dword ptr [0x128048c0], eax`); читают
  `0x10f0c770` `0x10f0c7ca`, `0x10f0ea50` `0x10f0ec39`, `0x10f0ed30` `0x10f0ef19`, `0x113224c0`
  `0x11322514`, `0x113235d0` `0x113236a9`, `0x11340560` `0x113408f8` + `0x11340bdb`, `0x11709440`
  `0x11709450`, `0x1181c440`, `0x118227d0` `0x11822875`, `0x119288b0` `0x11928dee`, `0x11997720`
  `0x11997723` (чтение — первая инструкция функции, похоже на getter). Анатомия создателя: проверки
  `cmp byte ptr [0x128042fc], bl` (`0x1004823a`) и `cmp byte ptr [0x12162beb], bl` (`0x10048247`),
  `call 0x11789a00` (`0x10048242`), аллокация 0x20 `mov edx,0x10` / `lea ecx,[edx+0x10]` /
  `call 0x1177f7c0` (`0x1004824f`, `0x10048254`, `0x10048257`), vtable `mov dword ptr [eax], 0x11f224c4`
  (`0x10048271`), поля `+4 +8 +0xc +0x10 +0x14 +0x18` обнулены.
- **Vtable контейнера `0x11f224c4` выбирает слоты по флагам `+0x10`**: слот 1 `0x100a5df0`
  (`test byte ptr [ecx+0x10],1` `0x100a5df3`, `mov esi,[ecx+0x14]`, `add esi,[ebp+8]`,
  `cmp dword ptr [ecx+8],-1`, `lea eax,[ecx+4]`, `mov eax,[edx+0x20]` → `call eax` в `0x100a5e14`),
  слот 2 `0x100a5e30` (бит 2), слот 3 `0x100a5e70` (бит 4); слот 0 `0x100a5b10` после `call 0x117097d0`
  ограничивает `esi` интервалами `[0x121bbff8]` (`0x100a5b25`) и `[0x121a7fdc]` (`0x100a5b2d`).

- **Эти id встречаются в трёх других таблицах `FC3.dll`** (следующие кандидаты на реестр
  дескриптора сессии): `0x1216aa2c` .. `0x1216aa44` (`-deathmatch 0x1216aa2c`, запасной `0x1216aa30`,
  `-firestorm 0x1216aa34`, `-domination 0x1216aa38`, `-headquarters 0x1216aa3c`, `-coop 0x1216aa40`,
  `-dash 0x1216aa44`); `0x12173ff0` .. `0x1217400c` (`-domination 0x12173ff0`, `-coop 0x12173ff4`,
  `-deathmatch 0x12173ff8`, `-headquarters 0x12174000`, `-firestorm 0x12174008`, `-dash 0x1217400c`,
  запасной `0x12173ffc`); пара `-coop`/`-dash` на `0x1216a00c` / `0x1216a010`; `0x9ba82025` повторяется
  через `0x12163450` .. `0x121636e8`; `0xd2fd0a6b` ещё и на `0x1216cab0`. Открытым остаётся, что
  выбирает каждая таблица и кто их обходит.
- **Вторая цепочка сравнения имён режима (`FC3.dll`, `0x11a8c944` .. `0x11a8c9c1`)** (начало фукции —
  `0x11a8c8b0`, см. запись в конце файла) (`-deathmatch`
  `0x11a8c944`, `-coop` `0x11a8c95d`, `-domination` `0x11a8c977`, `-headquarters` `0x11a8c991`,
  `-firestorm` `0x11a8c9a7`, `-dash` `0x11a8c9c1`); у `-online` есть ещё ссылки на `0x11a8b8d3`,
  `0x11a8b91f`, `0x11a8cd09`. Открытым остаётся, что выбирает эта цепочка.
- **Счёт patch-сайтов reloc-режима для global-ов id режима и контейнера (`scripts/fc3_reloc_xref.py
  --mode reloc`, 772159 записей на модуль).** Global id режима `-coop` в `FC3.dll` `0x1216aee4` имеет
  **30** сайтов, значение в которых на него указывает, и `FC3_d3d11.dll` `0x1218ffe4` — ровно те же
  **30**; у строки имени режима **2** сайта в каждом модуле (`FC3.dll` `0x11fb81fc` -> `0x11a8c534`,
  `0x11a8c95d`; `FC3_d3d11.dll` `0x11fdc0cc` -> `0x11aa08d4`, `0x11aa0cfd`). У базового global-а
  accessor-ов `0x1282f6cc` в `FC3.dll` **4** сайта HIGHLOW (`0x1199cdcb`, `0x1199cddd`, `0x1199cf76`,
  `0x11bb33f1`), а у global-а контейнера `0x12829424` — **0**: до него доходят только через `disp32`,
  который загрузчик не патчит, поэтому 17 502 записей выше получены поиском по байтовому шаблону, а не
  из `.reloc`. Полные списки сайтов лежат в `local/scratch/xref_sites_fc3.txt` и
  `local/scratch/xref_sites_d3d11.txt`.
- Урок (2026-11-05): id режимов не являются `hash(имя)`. Пересчёт `-deathmatch`, `-coop`, `-domination`,
  `-headquarters`, `-firestorm`, `-dash` и имени режима по умолчанию алгоритмами FNV-1a, FNV-1, djb2,
  sdbm, ELF и CRC32 (на уже проверенном наборе из 26 имён) дал **0** совпадений в обоих модулях, то есть
  это константы игровой таблицы, а не значения, выведенные из имён флагов.
- Урок (2026-11-05): `scripts/fc3_reloc_xref.py` в режиме `code` не видит форму `push imm32`. Для двух
  мест `push <адрес строки режима>` внутри тела каскада режим `code` выдал `verified instructions: 0` и
  `rejected (no instruction ends at the field): 2`, а `--mode reloc` нашёл оба места: для аргументов
  `push imm32` надёжен только reloc-режим.
- Открытым остаётся, доходит ли co-op id до реестра дескриптора сессии.
 Global-адрес `-coop` передаётся аргументом в `FC3_d3d11.dll` на `0x10fed7f1` /
  `0x10fed817` (`call 0x103878e0`, `call 0x10656d90`); остальные места, где он хранится:
  `0x10fed7f2`, `0x10fed818`, `0x10fed876`, `0x10fed8c7`, `0x10fed8f5`, `0x10fed933`, `0x10fee3e6`,
  `0x11123d95`, `0x111241d0`, `0x111a38b1`, `0x111a5335`, `0x111a62d6`, `0x11a6e78f`, `0x11a6e81e`,
  `0x11aa08e4`, `0x11aa0a5e`, `0x11aa0d0d`, `0x11aa0e60`. Адрес строки `-coop` в `FC3.dll`
  (`0x11fb81fc`) встречается только на `0x11a8c534` и `0x11a8c95d`.
  Урок: id не являются `hash(имя_режима)` по FNV-1a/FNV-1/djb2/sdbm/ELF/CRC32 (проверено 26
  имён с вариантами регистра и суффиксов — 0 совпадений), то есть это игровая таблица id.
  Урок: `fc3_reloc_xref.py` в режиме `code` не подтверждает форму `push imm32` (`verified
  instructions: 0, rejected (no instruction ends at the field): 2`), а reloc-режим эти места
  находит — для аргументов `push imm32` надёжен только reloc-режим.
- Переименование `bin/upc_r1_loader.dll` загрузчик не развязывает: игра тихо выходит примерно за
  25 с (без окна, без события WER) — загрузчик Ubisoft Connect является обязательной зависимостью
  старта.
- Писать в `HKLM` текущему пользователю недоступно (без повышения прав), поэтому реестр нельзя
  перенаправить на приватную копию. Оригинальные значения сохранены в `local/registry-fc3-backup.reg`.
- Что в 1.06 работает: патч мютекса единственного экземпляра (`jnz` в `0x100041bb` /
  `0x10004092` -> NOPы) позволяет запустить две копии рядом; патч падения по списку Uplay-party —
  только для 1.05 (в 1.06 образец не встречается, там новый UPC API).
- Меню мультиплеера в 1.06 мертво: показывается жёстко зашитый loc id `0x23894` = 145556 = X360_EU_NETWORK_XLSP_DOWN (см. `D:/Projects/Co-op Anything/FarCry3Coop-bantila/NOTES.md`, строка 152).
  Поэтому быстрый запуск должен обходить меню, а не проходить через него. В верхней ветке (Bantila) обход уже применён.
- Автоматизация меню: меню не принимает синтетические `mouse_event`-клики;
  нажатия клавиш, посланные через `SendInput`, доходят до меню
  (проверено: `local/menu.py press down|enter`).

**Вывод:** быстрый запуск ко-опа на этой сборке невозможен без (а) DRM-free exe версии 1.05 или
(б) патченного загрузчика-заглушки, который мог бы запускать нашу копию с аргументами быстрого
запуска. Нужно решение владельца.

## Размещение скриптов и артефактов

Скрипты анализа лежат в `scripts/` и попадают в git — по конвенции Bantila: `scripts/`
отслеживается, а `/tools/` игнорируется (там внешние бинарники вроде Ghidra или паковщика).
Скрипты этой сессии закоммичены: `fc3_dis.py`, `fc3_callers.py`, `fc3_addrmap.py`,
`fc3_iat_reverse.py`, `fc3_strgrep.py`, `fc3_reloc_xref.py`, `fc3_trampoline.py`,
`fc3_upc_probe.py`, `fc3_launch_probe.py`, `fc3_execlines_probe.py`. Кэши разборки
(`fc3_dis_cache.json`), черновики агента (`Temp/`) и приватное состояние (`local/`,
`local/scratch/`) игнорируются. Карточки черновиков в `.gitignore`: `*.scratch*`,
`.scratch*`, `scratch_*`, `local/scratch/`.

Каноническая схема владельца: всё под `E:\FarCryCoop\` — `Downloads\`, `Tools\`, `Data\`, `Temp\`,
`Work\` (см. `docs/workspace.md`). Диска `E:` на этом ПК нет, поэтому все записи про `E:\FarCryCoop\`
в этом файле — историческая запись с ПК владельца (октябрь 2026), а не рабочий путь здесь.
Рабочие пути этого ПК (проверено 2026-11-02, канон — `AGENTS.md` + `docs/LOCAL.md` в конвейере
FarCryCoop): репозитории в `D:\Projects\Co-op Anything\`, тестовая копия 1.06 в
`H:\FarCryCoop\TestFC3\`, установка только для чтения (Ubisoft Connect) в `H:\Uplay\Far Cry 3\`.
Перенесённые из bantila скрипты изначально содержали устаревшие константы
`DLL_106 = E:\SteamLibrary\steamapps\common\Far Cry 3\bin\FC3.dll` и
`DLL_105 = E:\FarCryCoop\Downloads\fc3-1.05\bin\FC3.dll`; в местных `scripts\` они перенацелены на
проверенные пути выше. Агентская программа пишет временные файлы в
`C:\Users\<user>\AppData\Roaming\pi-desktop\tmp` (выводы делегатов, скрипты песочницы) — это
противоречит золотому правилу 2 (не трогать C:): отмечено, не исправлено. Противоречие внутри
правил владельца решает владелец, а не агент.

В корне рабочей папки лежит неудаляемый файл `nul`, из-за него `git add -A` падает
(`error: invalid path 'NUL'`) — коммитить только с явными путями.

## execLines и запуск batch-файлов (проверено 2026-11-02, 1.06)

- Поле `execLines` регистрируется как параметр сессионной операции, а не как точка исполнения: в `FC3.dll`
  `push 0x11f90af0; push 8` (`0x113429ca`, защита `cmp byte ptr [0x12805494], 0`, вызов `0x1030f2b0`, кэш
  `[0x1282f900]`/`[0x1282f8fc]`); близнец в `FC3_d3d11.dll` — `push 0x11fb4a80; push 8; call 0x103173f0`
  (`0x1134824a`), соседи `execFilenames` `0x11fb4ad8`, `map`, `mapIdLow/High`, `mapIdType`, `bfname`. У строки
  `execLines` в обеих 1.06-библиотех ровно одна код-ссылка: отследить, кто её читает, через
  `PE.code_refs()` нельзя — тот метод видит только `push imm32`/`mov r32, imm32` и слеп к vftable и
  поле-ссылкам.
- Контейнер ExecLines — глобальный синглтон `0x12829424` (`FC3_d3d11.dll`); конструируется в
  `0x11788440`..`0x11788476`: `test byte ptr [0x128ac410], al; push 0; mov ecx, 0x128ac3f4; call 0x106c4b40`,
  затем `push 0x11bb98c0; call 0x1070085d` и `mov dword ptr [0x12829424], 0x128ac3f4; ret`. Сырой поиск по
  `.text` даёт 17 502 обращений и ровно два constructor-подобных записи (`c7 05` в `0x1178846e` и `0x118e82fd`)
  — то есть это один объект, а не реестр всех batch-файлов. VA в образе little-endian (`0x12829424` → байты
  `24 94 82 12`).
- Консольная команда `exec`: имя `exec` `0x11fdd478`, сигнатура `Game:Exec(%%)` `0x11fdd468`, справка
  `Runs a console command "batchfile". Usage: "exec <batchfile>"` `0x11fdd428` (ссылки `0x111f5c0a`,
  `0x111f5c1a`); блок регистрации `0x111f5b80`..`0x111f5cbc`, его функция начинается с `0x111f4eab`; сам вызов
  регистрации `0x111f5c7b`: `push ebx; push 1; push edx; push <sig>; push eax; push ecx; call 0x1163d910` —
  указатель обработчика лежит в `ebx`, поэтому `code_refs()` его не находит. Близнецы в `FC3.dll`: справка
  `0x11fb9558`, `Usage: runbatch <batchfile>` `0x11fba5bd` (близнец `0x11fde48d`), ссылки `0x111f032a`,
  `0x111f033a`.
- Класс batch-файловой операции движка `CFCXRunBatchFileOperation` (`FC3_d3d11.dll` `0x12032844`, ссылка
  `0x104e5fe9`; `FC3.dll` `0x1200e72c`, ссылка `0x104df159`) регистрируется из инициализационной функции
  `0x104e5f42` (защита `cmp byte ptr [0x1288b074], 0`; идиом `call 0x103a9180` → `push <имя>` →
  `call 0x10038870`), рядом регистрируются `CFCXClearMessageBoxManager` `0x12032808` и
  `CFCXPrepareUnloadWorldOperation` `0x12032824`. У `0x104e5f42` и у `0x111f5a00` ноль E8-вызовов: блоки
  регистрации выполняются CRT-инициализатором (`.initterm`), поэтому «0 вызовов» не означает «мёртвый код».
- Вызов создания сессии в ветке `-quickmatch`/`-host` (`FC3.dll`, `0x11a8d0c5`..`0x11a8d0e2`):
  `lea edx,[ebp+8]; push edx; lea edi,[ebp-0xb4]; call 0x11341dc0` → `mov ecx,[eax]` (vftable) →
  `mov eax,[ecx]` → `mov eax,[eax+0x74]` → `lea edx,[ebp+0x10]; push edx; lea esi,[ebx+0x18]; call eax` →
  `mov edi,eax; call 0x10005570`; путь отказа — `lea ecx,[ebp-0xb4]; call 0x11343640` (`0x11a8d0f6`),
  освобождающий поля `[obj+0x98]`/`[obj+0x9c]`. Вывод: слот vftable `0x74` принимает ровно один аргумент —
  180-байтную (0x
B4) структура параметров сессии по адресу `[ebp+0x10]`. Геттер интерфейса сессии `0x11341dc0` имеет
31 E8-вызовов в `FC3.dll` (`0x10ce93c9`, `0x10edd676`, `0x11163252`, `0x111fa8de`…) — это общий
аксессор, поэтому ко-оп маршрут нужно опознавать по раскладке 180-байтной структуры, а не по уникальности
вызывающих.
- Урок (2026-11-02): блоки регистрации, которые запускаются только CRT-инициализатором, не видны поиску
  вызывающих, а указатели обработчиков, переданные в регистрах, не видны `code_refs()`. Единственная
  зацепка для потребителя `-exec` — 180-байтная структура параметров сессии в `FC3.dll 0x10a8d0db` и поле
  сессионного объекта `[obj+0x98]`; их и нужно зондировать следующим шагом.

- **Тупик статического анализа для исполнителя `execLines` (2026-11-02, `FC3_d3d11.dll` 1.06):**
  ленивый thunk с начала `0x118e82d0` — это Meyers-синглтон-доступ контейнера: `test byte ptr
  [0x128ac410], 1` / `or dword ptr [0x128ac410], 1` / `push 0; mov ecx, 0x128ac3f4; call 0x106c4b40` /
  `push 0x11bb98c0; call 0x1070085d` / `mov al, 1; mov dword ptr [0x12829424], 0x128ac3f4; ret`, а
  `0x11bb98c0` — thunk уничтожителя, зарегистрированный в atexit (`mov ecx, 0x128ac3f4; jmp 0x106c4b70`),
  сосед `0x11bb98d0` для следующего глобального объекта. Имена полей сессионной операции
  регистрируются при первом обращении (`0x11348230` с защитой `byte [0x1282a60c], 0` → `call 0x11347c60`
  с защитой `byte [0x1282a607], 0` → `push <имя>; push <размер>` → `call 0x103173f0`, далее пары
  `[0x12852d54]`/`[0x12852d58]` и `[0x12852dcc]`/`[0x12852dd0]`, `sar ecx, 2` = число элементов), а
  реестр трогают только 10-байтные accessor-thunkи в `0x11bc8a80`..`0x11bc8ad0` вида
  `b8 <слот> ; e9 <jmp 0x10004d70>`. Вывод: статически исполнителя не изолировать — идиом
  `mov ecx,[0x12829424]; call 0x106c99b0; mov eax,<строка>; call 0x11788480` в библиотеке 8 311
  экземпляров (это общий контейнерный вызов), а у кандидата в исполнители `0x118e8310` вообще 0 E8
  ссылок. Следующая зацепка — динамическая: запуск тестовой копии с `-exec`/`-execline` либо
  декомпиляция thunkей в Ghidra (`scripts/fc3_ghidra_dis.py`).

## Зацепки 1.06: флаг `-coop`, пароль бунта и блок полей сессии (2026-11-02, сеанс 22:xx)

- **Второй разбор аргументов в `FC3_d3d11.dll` полностью разобран (2026-11-05, `scripts/fc3_mode_cascade.py`):**
  цепочка `0x11aa0cfc .. 0x11aa0d7a` сравнивает тот же набор флагов и пишет id режима в локальную ячейку
  `[ebp+0xc]` (главный разбор пишет в `[ebp+8]`): `-coop` → `0x11aa0d0b mov ecx, [0x1218ffe4]`
  (`0x43521e6f`) → `0x11aa0d11 mov [ebp+0xc], ecx`; `-domination` → `[0x1218ffec]` = `0xab4fc8e2`, запись
  `0x11aa0d2b`; `-headquarters` → `[0x1218fff4]` = `0x9991daec` (чтение `0x11aa0d3f`); `-firestorm` →
  `0x11aa0d55 mov ecx, [0x1218fffc]` = `0xf66ffb87` → `0x11aa0d5b`; `-dash` → `0x11aa0d6f mov edx,
  [0x12190004]` = `0x2100ced7` → `0x11aa0d75`; резерв `[0x1219000c]` = `0x6c2b3868`. Зеркало в `FC3.dll`
  подтверждено только по строкам (`-coop` `0x11a8c95d`, `-domination` `0x11a8c977`, `-headquarters`
  `0x11a8c991`, `-firestorm` `0x11a8c9c1`): интервал `0x11a8c944 .. 0x11a8c9c1` — это часть функции с началом в **`0x11a8c8b0`** (RESOLVED 2026-11-06); см. запись «Вторая цепочка launch-режимов, вход RESOLVED» в конце файла.
- **Флаг `-coop` и разбор запуска.`FC3_d3d11.dll`** строка `-coop` лежит в `0x11fdc0cc` — в том же
  связном блоке переключателей, что `-perfhud` `0x11fdbc1c`, `-logFile` `0x11fdbc28`, `-bfname`
  `0x11fdbc34`, `-execline` `0x11fdbc3c`, `-exec` `0x11fdbc48`, `-quickstartup` `0x11fdbc50`,
  `-debugFramerateCameras` `0x11fdbcf8`. Флаг разбирают две функции: главный разбор аргументов входа
  `0x11aa0670` (`-coop` в `0x11aa08d3` и `0x11aa0cfc`, `-bfname` в `0x11aa089e` и `0x11aa0cb9`) и второй
  блок разбора `0x11aa1b90` (там же `-netemulator` `0x11fdc10c` в `0x11aa10cc`/`0x11aa10db`, `-ranked`
  `0x11fdc11c` в `0x11aa0bda`/`0x11aa1198`, `-spawn` `0x11fdbe90`, `-spawnpos` `0x11fdbe98`,
  `-spawnangle` `0x11fdbea4`, `-class` `0x11fdbe88`). Отдельный парсер консольных строк — `0x11aa2fe0`
  (`-bfname` `0x11aa302a`, `-execline` `0x11aa3054`/`0x11aa3070`/`0x11aa30ce`, `-exec`
  `0x11aa3144`/`0x11aa3167`/`0x11aa3244`/`0x11aa3250`, `-quickstartup` `0x11aa326a`, дальше консольные
  строки `gfx_NoLaunchCountdown 1` `0x11fdbc60`, `gfx_NoPostRoundCountdown 1` `0x11fdbc78`,
  `gfx_NoPreRoundTimeOut 1` `0x11fdbc94`, `multi_DisableInitialSpawnDelay 1` `0x11fdbcac`,
  `hack_loadOutIsWaitingForMissionBlocks 0` `0x11fdbcd0`). Три вызова в `0x11aa2fe0`: `0x117b565d`,
  `0x117b7003`, `0x11997f41` плюс слот таблицы `0x11f31b10`. У `-splitscreen` `0x11f31d75`,
  `-splitscreen4` `0x11f31d85` и `-numclients` `0x11fb1e18` нет ссылок с немедленным операндом — их
  читают через смещения строковой таблицы, поэтому поиск `push imm32` / `mov reg, imm32` их не находит.
- **Обработчик `-password` и буфер пароля бунта:** функция `0x117b5580` (`push ebp; mov ebp,esp;
  sub esp,8; mov eax,[ebp+8]`) собирает строку из `[eax]` или `[eax+0xc]` (при пустом значении
  `0x117b5597 mov eax,0x11f31ea8`), далее `0x117b55a6 mov eax,0x11f31d34 str='-password'` и
  `0x117b55ab call 0x117b5160` (разбор переключателя). Если значение есть и `[ebx+4]` не ноль:
  `0x117b55c4 test byte ptr [0x128b59a8],1`, `0x117b55cc or dword ptr [0x128b59a8],1`,
  `0x117b55d2 push 0x11f31bbc str='******'` и `mov dword ptr [0x128b59a4],0`, `0x117b55dd call 0x10002a50`,
  `0x117b55f1 call 0x10001ec0`, `0x117b55f6 push 0x11bb76a0`, `0x117b55fb call 0x1070085d`
  (регистрация колбэка), `0x117b5606 push 0x128b59a4`. В этой же функции находятся вызовы
  `0x117b5653 call 0x11aa0670` и `0x117b565d call 0x11aa2fe0`. Вывод: пароль бунта (`-bfname`)
  обрабатывается до разбора аргументов запуска, то есть защищённый паролем бунт доступен из
  командной строки.
- **Правка блока имён полей сессии для 1.06:** имена полей реестра сессионной операции лежат связно в
  `.rdata` с `0x11fb4a00` по `0x11fb4c98`; обходитель реестра `0x103173f0` (`cmp byte ptr
  [0x128293f4],0`) резервирует запись размером `0x10 + 0x20` = 48 байта через `0x11790750` /
  `0x118e8340`. Полные адреса 1.06: `untFlags` `0x11fb4a00` (вызов `0x11347a0c`), `controllerIndex`
  `0x11fb4a0c` (вызов `0x11347a34`), `silentLogin` `0x11fb4a1c` (вызов `0x11347a5c`), `isPrimaryIndex`
  `0x11fb4a28` (вызов `0x11347a84`), `platformSpecificId` `0x11fb4a38` (цепочка `0x11342759 call
  0x103173f0`, есть и другие места), `demoname` `0x11fb4a4c`, `isInEditor` `0x11fb4a58`,
  `fullNetworkInitialization` `0x11fb4a64` (место `0x113478e0`), `execLines` `0x11fb4a80` (единственное
  место `0x1134824a`), `mapIdLow` `0x11fb4a8c`, `mapIdHigh` `0x11fb4a98`, `versionIdHigh` `0x11fb4aa4`,
  `versionIdLow` `0x11fb4ab4`, `mapIdType` `0x11fb4ac4`, `bfname` `0x11fb4ad0` (места `0x11328860`,
  `0x113482f4`), `execFilenames` `0x11fb4ad8` (место `0x11348300`), `saveGameFilename` `0x11fb4ae8`,
  `unloadWorld` `0x11fb4afc`, `SearchIdAddress` `0x11fb4b08`, `SearchSessionId` `0x11fb4b18`, `JoinGame`
  `0x11fb4b28` (место `0x11348010`), `JoinService` `0x11fb4b34`, `JoinSessionId` `0x11fb4b40`,
  `DeleteSessionId` `0x11fb4b50`, `DeleteMessageId` `0x11fb4b60`, `PlaylistId` `0x11fb4b70`,
  `MatchMakingEnabled` `0x11fb4b7c`, `SessionRefId` `0x11fb4b90`, `SessionType` `0x11fb4ba0`,
  `NetEngineType` `0x11fb4bac`, `AccountType` `0x11fb4bbc`, `SearchRanked` `0x11fb4bc8`,
  `SearchRetries` `0x11fb4bd8`, `SearchSlots` `0x11fb4be8`, `SearchNeedSlots` `0x11fb4bf4`,
  `SearchGameModes` `0x11fb4c04`, `Silent` `0x11fb4c14`, `DeleteCurrent` `0x11fb4c1c`, `WasInvited`
  `0x11fb4c2c`, `EmulatorProfileId` `0x11fb4c38`, `UseTicketRequestAttachment` `0x11fb4c4c`,
  `FollowHost` `0x11fb4c68`, `DoNATTypeDetection` `0x11fb4c74`; дальше идут поля быстрого запуска и
  возвращения игрока `mode` `0x11fb4c88`, `touched` `0x11fb4c90`, `spawnPtId` `0x11fb4c98`,
  `keepPlayerState` `0x11fb4ca4`, плюс `OperationId` `0x11fb4938`, `EngineUpdateFlag` `0x11fb4944`,
  `ranked` `0x11f45e34`, `world` `0x11f46910`, `pos` `0x11f39450`. Прежняя заметка (ссылки вида
  `SearchSessionId 0x11f90c…`, `FollowHost 0x11f90cd8`, `DoNATTypeDetection 0x11f90ce4`, `execLines
  0x11f90af0`) относилась не к тому билду: те адреса — dword-ы указательной таблицы `.rdata`, а не
  строки. Тип поля `execLines` — код `8` из `push` рядом с именем (строка/вектор); оценка «180 байт» из
  прежней заметки тоже относилась не к этому билду; по 1.06 это указательный массив (см. раздел ниже).

## Таблицы записей дескриптора сессии (2026-11-03, оба модуля 1.06)

**Как это было перечислено.** `python -I scripts/fc3_param_records.py <pe> <VA создателя> … --window 64`
сканирует `.text` на `e8 rel32` в создателя дескриптора, а затем назад на `--window` байт ищет
`push imm32` (VA строки имени) и `push imm8` (код типа). Форма регистрации в обоих модулях:
`push 3; push 0; push <VA дескриптора>; push 0; push 0x21; mov eax, <VA имени>; call <регистратор>`.
Просмотр назад может захватывать чужую границу функции, поэтому нужен `--ident-only`
(`IDENT_RE = ^-?[A-Za-z][A-Za-z0-9_.]{1,39}$`) и фильтр мест вызова по блоку регистрации:
`FC3_d3d11.dll 0x11347xxx` / `0x11348xxx`, `FC3.dll 0x11341xxx` / `0x11342xxx`. Полные выгрузки:
`local/scratch/param_recs_session_block_d3d11.txt` (63 строки) и
`local/scratch/param_recs_fc3dll_families.txt` (4118 строк).

**Семейства регистраторов (исправлено).** `FC3_d3d11.dll` 1.06: `0x11acaba0` / `0x11acac50` /
`0x11acad00` / `0x11acb320` / `0x11acaf10` — это **регистраторы обычных атрибутов движка** (они же
регистрируют `Name` `0x11f32188`, `Value` `0x11f333d4`, `Data` `0x11f32280`, `zoneId` `0x11f59698`),
поэтому таблицу сессии надо отделять по месту вызова. `FC3.dll` 1.06 (по распределению вызовов в
`0x11341000..0x11343000`): `0x11ab6800` ×25, `0x11ab6750` ×9, `0x11ab6ed0` ×11, `0x11ab6ac0` ×8,
`0x11ab68b0` ×6, `0x11ab6b70` ×2, `0x11ab6e20` ×1, `0x11ab6a10` ×1, плюс `0x1003b4b0` ×25,
`0x106f9ff0` ×21, `0x100945e0` ×9, `0x10a00be0` ×5, `0x10001ed0` ×5, `0x1030f2b0` ×2. Прежний список
(`0x11ab64c0` … `0x11ab70b0`) **неверен** — с ним перечисление даёт
`records with a name: 0   all creator call sites: 0`. `0x1030f2b0` (FC3.dll) и `0x103173f0`
(FC3_d3d11.dll) — не регистраторы: это `hkVirtual::operator new(0x1c)` (таблица `0x10bcd68`:
+0 `0x112f0e10` MemoryRealloc, +4 `0x112f0fa0` operator new, +8 `0x112f1530` MemoryDealloc).

**Список параметров — это массив 4-байтных указателей со счётчиком, а не 48-байтные записи**
(заменяет прежнюю оценку). `FC3.dll` 0x113429d1..0x11342a00 (`local/scratch/exec_records_dll.txt`):
`mov byte ptr [0x12805494], 1` → `call 0x1030f2b0` (0x113429d8) → `mov ecx, [0x1282f900]` (счётчик) →
`mov eax, [0x1282f8fc]` (база массива) → `lea eax, [eax + ecx*4]` → `push 1` → `mov esi, 0x1282f8fc` →
`call 0x1003b4b0` (**создатель exec-записей**) → `push 3; push 0; mov dword ptr [eax], edi`
(дескриптор hkVirtual пишется в смещение 0 вернутого объекта). Зеркало `FC3_d3d11.dll`
0x11348251..0x1134828b (`local/scratch/exec_records_d3d11.txt`): `mov byte ptr [0x1282a60c], 1` →
`call 0x103173f0` (0x11348258) → `mov ecx, [0x12852dd0]` → `mov eax, [0x12852dcc]` →
`lea eax, [eax + ecx*4]` → `push 1` → `mov esi, 0x12852dcc` → `call 0x10638fb0` (**создатель
exec-записей, новый якорь**) → `push 3; push 0; mov dword ptr [eax], edi` →
`push 0x48; push 0x11fb178c ('map'); mov eax, esi; call 0x11acaba0`.

**Exec-записи есть в обоих модулях:** `execLines` (FC3.dll место `0x113429ca`, `call 0x1003b4b0` в
`0x113429f7`, тип `0x01`; FC3_d3d11 место `0x1134824a`, `call 0x103173f0` в `0x11348258`, тип `0x08`) и
`execFilenames` (FC3.dll место `0x11342a7f`, `call 0x1003b4b0` в `0x11342aa0`, тип `0x01`; FC3_d3d11
место `0x113482ff`, `call 0x11348306`, тип `0x14`). Оба создателя вызываются с `push 1`, то есть как
одноэлементная коллекция — именно поэтому exec-записи не используют обычные строковые/числовые
регистраторы.

**Имена сессии и VA их строк (FC3_d3d11.dll → зеркало FC3.dll):** `abortEngineUpdateFlag 0x11fb4990` →
`EngineUpdateFlag 0x11f909b4` (регистратор `0x11ab6b70`, тип `0x08`); `login 0x11fb49c0` → `0x11f90a30`;
`password 0x11fb49d4`; `onlineKey 0x11fb49e0`; `primaryAccount 0x11fb49ec`; `accountFlags 0x11fb49fc`;
`controllerIndex 0x11fb4a0c`; `platformSpecificId 0x11fb4a38`; `demoname 0x11fb4a4c` → `0x11f90abc`;
`execLines 0x11fb4a80` → `0x11f90af0`; `mapIdLow 0x11fb4a8c` → `0x11f90afc`; `mapIdHigh 0x11fb4a98` →
`0x11f90b08`; `versionIdHigh 0x11fb4aa4` → `0x11f90b14`; `versionIdLow 0x11fb4ab4` → `0x11f90b24`;
`mapIdType 0x11fb4ac4` → `0x11f90b34`; `bfname 0x11fb4ad0` → `0x11f90b40`; `execFilenames 0x11fb4ad8` →
`0x11f90b48`; `saveGameFilename 0x11fb4ae8` → `0x11f90b58`; `SearchIdAddress 0x11fb4b08` → `0x11f90b78`;
`SearchSessionId 0x11fb4b18` → `0x11f90b88`; `JoinSessionId 0x11fb4b40` → `0x11f90bb0`;
`DeleteSessionId 0x11fb4b50` → `0x11f90bc0`; `PlaylistId 0x11fb4b70` → `0x11f90be0`;
`SessionRefId 0x11fb4b90` → `0x11f90c00`; `SessionType 0x11fb4ba0` → `0x11f90c10`;
`NetEngineType 0x11fb4bac` → `0x11f90c1c`; `AccountType 0x11fb4bbc` → `0x11f90c2c`;
`SearchRetries 0x11fb4bd8` → `0x11f90c48`; `SearchSlots 0x11fb4be8` → `0x11f90c58`;
`SearchNeedSlots / SearchGameModes / Silent / DeleteCurrent / WasInvited / EmulatorProfileId /
UseTicketRequestAttachment / FollowHost / DoNATTypeDetection / mode` в `0x11fb4bf8..0x11fb4c88`;
`map 0x11fb178c` → `0x11f8d78c`; `world 0x11f46910` → `0x11f234e0`; `spawnPtId 0x11fb4c98` → `0x11f90d08`;
только в FC3.dll `pos 0x11f15f68` (место `0x1134267f`, регистратор `0x11ab6a10`, тип `0x08`).
Коды типов обычных регистраторов: `0x11ab6750` (`map` `0x48`, `bfname` `0x4c`, `login` `0x04`,
`demoname` `0x0c`), `0x11ab6ac0` (`mapIdLow` `0x28`, `mapIdHigh` `0x20`, `versionIdHigh` `0x38`,
`versionIdLow` `0x40`, `spawnPtId` `0x18`, `SearchSessionId` `0x10`), `0x11ab6ed0` (`PlaylistId` `0x34`,
`SessionRefId` `0x20`, `SessionType` `0x38`, `NetEngineType` `0x3c`, `AccountType` `0x40`,
`SearchRetries` `0x44`, `SearchSlots` `0x48`, `mapIdType` `0x30`), `0x11ab6800` (`0x10`/`0x20`/`0x4c`/`0x4e`/`0x4f`/`0x53`/`0x54`).

**Пары, подтверждённые псевдокодом в обоих модулях 1.06** (`local/scratch/rea/pseudo-fc3dll-v2/functions/`,
`local/scratch/rea/pseudo-d3d11-v2/functions/`). Соответствия FC3.dll ↔ FC3_d3d11.dll:
`0x11342200`↔`0x11347bc0`/`0x11347c60`/`0x11347da0` (копирователь массивов, FC3_d3d11 экспортирует два
идентичных близнеца), `0x113423e0`↔`0x11347c60`, `0x11342480`↔`0x11347d00`, `0x113425c0`↔`0x11348480`,
`0x11342740`↔`0x11347fc0`, `0x113429b0`↔`0x11348230`, `0x11342af0`↔`0x11348370`, `0x1030f2b0`↔`0x103173f0`,
`0x1030f250`↔`0x10317390`, `0x1030f310`↔`0x10317450`, `0x1030f130`↔`0x10317270`, `0x1030f200`↔`0x10317340`,
`0x1030f430`↔`0x10317570`, `0x1003b4b0`↔`0x10638fb0`, `0x11ab6750`↔`0x11acaba0`,
`0x11ab6a10`↔`0x11acafc0`, `0x11ab6ac0`↔`0x11acaf10`, `0x11ab6800`↔`0x11acac50`, `0x11ab68b0`↔`0x11acad00`,
`0x11ab6e20`↔`0x11acb270`, `0x11ab6ed0`↔`0x11acb320`.

- `0x1003b4b0` / `0x10638fb0` — помощники **добавления** в вектор, аргумент = количество элементов: псевдокод
  `puVar2 = FUN_10638fb0(1); *puVar2 = uVar1;`, а в копирователе `sVar1 = DAT_12845d90 * 4;
  pvVar3 = FUN_10638fb0((int)sVar1 >> 2); memcpy(pvVar3, pvVar2, sVar1);`. Тип элемента записи —
  **4-байтовый указатель**, каждая регистрация добавляет один указатель.
- Копирователь (`FC3.dll 0x11342200`): `pvVar2 = DAT_12820c1c; if (DAT_1280548f == '\0') { sVar1 = DAT_12820c20 * 4;
  pvVar3 = FUN_1003b4b0((int)sVar1 >> 2); memcpy(pvVar3, pvVar2, sVar1); DAT_1280548f = 1; } pvVar2 = DAT_1282f884;
  sVar1 = DAT_1282f888 * 4; pvVar3 = FUN_1003b4b0((int)sVar1 >> 2); memcpy(...)`. Близнец FC3_d3d11 — такая же
  форма с `DAT_12845d8c`/`DAT_12845d90` под защитой `DAT_1282a607`, затем `DAT_12852d54`/`DAT_12852d58`.
  ⇒ именованные статические массивы — одноразовые *шаблоны*; живые списки записей именно в векторах.
- Строитель exec-записей (`FC3_d3d11.dll 0x11348230`, защита `DAT_1282a60c`; близнец FC3.dll `0x113429b0`,
  защита `DAT_12805494`, сначала вызывает `0x113423e0`): `uVar1 = FUN_103173f0(8,"execLines");
  puVar2 = FUN_10638fb0(1); *puVar2 = uVar1; FUN_11acaba0(&DAT_11fb178c /* "map" */,0x48,0,3);
  FUN_11acaf10("mapIdLow",0x28,0,3); FUN_11acaf10("mapIdHigh",0x20,0,3); FUN_11acaf10("versionIdHigh",0x38,0,3);
  FUN_11acaf10("versionIdLow",0x40,0,3); FUN_11acb320("mapIdType",0x30,0,3); FUN_11acaba0("bfname",0x4c,0,3);
  uVar1 = FUN_103173f0(0x14,"execFilenames"); puVar2 = FUN_10638fb0(1); *puVar2 = uVar1;
  FUN_11acaba0("saveGameFilename",0x50,0,3); FUN_11acac50(0x54,0,&DAT_12852dcc,0,3);` — размеры дескрипторов:
  `execLines` = 8, `execFilenames` = 0x14; `&DAT_11fb178c`/`&DAT_11f8d78c` — строка `map`.
- Аккаунт-строитель (`0x11348370` ↔ `0x11342af0`): `login` 4, `password` 8, `onlineKey` 0xc, `primaryAccount` 0x14,
  `accountFlags` 0x18, `controllerIndex` 0x1c (регистратор `0x11acb270`↔`0x11ab6e20`), `platformSpecificId` 0x20,
  плюс безымянные смещения `0x10`/`0x1d`/`0x1e` на `&DAT_12852d9c` ↔ `&DAT_1282f8cc`.
- Цепочка условий, которую обязан пройти любой дескриптор (`FC3.dll 0x1030f2b0`): `if (DAT_128042fc == '\0')
  FUN_11789a00(); if (DAT_12162beb != '\0') { iVar1 = FUN_1177f7c0(); if (iVar1 != 0) goto create; }`
  `if (FUN_118db560() == 0) return 0; create: return FUN_1030f430(size, name);`. Близнец FC3_d3d11
  (`0x10317390`): `DAT_128293f4`→`FUN_1179a990()`, `DAT_12187c6b`→`FUN_11790750()`, `FUN_118e8340()` →
  `FUN_10317450()`.
- Строитель world/точки порождения (`FC3.dll 0x113425c0`, защита `DAT_12805498`): кладёт объект, возвращённый
  `FUN_1030f200` (0, если условия не прошли), и пишет `world` 4, `pos` 8 (`&DAT_11f15f68`, регистратор
  `0x11ab6a10`), `spawnPtId` 0x18, безымянное `0x21` на `&DAT_1282f95c` и `0x20` как `iVar1 + 0x14`.
  Близнец FC3_d3d11 `0x11348480` (защита `DAT_1282a609`) пишет `abortEngineUpdateFlag` 0x14 (регистратор
  `0x11acafc0`) + безымянные `0x10`/`0x18`/`0x19`/`0x1a`/`0x1b` на `&DAT_12852d84`.
- **Семейство конструкторов дескриптора** (одинаковая раскладка слотов в обоих модулях; слот *n* =
  объект + 0x<*n*4>): `0x1030f310` ↔ `0x10317450` пишет slot1 = `"GameOpInfos"`, slot2 = помощник strlen
  `FUN_11788030` ↔ `FUN_11798fc0`, slot3 = 0 (феговый байт), slot4 = 3, slot5 = 4, slot7 = `"infos"`, slot9 =
  указатель на данные `&DAT_11f167b6` ↔ `&DAT_11f31ea8`, slot10 = `0xffffffff`, vtable `&PTR_FUN_11f90de0` ↔
  `&PTR_FUN_11fb4d70`, thunk purecall `&PTR_purecall_11f0ebec` ↔ `&PTR_purecall_11f321ac`. `0x1030f430` ↔
  `0x10317570` — та же форма, но slot1 = имя из аргументов, slot5 = `param_1`, slot7 = литерал `"command"`.
  `0x1030f130` ↔ `0x10317270` — деструктор: для слотов `0x12`/`0x13`/`0x14` выполняет
  `LOCK(); iVar1 = *piVar2; *piVar2 = *piVar2 + -1; UNLOCK(); if (iVar1 == 1) FUN_100945e0();` ↔ `FUN_10174710()`,
  затем два вызова `FUN_11788aa0()` ↔ `FUN_11799a30()` и сбрасывает vtable до `&PTR_FUN_11f0e9c4` ↔
  `&PTR_FUN_11f31f88` ⇒ встроенные атомические счётчики в объекте +0x48/+0x4c/+0x50.
- Сам объект записи (`FC3.dll 0x1030f200` ↔ `FC3_d3d11.dll 0x10317340`, **не** `0x10317450`) по ограниченному
  дизассемблеру `local/scratch/rd_fc3_1030f1f0.txt` / `local/scratch/rd_d3d_10317330.txt`: purecall vtable
  `0x11f0ebec` ↔ `0x11f321ac`, затем строковый литерал **`"Data"`** в объекте +4 (`0x11f0ecc0` ↔ `0x11f32280`),
  вызов помощника длины `0x11788030` ↔ `0x11798fc0` со значением в +8, промежуточная vtable `0x11f0edfc` ↔
  `0x11f323bc` с `+0x10 = 3` и повтор возвращённого значения в `+0x14`, `+0x18`, `+0x1c`, `+0x20`, затем
  настоящая vtable `&PTR_FUN_11f90dac` ↔ `&PTR_FUN_11fb4d3c`, и объектное поле +0x24 = `0x11341420` ↔
  `0x11346ca0`.
- Цель +0x24 — **3-байтовый thiscall-геттер**, побятно identical в обоих модулях:
  `mov al, byte ptr [ecx + 0x21]; ret` (FC3.dll `0x11341420`, FC3_d3d11.dll `0x11346ca0`). Это предикат флага,
  а не тик; ни один из двойников не экспортирован псевкодом Ghidra, поэтому pairing получен через
  `region_dump.py`.
- Пошаговый **update/execute** тик — `FC3.dll FUN_11341340` ↔ `FC3_d3d11 FUN_11346bc0` (`this` в `ECX`,
  диапазоны Ghidra `0x11341340-0x11341417` ↔ `0x11346bc0-0x11346c97`, реальный epilogue `ret 4` в `0x11341415` /
  `0x11346c95`, дальше padding с accessor-танками). Псевкод обоих идентичен построчно: флагные байты +0x55,
  +0x5f, +0x19, +0x1a, +0x5e, +0x17, +0x1c, +0x57, +0x26, +0x5d; вызывает `FUN_113401c0` ↔ `FUN_11345a40`, слот
  vtable объекта +0x54, `FUN_1133fbe0` ↔ `FUN_11345460`, `FUN_116332b0(100)` ↔ `FUN_11644220(100)`,
  `FUN_11341280` ↔ `FUN_11346b00`, пишет широкую строку **`L"ExecuteEvent"`** через `FUN_1030fb90` ↔
  `FUN_10317cd0` (снимая флаг +0x57), затем `FUN_10c47220` ↔ `FUN_10c4d740`. Последняя проверка обращается к
  синхронизатору `DAT_12805180` ↔ `DAT_1282a2f8`: читает байт `(*(синхронизатор + 4) + 0x205)`, вызывает слот
  vtable +0x28 и при успехе устанавливает **`DAT_12805e42 = 1`** ↔ **`DAT_1282afba = 1`**.
- Потребители этой флаг-пары: `FC3.dll 0x10c47320`/`0x10c473b0`/`0x10c477c0`/`0x10c47a00` ↔ `FC3_d3d11.dll`
  `0x10c4d840`/`0x10c4d8d0`/`0x10c4dce0`/`0x10c4df20` (читают и снимают `DAT_12805e41`/`DAT_12805e42` ↔
  `DAT_1282afb9`/`DAT_1282afba`). Внутри `FUN_10c473b0` ↔ `FUN_10c4d8d0` закрытый путь вызывает `FUN_1069fbe0`
  ↔ двойник и `FUN_10504b50(дескриптор, L"ExecuteEvent")` ↔ двойник — путь выполнения скриптового события,
  однократно для объекта (ставит guard-байт в объекте +0xe8).
- Смежные accessor-танки (FC3.dll ↔ FC3_d3d11.dll): `0x11341420` ↔ `0x11346ca0` (`mov al, byte ptr [ecx + 0x21]`),
  `0x11341430` ↔ `0x11346cb0` (`mov dword ptr [eax + 4], ecx`), `0x11341440` ↔ `0x11346cc0` (`mov al, byte ptr
  [eax + 0x5c]`), затем только в FC3_d3d11 `0x11346cd0` (ставит байт +0x5c), `0x11346ce0` (`add eax, 0x30`),
  pair-гатеры `0x11346cf0` (+0x28/+0x2c), `0x11346d00` (+0x18/+0x1c), `0x11346d10` (+0x10/+0x14), сеторы
  `0x11346d20` (+0x40), `0x11346d30` (+0x3c), `0x11346d40` (+0x38). Все заканчиваются `ret` и лежат в paddingе
  после тика.
- Доступные по этому же адресу accessor-танки FC3.dll: `0x11341430` (`mov dword ptr [eax + 4], ecx`) и
  `0x11341440` (`mov al, byte ptr [eax + 0x5c]`).

### Владелец реестровых таблиц: guarded-фабрика и её vftable (2026-11-05)

Тупик «у thunk-обёрток `0x118531c0` / `0x11863de0` нет E8-вызывающих, только по одной абсолютной ссылке на
каждую» **решён: это слоты виртуальной таблицы**, поэтому записи о таблице слотов в data directories PE нет —
указатель лежит в vftable в `.rdata`, которую устанавливает фабрика из `.text`.

| FC3.dll | FC3_d3d11.dll | роль |
|---|---|---|
| `FUN_111e9540` (`0x111e9540`-`0x111e9597`) | `FUN_111eeed0` (`0x111eeed0`-`0x111eef27`) | guarded-создатель: выделяет объект и ставит его vftable |
| `&PTR_FUN_11fbb2c0` (`.rdata 0x11fbb2c0`) | `&PTR_FUN_11fdf198` (`.rdata 0x11fdf198`) | vftable, которую она устанавливает |
| слоты `+0x00 0x1039ed60`, `+0x04 0x1039ed00`, `+0x08 0x104833e0` | `0x103a5e90`, `0x103a5e30`, `0x10005920` | другие виртуальные слоты |
| слот **`+0x0c` = `0x118531c0`** | слот **`+0x0c` = `0x11863de0`** | thunk «обеспечь таблицу»: строит таблицу реестров один раз, затем возвращает глобал-контейнер |
| слоты `+0x10 0x113418e0`, `+0x14 0x1033ebf0`, `+0x18 0x1039ed50`, `+0x1c 0x104833e0` | `0x11347160`, `0x1013e460`, `0x103a5e80`, `0x10005920` | остальные слоты |
| слот `+0x20 0x11853120` | слот `+0x20 0x11863d40` | соседний accessor |

Pseudocode фабрики, идентичный по строкам индексов (`local/scratch/rea/pseudo-fc3dll-v2/functions/0x111e9540.c` ↔
`pseudo-d3d11-v2/functions/0x111eeed0.c`):

```c
if (DAT_128042fc == '\0') FUN_11789a00();        if (DAT_128293f4 == '\0') FUN_1179a990();
if (DAT_12162beb != '\0')                          if (DAT_12187c6b != '\0')
  { if (FUN_1177f7c0() != 0) goto ok; }             { if (FUN_11790750() != 0) goto ok; }
if (FUN_118db560() == 0) return 0;               if (FUN_118e8340() == 0) return 0;
ok: puVar2 = FUN_11341d40();                     ok: puVar2 = FUN_113475c0();
    *puVar2 = &PTR_FUN_11fbb2c0;                     *puVar2 = &PTR_FUN_11fdf198;
    return puVar2;                                 return puVar2;
```

- Тот же тройной гейт, что и у описанных выше дескриптор-обёрток (`FUN_11789a00`/`FUN_1177f7c0`/`FUN_118db560` ↔
  `FUN_1179a990`/`FUN_11790750`/`FUN_118e8340`): фабрика лежит в той же подсистеме «инициализировать один раз»;
  двойники алкаторa — `FUN_11341d40` ↔ `FUN_113475c0`.
- Thunk слота `+0x0c` охраняется флаг-байтом, FC3.dll `0x1280562f` ↔ FC3_d3d11.dll `0x1282a7a7`: `cmp byte
  [flag],0; jne skip; call 0x11853140 / 0x11863d60; mov byte [flag],1; ret`, а вариант, который хранится в слоте,
  заканчивается `mov eax,0x128334a0` / `mov eax,0x12856970` = **вернуть глобал-контейнер**. Таблица реестров
  строится **один раз, лениво, при первом виртуальном вызове слота `+0x0c`**; `0x11853140`/`0x11863d60` —
  исполнители (построение записи + копия, выше).
- Рядом с vftable идут строки описателей классов: `CFCXGOContextNode`, `CFCXGOContextFlagsNode`, `IGameFileData`,
  `CCampaignGameFileData`, `CFCXGameModeContextNode`, `CFCXGameModeContextFlagsNode`, `CFCXGameMode` (FC3.dll
  `0x11fbb294`, `0x11fbb2a8`, `0x11fbb2e4`, `0x11fbb2f4`, `0x11fbb34c`, `0x11fbb364`, `0x11fbb384`; FC3_d3d11.dll
  `0x11fdf16c`, `0x11fdf180`, `0x11fdf1bc`, `0x11fdf1cc`, `0x11fdf224`, `0x11fdf23c`). Владелец реестровых таблиц —
  семейство **game-object / game-file-data / game-mode context**, то есть семейство, которое несёт запрос сессии;
  поэтому значения `-exec` / `-execline` попадают в параметры сессии (`execLines`), а не выполняются как консольная
  команда.
- Ссылающийся dword лежит в `.text` в обоих модулях (FC3.dll `0x111e9588` → `0x11fbb2c0`, FC3_d3d11.dll
  `0x111eef18` → `0x11fdf198`); это операнд `mov *puVar2, offset …` самой фабрики, а не отдельная таблица.
  Вторая `.text`-ссылка на соседний описатель: FC3.dll `0x1039edd4` → `0x11fbb2e4` (`IGameFileData`) ↔ FC3_d3d11.dll
  `0x103a5f04` → `0x11fdf1bc`.
- **Как доходит до фабрики.** У `FUN_111e9540` / `FUN_111eeed0` — 0 вызовов `E8` и 9 абсолютных ссылок, по одной
  на каждый элемент реестра родственных-классов in `.rdata`. FC3.dll `0x11fbaf44` (файл `0x1fba144`) ↔
  FC3_d3d11.dll `0x11fdee1c` (файл `0x1fdd41c`), шаг `0x28`, элементы `af44, af6c, af94, afbc, afe4, bb00c,
  bb034, bb05c, bb084` ↔ `ee1c, ee44, ee6c, ee94, eebc, eee4, ef0c, ef34, ef5c`; реестр лежит на `0x37c` байт
  раньше vftable. Элемент 0 FC3.dll `111e9540 111e94e0 1030d1d0 1039e3c0 104833e0 111e9fd0 111e8e10 113433b0
  111e9460 111e95a0` ↔ FC3_d3d11 `111eeed0 111eee70 103a56f0 103a5470 10005920 111ef960 111ee7a0 11348c30
  111eedf0 111eef30`: поле 0 = guarded-создатель, поле 1 = его смежная точка входа, поля 4 (`0x104833e0` ↔
  `0x10005920`), 7 (`0x113433b0` ↔ `0x11348c30`) и 9 (`0x111e95a0` ↔ `0x111eef30`) одинаковы в элементах 0-7,
  поле 2 = `0x1030d1d0` ×4 / `0x1039e640` ×3 ↔ `0x103a56f0` ×4 / `0x103a5750` ×3, поле 3 идёт по серии с шагом
  `0x50` (`0x1039e3c0,410,460,4b0,500,550,5a0,5f0` ↔ `0x103a5470,4c0,510,560,5b0,600,650,6a0`). **Элемент 8 —
  другое семейство** (FC3.dll `100519c0 1030df50 10122130 104ef020 1039e780 103feb40 102dac70 100055a0` ↔
  FC3_d3d11 `1027d850 102a8ea0 10241570 1046b300 103a5890 10104c00 10316060 10005880`). Форма — один создатель
  на девять родственных-классов — соответствует именам описателей выше (`CFCXGOContextNode`,
  `CFCXGOContextFlagsNode`, `IGameFileData`, `CCampaignGameFileData`, `CFCXGameModeContextNode`,
  `CFCXGameModeContextFlagsNode`, `CFCXGameMode`). Файл пробы: `local/scratch/fc3_region_dump.py` — выводит диапазон `.rdata` в виде dword-ов
  с декодированием names/strings (`<pe> <va_start> <va_end> <out>`); нужен image base (оба modules 1.06 —
  `0x10000000`, PE32 magic `0x10b`), потому что таблица секций хранит RVAs — FC3.dll `.rdata`
  = VA `0x11bcb000`..`0x12092d21` from file `0x1bca200`, FC3_d3d11.dll `.rdata` = VA
  `0x11be1000`..`0x120b7347` from file `0x1bdf600`. Открытые вопросы этой области: какой из девяти
  элементов владеет реестровой таблицей (`0x118531c0` ↔ `0x11863de0` — её слот `+0x0c`), кто итерирует реестр по адресам
  `0x11fbaf44` ↔ `0x11fdee1c` во время выполнения, и что собой представляет семейство элемента 8.
- Файлы проб: `local/scratch/rd_reg_table_fc3.txt`, `local/scratch/rd_reg_table_d3d.txt`,
  `local/scratch/rd_table_slot_scan.txt` (принадлежность слотов + сырые байты),
  `local/scratch/rd_snapshot_callers.txt` (guarded-одноразовые thunks). `local/scratch/rd_disasm.py` требует
  **четыре** аргумента `<pe> <start_va> <end_va> <out_file>`; с тремя он не печатает ничего.

**Открытые вопросы этой области:** ~~декомпилировать `0x10638fb0` (FC3_d3d11.dll) и `0x1003b4b0`
(FC3.dll), чтобы получить тип элемента exec-записи~~ — решено: 4-байтовые указатели, добавляются по
количеству. Подтвердить, что потребители
`FC3_d3d11.dll 0x11348360` / `FC3.dll 0x11342a2e` обходят `[0x12852dcc]` / `[0x1282f8fc]` в порядке
регистрации; кто записывает разобранную строку `-exec` в контейнер `[0x12829424]` под ключом `execLines`
(известно место вызова `0x11785e60`, возвращаемое смещение `[eax+0x28]`); ~~какая функция владеет
слот-9 указателем~~ — решено: `0x11341420` ↔ `0x11346ca0` — побятно identical flag-гатеры
(`byte [this + 0x21]`), а тик, который они охраняют — `FC3.dll FUN_11341340` ↔
`FC3_d3d11 FUN_11346bc0`; и достижим ли `Exec`
(`0x11fbda40`, указатель обработчика сохранён в `0x111f5c71`) из этого контейнера при запуске.
- Осталось: кто вызывает слот vftable `+0x0c` (`0x118531c0` ↔ `0x11863de0`) — у виртуального вызова нет формы
  ближнего `E8`, поэтому следующий шаг — вызывающие фабрики (`FUN_111e9540` ↔ `FUN_111eeed0`) и вызов создания
  сессии, который база знаний coop-anything относит к **слоту vftable `+0x74`** объекта сессии (FC3.dll
  `0x11a8d0c5`-`0x11a8d0eb`: `call 0x11341dc0`; `mov ecx,[eax]`; `mov eax,[ecx]`; `mov eax,[eax+0x74]`;
  `call eax`; деструктор сессии `0x11343640`, vftable `0x11f90644`, освобождаемые поля `[obj+0x98]`/`[obj+0x9c]`).
- Функции FC3_d3d11 `0x111eed50`, `0x111eedf0`, `0x111eec50`, `0x111eecb0` читают и пишут поля `+0x98` и `+0xa4`
  (`if (*(int *)(in_ECX + 0xa4) == -1 && *(int *)(in_ECX + 0x98) != 0) { *(undefined4 *)(in_ECX + 0xa4) =
  *(undefined4 *)(*(int *)(in_ECX + 0x98) + 0x10); … }`) — это совпадает с раскладкой `[obj+0x98]` объекта сессии; их
  pairить следующими. Дальнейший шаг: 180-байтовый struct параметров сессии в `FC3.dll 0x11f31d8c` ↔
  `FC3_d3d11.dll 0x11f54b70` либо `0x11f31a80` ↔ `0x11f548a0` — выгрузить диапазон и декодировать его dword как
  пары строка/указатель.
- Новое (та же область): являются ли эти записи контейнерами `std::vector` (триады base/count/
  capacity) — статически `[0x12852dcc]` и сосед `[0x12852dd0]` равны адресу inline-хранения
  `0x12856968`, т.е. пустой контейнер; следующий зонд — `FC3.dll 0x113429b0` ↔
  `FC3_d3d11.dll 0x11348230` и `FC3.dll 0x11341fe0` ↔ `FC3_d3d11.dll 0x11347860`.

### Таблица записей находится внутри цепочки из 57 статических контейнеров (2026-11-05)

Эти глобалы обслуживают два семейства accessor-ов; оба найдены `scripts/fc3_reloc_xref.py
--mode reloc` — режим `code` видит только формы `ModRM` с displacement и пропускает `mov reg,
imm32`, поэтому для этих глобалов надёжен reloc-режим.

- Семейство 1, живые контейнеры. Каждый accessor — двухинструкцияный thunk `mov eax, imm32` затем
  `jmp 0x10004b70` (FC3.dll) / `jmp 0x10004d70` (`FC3_d3d11.dll`), thunk'и с шагом `0x10`, возвращают
  глобалы в основном с шагом `0x18`. Цепочка, которой принадлежит таблица записей, насчитывает
  **57 accessor-ов**: `FC3.dll 0x11bb33f0` .. `0x11bb3780` с глобалами `0x1282f6cc` .. `0x1282fca8`, зеркало
  `FC3_d3d11.dll 0x11bc8950` .. `0x11bc8ce0` с глобалами `0x12852b9c` .. `0x12853178`. Внутри неё
  таблица exec-записей — `FC3.dll 0x11bb3560` → `0x1282f8fc` ↔ `FC3_d3d11.dll 0x11bc8ac0` →
  `0x12852dcc`, а таблица, добавленная к этой паре 2026-11-03 — accessor на слот раньше: `FC3.dll
  0x11bb3550` → `0x1282f8e4`/`0x1282f8e8` ↔ `FC3_d3d11.dll 0x11bc8ab0` → `0x12852db4`/`0x12852db8`.
  Прежний вариант этого раздела прочитал это окно как семейство из семи элементов (`0x11bb3510` ..
  `0x11bb3570`); это хвост данной 57-элементной цепочки, а не её полный диапазон.
- Замер по всему модулю той же формы thunk'а (`scripts/fc3_accessor_thunks.py`, скан `.text`): в
  FC3.dll
  **3228** thunk'ов `mov eax, imm32` + `jmp 0x10004b70` в **523** непрерывных цепочках; в
  `FC3_d3d11.dll` столько же **3228** в тех же **523** цепочках. Здесь важны две из них: 57-
  элементная цепочка таблицы записей выше и 19-элементная цепочка вторичного блока: `FC3.dll
  0x11bb6430` .. `0x11bb6550` (глобалы `0x128333a8` .. `0x12833564`) ↔ `FC3_d3d11.dll 0x11bcb980` ..
  `0x11bcbaa0` (глобалы `0x12856878` .. `0x12856a34`).
- Счёт patch-сайтов reloc-режима зеркален по модулям: `--mode reloc 0x1282f884` (FC3.dll) → 36
  сайтов; `--mode reloc 0x12852d54` (FC3_d3d11.dll) → 36 сайтов. Сайты группируются триадами с
  шагами `+0`, `+0x12`, `+0x27` — это три поля одного контейнера, записываемые в каждом routine:
  инициализация `FC3.dll 0x10f92e98`/`0x10f92eaa`/`0x10f92ec7` ↔ `FC3_d3d11.dll 0x10f986f8`/
  `0x10f9870a`/`0x10f98727`; consumers `FC3.dll 0x11341f28` .. `0x11342577` ↔ `FC3_d3d11.dll
  0x113477a8` .. `0x11347df7`; teardown `FC3.dll 0x113439f8`/`0x11343a0a`/`0x11343a27` ↔
  `FC3_d3d11.dll 0x11349278`/`0x1134928a`/`0x113492a7`; accessors `FC3.dll 0x113415f1` ↔
  `0x11346e71` и `0x11bb3511` ↔ `0x11bc8a71`.
- Семейство 2, вторичный блок, который заполняют все контейнеры (та же 19-элементная
  цепочка). `FC3.dll 0x11bb6440` .. `0x11bb64d0` возвращают `0x128333c0`, `0x128333d8`, `0x128333f0`, `0x12833408`, `0x12833420`,
  `0x12833438`, `0x12833450`, `0x12833468`, `0x12833480`, `0x128334a0` (шаг `0x18`, последний шаг
  `0x20`); `FC3_d3d11.dll 0x11bcb9c0` .. `0x11bcba10` возвращают `0x128568d8`, `0x128568f0`,
  `0x12856908`, `0x12856920`, `0x12856938`, `0x12856950`. `FC3_d3d11.dll 0x11b73160` ..
  `0x11b73177` разбирает один элемент этого блока: `xor eax,eax` / `push 0x11bcba10` /
  `mov [0x12856958],eax` / `mov [0x12856964],eax` / `call 0x1070085d` (освободить контейнер с
  базой `0x12856968`, передав его собственный accessor), затем `0x11b73180` `push 0x11f31b44
  "Main"; call 0x11798fc0`.
- Этот контейнер — тот же самый, который возвращает слот vftable `+0x0c`. `FC3.dll 0x11853140` ↔
  `FC3_d3d11.dll 0x11863d60` — once-init под accessors'ом (byte-guard `0x1280562f` у FC3.dll,
  `0x1282a7a7` у FC3_d3d11.dll): вызывает `FC3.dll 0x113429b0` ↔ `FC3_d3d11.dll 0x11348230`,
  читает `[0x1282f8fc]`/`[0x1282f900]` ↔ `[0x12852dcc]`/`[0x12852dd0]` с полем ёмкости
  `[0x128334a4]` ↔ `[0x12856974]`, дописывает через `FC3.dll 0x1003b4b0` (проверенный `call` в
  `0x1185317a`) ↔ `FC3_d3d11.dll 0x10638fb0` (проверенные места вызова `0x11863d9a` в once-init
  шестого контейнера и `0x11863cfa` в once-init пятого), и копирует через `FC3.dll
  0x106f9ff0` ↔ `FC3_d3d11.dll 0x10700c10` (`push edi; push ebx; push eax; call 0x10700c10; add
  esp,0xc`). Парный once-init `FC3.dll 0x118530a0` ↔ `FC3_d3d11.dll 0x11863cc0` проделывает то же
  для пятого контейнера (`[0x1282f8e4]`/`0x1282f8e8` ↔ `[0x12852db4]`/`0x12852db8`, поля
  `0x128334b8`/`0x128334bc` ↔ `0x12856988`/`0x1285698c`) через `FC3.dll 0x11341fe0` ↔
  `FC3_d3d11.dll 0x11347860`. Их вызывающие — только служебные (`0x118531a9`/`0x118531c9` и
  `0x11853109`/`0x11853129` ↔ `0x11863dc9`/`0x11863de9` и `0x11863d29`/`0x11863d49`): обе функции
  достигаются как слоты vftable `+0x0c` и `+0x20` и через их собственный once-init, а не как ближний
  вызов.

## TODO

- **Адресной аудит строк 1.06 и блок полей GameContext.** Проверено прямыми чтениями `FC3_d3d11.dll`
  по отображению `.rdata` (смещение файла = VA - `0x11be1000` + `0x1bdf600`; проверка: строка `execLines`
  в файле `0x1fb3080` = VA `0x11fb4a80`). Строка `-coop` начинается в `0x11fdc0cc`, `-numclients` —
  `0x11fb1e18`; у `-numclients` есть ссылка `0x1135381e` (в цепочке реестра сессий), а у `-splitscreen`
  и `-splitscreen4` ссылок нет. Опции спойла разобраны в трёх группах: `-spawn` `0x11aa0eb8`/`0x11aa0ecb`,
  `-spawnpos` `0x11aa0f09`/`0x11aa0f20`, `-spawnangle` `0x11aa0fbd`/`0x11aa0fd4` (функция `0x11aa0670`);
  `0x11aa15f3`/`0x11aa1606` … `0x11aa16fb`/`0x11aa1712` (функция `0x11aa1b90`) и `0x11aa2777`/`0x11aa278a`
  … `0x11aa286b`/`0x11aa2882` (третья функция разбора). Имена полей узла контекста игры идут подряд
  (`world` `0x11f46910`, `spawnAt` `0x11f4692c`, `spawnPos` `0x11f46934`, `spawnAngle` `0x11f46940`,
  `usePosAngle` `0x11f4694c`) и все четыре ссылает одно место — `0x1170fdba`, `0x1170fdcf`, `0x1170fde7`,
  `0x1170fe03`. Имена реестра сессий `spawnPtId` `0x11fb4c98` → `0x11347f0b`, `keepPlayerState`
  `0x11fb4ca4` → `0x11347f20`, `touched` `0x11fb4c90` → `0x11347e68`, `mode` `0x11fb4c88` → `0x11347f6c`
  и `0x11347f9c`. В `0x11fdbdc4`, `0x11fdbde4`, `0x11fdbe0c`, `0x11fdbe30` лежат готовые строки с
  уже подставленными значениями: `' -spawnpos=333.63,964.5,124.465'`, `' -spawnangle=0.0,0.0,0.0'`,
  `' -spawnpos=-857.626,2317.1,119.752'`, `' -spawnangle=0.0,0.0,-20.0'`; они ссылает парсер `0x11aa2fe0`
  (`0x11aa3aeb`, `0x11aa3afe`, `0x11aa394c`, `0x11aa395f`) — то есть игра уже умеет собирать полную
  командную строку запуска со спойлом. Отсутствие в 1.06 имён `sessionName`, `sessionDescription`,
  `sessionFilter1/2`, `clientName`, `serverName`, `sessionExecFile`, `execFilenames2`, `bundleName`,
  `bundleDir`, `bundlePath`, `sessionPath`, `sessionParams`, `spawn_angle` подтверждено (0 вхождений).

- **Путь spawn-данных решён (проверено дизассемболом).** Все четыре ссылки на spawn-поля GameContext
  (`0x1170fdba`, `0x1170fdcf`, `0x1170fde7`, `0x1170fe03`) — это thunk отложенной регистрации
  дескриптора `0x1170fd70` (проверка `cmp byte [0x128299c8],0`): `push <смещение>; push <VA имени>;
  mov eax,0x1282db10; call 0x11acaba0|0x11acac50|0x11acae60`. Карта полей узла `0x1282db10`: `world`
  +0x08, `mergedBigFileName` +0x10, `spawnAt` +0x0C (строка), `spawnPos` +0x14 (три float), `spawnAngle`
  +0x20 (три float), `usePosAngle` +0x2C (байт), `loadingdepActive` +0x2D. Парный thunk `0x1170fe40`
  регистрирует `networkGame`/`host` на другом структе (`0x1282daf8`). Запись происходит только в трёх
  парсерах: цепочка всегда одна — `strstr [0x11be1618]` → извлекатель значения `call 0x11a1b940` →
  `sscanf [0x11be1398]` с форматом `'%g,%g,%g'` (`0x11f39f50`) → доступ к узлу (`mov eax,[esi+0x9c]`;
  `mov edi,[eax+4]`; `mov eax,[edx+0x18]`; `call eax`; результат кэшируется в `[edi+8]`) → запись.
  `-spawn` → `call 0x11347550` (`0x11aa0eeb`, близнец `0x11aa27a3`) пишет строку со ссылками в
  узел +0x0C (двойник `0x113474e0` с `add eax,8` пишет `world` в +0x08). `-spawnpos` пишет
  `movss [eax+0x14]`, `+0x18`, `+0x1c` и ставит `mov byte [eax+0x2c],1` без перевода градусов
  (`0x11aa0f8a`–`0x11aa0f9e`, также `0x11aa16c8`–`0x11aa16de`, `0x11aa2839`–`0x11aa284d`).
  `-spawnangle` загружает `0x11f32198` = 0,0174532924 (1 градус в радианах), умножает все три
  компонента (`mulss`, `0x11aa1031`–`0x11aa1039`) и пишет `movss [eax+0x20]`, `+0x24`, `+0x28`
  (`0x11aa1014`–`0x11aa1084`, также `0x11aa1723`–`0x11aa17d0`, `0x11aa288d`–`0x11aa28b6`) — игра
  хранит **радианы**. Парсер exec также собирает целые командные строки: вариант A
  (`0x11aa3909`–`0x11aa3967`, затем `call 0x11aa2630` в `0x11aa3989`) = `' -world=fc3_main'`
  (`0x11fdbdb0`) + `' -spawnpos=-857.626,2317.1,119.752'` + `' -spawnangle=0.0,0.0,-20.0'` после
  `'Gamescom'` (`0x11fdbe00`); вариант B (`0x11aa3aaa`–`0x11aa3b06`) с `333.63,964.5,124.465` и
  `0.0,0.0,0.0`; рядом `-benchmark` (`0x11fdbe4c` → `call 0x11aa14b0` в `0x11aa39ed`), `-host`
  `0x11f31c7c`, `-client` `0x11f31d18`, `'multi_SkipIntroMovie 3'` `0x11fdbd30`,
  `'starMatch_with_debugCameras 1'` `0x11fdbd10`. **Отдельного спойла на игрока нет:** `spawnPtId`
  `0x11fb4c98` и `keepPlayerState` `0x11fb4ca4` встречаются только внутри `0x11347e40` — одноразовой
  регистрации (`cmp byte [0x1282a610],0` → `jne 0x11347f2c`), которая выделяет ровно одну запись
  `0x10+0x20` (`call 0x11790750`), добавляет её единственным `push 1` в массив по 4 байта на запись
  (`[0x12852e2c]`/`[0x12852e30]`, отдельный контейнер, не `[0x12852dcc]`/`[0x12852dd0]` с execLines)
  и регистрирует поля от `lea esi,[edi+0x14]`: `world` +4, `pos` `0x11f39450` +8, `spawnPtId` +0x18
  (`call 0x11acaf10` в `0x11347f11`), `keepPlayerState` +0x20 (`call 0x11acac50` в `0x11347f24`).
  Нет петли, индекса и перебора игровых слотов → спойл один на всех. РЕШЁНО (2026-11-02, локальная
  проверка дизассемблером по `.text` файла 1.06): адреса быстрой загрузки `0x111e1e54`/`0x111e2970`/
  `0x111e4140` — не потребители спойла, а писателя `[edi+0x430]`/`[edi+0x434]` не существует (полный скан
  `.text` не нашёл `lea ecx,[edi+0x430]`; только 4 mov-подобных обращения к `+0x430`/`+0x434` вне спойла:
  `0x10544b8d`, `0x10e6356d`, `0x10e63597`, `0x1160ab44`). `0x111e2970`/`0x111e2984` лежат внутри функции
  `0x111e27b0`, `0x111e4140` — внутри `0x111e3f30`, а `0x11057c60`/`0x10404f40` не имеют пролога в пределах
  0x900 байт. `0x111e1e54` — тонкая обёртка (`mov eax,[0x1282a7b4]; call 0x111e9bd0; ret 4`), а
  `0x111e9bd0` — чистый диспетчер (`call 0x111e7300`, `0x102745d0`, `0x11706240`, `0x11310be0`, `0x1013b1c0`,
  `0x106bd910`), spawn-смещения не трогает; сингл `[0x1282a7b4]` (91 ссылок) не содержит указателя на узел
  `0x1282db10` (0 вхождений вне `.text`). Сам узел `0x1282db10` имеет 13 ссылок — все в цепочке
  регистрации/потребления (`0x1125ed11`, `0x1170f491`, `0x1170fc5b`, `0x1170fc6d`, `0x1170fd8e`..`0x1170fe36`,
  `0x11bbb1f1`). Проверочные скрипты: `local/scratch/ql_local.py`, `ql2.py`, `ql3.py`, вывод в
  `local/scratch/out/ql_local.txt`, `out/ql2.txt`, `out/ql3.txt`.
- [x] Уточнить версия игры и путь у владельца (1.06, `H:\Uplay\Far Cry 3\`)
- [x] Подготовить тестовую копию FC3 1.06 (`H:\FarCryCoop\TestFC3\`, вне папки установки)
- [x] Изучить результаты Фазы 1 из Bantila/FarCry3Coop (см. `NOTES.ru.md`, `PLAN.ru.md`)
- [x] Решение принято (2026-11-02): отдельный форк этого репозитория `peperonid/FarCry3Coop`
    (fork от Bantila/FarCry3Coop) — рабочая копия этой игры; форк конвейера `peperonid/FarCryCoop`
    остаётся для других игр (`games/far-cry`, `games/far-cry-6`). Ветка `backup` владельца не трогать.
- [ ] Фаза 2: архитектурный спайк (неткод движка vs свой UDP)
- [ ] Изучить мод scubrah для запуска острова в ко-опе
- [ ] Зондировать структуру параметров сессии в `FC3.dll` 1.05 `0x10a8d0db` (`push <имя>; push 0x1c;
  call 0x10a46480` — helper регистрации опций, тип 8 в 1.06) и vftable слот 0x74 сессионного интерфейса —
  через них `-exec`/`-execline` попадают в запрос ко-оп сессии; размеры структуры в 1.05 подтвердить
  динамически (статическая проверка «180 байта» не подтверждена)
- [ ] Динамическая зацепка для исполнителя `execLines`: запустить тестовую копию с `-exec <файл>`
  (или `-execline`) и зафиксировать, как строка попадает в 180-байтную структуру сессии; параллельно
  разобрать в Ghidra accessor-thunkи `0x11bc8a80`..`0x11bc8ad0` и `0x10004d70`
- **Таблица регистрации launch-режимов `FC3.dll 0x1119dec0 .. 0x1119df9a` и `0x1119f910 .. 0x1119fa42`**
  (проверено 2026-11-06, `local/scratch/probe36.txt`, `probe37.txt`). `0x1119dec0` кладет id-глобалы в
  объект `[0x12804acc]`: на каждый вызов `mov eax, dword ptr [0x12804acc]` + `add eax,4` +
  `push <VA id-глобалы>` + `call 0x1029eb00` (хелпер `0x1029eb00` = регистрация id). Порядок такой же,
  как в таблице `0x1111e390` / `0x1111e860`: `0x1216caac` (`0x1119dec6`), `0x1216ca9c` (`0x1119ded8`),
  `0x1216ca90` (`0x1119deea`), `0x1216caa4` (`0x1119defc`), `0x1216caa8` (`0x1119df0e`), **`0x1216aee4`
  `-coop` (`0x1119df20`)**, `0x1216aedc` (`0x1119df32`), `0x1216af0c` (`0x1119df44`), `0x1216aeec`
  (`0x1119df56`), `0x1216aef4` (`0x1119df68`), `0x1216aefc` (`0x1119df7a`), `0x1216af04` (`0x1119df8c`),
  `pop ecx; ret` в `0x1119df99`. `0x1119f910` привязывает обработчик к id: `mov esi, dword ptr
  [0x12804acc]` + `add esi,4` + `push <VA обработчика>` + `push <VA id-глобалы>` + `call 0x1029edc0`
  (парами: `0x11152dd0`↔`0x1216caac` в `0x1119f91d`, `0x11152f50`↔`0x1216ca9c` в `0x1119f935`,
  `0x11a7a2e0`↔`0x1216ca90` в `0x1119f94d`, `0x11a5a290`↔`0x1216caa4` в `0x1119f965`, `0x11a5a0b0` ↔
  `0x1216caa8` в `0x1119f97d`, **`0x11a5a6e0`↔`0x1216aee4` (co-op, `0x1119f995` / `0x1119f99f`)**,
  `0x11a7aff0`↔`0x1216aedc` в `0x1119f9ad`, `0x11a7aa00`↔`0x1216af0c` в `0x1119f9c5`, `0x11a7ae80` ↔
  `0x1216aeec` в `0x1119f9dd`, `0x11a7ad00`↔`0x1216aef4` в `0x1119f9f5`, `0x11a7ab80`↔`0x1216aefc` в
  `0x1119fa0d`, `0x11a5a560`↔`0x1216af04` в `0x1119fa25`, `pop esi; ret` в `0x1119fa42`).
- **Конструктор co-op режима `FC3.dll 0x11a5a6e0 .. 0x11a5a772`** (обработчик, привязанный к `-coop`):
  шлюзы `cmp byte ptr [0x128042fc],0` → `call 0x11789a00` (`0x11a5a6ea..0x11a5a6f2`) и `cmp byte ptr
  [0x12162beb],0` (`0x11a5a6f7`); размер объекта `mov edx,0x10; lea ecx,[edx+0x30]` (=0x40) +
  `call 0x1177f7c0` (`0x11a5a6ff..0x11a5a707`, запасной путь `mov eax,0x10; lea edi,[eax+0x30];` +
  `call 0x118db560` в `0x11a5a710..0x11a5a718`); инициализация `[obj] = 0x11f23408` (`0x11a5a721`),
  поля `+4`..`+0x2c` обнуляются, `+0x30 = 0xffffffff` (`0x11a5a748`), `+0x34` (слово) и `+0x36` (байт)
  тоже 0; затем виртуальная таблица → **`0x11fbb838`** (`0x11a5a756`) и `mov ecx, [0x1216aee4]` (`0x11a5a75c`) →
  `mov dword ptr [eax + 0x30], ecx` (`0x11a5a762`); при отказе шлюза возвращает 0 (`0x11a5a76b`).
  Смещение `+0x30` — ровно то, что читают co-op проверки: `[0x12804acc]` → `+0x68` →
  `mov eax,[eax+0x30]` → `cmp eax,[0x1216aee4]` в `0x1111e4d4/0x1111e4e3` и
  `0x1111e907/0x1111e916/0x1111e91e`.
- **Вторая цепочка launch-режимов, вход RESOLVED: `FC3.dll 0x11a8c8b0 .. 0x11a8d110`** (ранний
  «mid-instruction» диапазон `0x11a8c944 .. 0x11a8c9c1` — внутри этой функции). Аргументы разбираются
  через `mov edi, dword ptr [0x11bcb5c0]` (`0x11a8c8bc`) + `call edi`: `-login` `0x11fb8170`
  (`0x11a8c8c2`); путь ошибки `push 0x11fb81b4` = `'Invalid parameters for -host, missing (-login)'`
  (`0x11a8c8d6`, `0x11a8c8e6`) → `call 0x10a00be0` / `call 0x10001ed0` → `xor al,al; ret 0xc`
  (`0x11a8c8f2..0x11a8c8fa`); далее `-host` `0x11f0e6ac` (`0x11a8c90b`, геттер `call 0x11a08190`),
  `-bfname` `0x11fb7d64` (`0x11a8c919`), `-dedicated` `0x11f0e7dc` (`0x11a8c927`), `-sessionuid`
  `0x11fb81f0` (`0x11a8c935`), `-deathmatch` (`0x11a8c943`), `-coop` `0x11fb81fc` (`0x11a8c95c`),
  `-domination` `0x11fb8204` (`0x11a8c976`), `-headquarters` `0x11fb8210` (`0x11a8c990`), `-firestorm`
  `0x11fb8220` (`0x11a8c9a6`), `-dash` `0x11fb822c` (`0x11a8c9c0`). Id режима попадает в локальный слот
  `[ebp+0xc]` (`-coop`: `0x11a8c96b mov ecx,[0x1216aee4]` затем `0x11a8c971 mov [ebp+0xc],ecx`;
  `-deathmatch`: `0x11a8c952 mov eax,[0x1216aedc]` затем `jmp 0x11a8c9df`; default `0x11a8c9da
  mov eax,[0x1216af0c]` затем `0x11a8c9df mov [ebp+0xc],eax`) и сохраняется в launch-объект:
  `0x11a8c9e2 mov ecx,[ebp+0xc]` → **`0x11a8c9ea mov dword ptr [esi + 0xa4], ecx`**, дальше виртуальные
  вызовы `[+0x14]` и `[+0x18]` (`0x11a8c9e5..0x11a8c9fb`), `push [ebp+0x10]; mov edi,esi;` +
  `call 0x119696d0` (`0x11a8c9fd..0x11a8ca03`), `lea edx,[esi+4]; push edx; push ebx;` +
  `call 0x11a8bdc0` (`0x11a8ca08..0x11a8ca0d`), `mov eax,[esi+0x9c]; lea edi,[ebp-0xc];` +
  `call 0x11341c60` (`0x11a8ca12..0x11a8ca1b`), `mov ecx,[edi+0x9c]; add eax,0x4c; cmp ecx,eax`
  (`0x11a8ca20..0x11a8ca2c`). Co-op ветка: `0x11a8cabb mov ecx,[ebp+0xc]` → `0x11a8cabe cmp ecx, dword
  ptr [0x1216aee4]` → `0x11a8cac4 jne 0x11a8cb17` → **`0x11a8cac6 push 0x11fb7fb8` = `-class`** →
  `push ebx;` + `call dword ptr [0x11bcb5c0]` (`0x11a8cacb..0x11a8cacc`) — `-coop` запрашивает имя
  класса режима; дальше `lea eax,[ebp+0xc]` + `mov dword ptr [ebp+0xc],0` + `call 0x11a08190`
  (`0x11a8cad9..0x11a8cae9`), поиск имени `mov eax,[esi+0x9c]` + `lea edi,[ebp+0xc]` + `call 0x11341b80`
  (`0x11a8caee..0x11a8cafa`), освобождение ссылки `lock xadd dword ptr [eax], edx` (`0x11a8cb09`) и
  `mov eax,edi; call 0x100945e0` (`0x11a8cb10..0x11a8cb12`), следующий аргумент `-spawn` `0x11fb7fc0`
  (`0x11a8cb17..0x11a8cb1d`).
- **Геттеры реестра режимов и маска доступности (проверено 2026-11-06)**: `FC3.dll 0x117094a0` и
  `0x117094d0` находят запись реестра по id режима — `ecx` = VA id-глобалы, `eax` = контейнер
  (`[0x128048c0]`): `mov edx, dword ptr [ecx]` (id), количество `mov ecx, dword ptr [eax+8]`, база
  массива `mov esi, dword ptr [eax+4]`, `lea ecx, [esi+ecx*4]`, цикл `cmp dword ptr [esi+0x20], edx`
  (`0x117094b4` / `0x117094e7`), при совпадении `mov eax, dword ptr [edx+0x24]` (`0x117094c7` /
  `0x117094fa`) — поле записи **`+0x24`**, которое `0x117094d0` проверяет маской: `test dword ptr
  [ebp+8], eax; setne al; ret 4` (`0x117094fd..0x11709504`), при промахе `xor eax,eax` и та же проверка
  (`0x11709507..0x1170950f`). Цикл шлюза по режимам — `FC3.dll 0x10f3cd30 .. 0x10f3d260`: маска из
  **`[ebx + 0xc4]`** (`0x10f3cfd0`, `0x10f3cff3`, `0x10f3d015`, `0x10f3d03c`, `0x10f3d063`), реестр
  `mov eax, dword ptr [0x128048c0]` (`0x10f3cfc9`, `0x10f3d01c`, `0x10f3d042`, `0x10f3d069`),
  кандидаты `0x1216af04` (`-dash`, `0x10f3cfd7`), `0x1216af0c` default (`0x10f3cffa`), `0x1216aeec`
  (`-domination`, `0x10f3d021`), `0x1216aef4` (`-headquarters`, `0x10f3d048`), `0x1216aefc`
  (`-firestorm`, `0x10f3d06f`); при успехе `push <VA id>` + `push edi` + `mov ecx, ebx` +
  `call 0x11258cc0` (`0x10f3d00d..0x10f3d010`), входной тест `cmp dword ptr [esp+0x20], 1`
  (`0x10f3cfc4`), конец `mov eax, edi; call 0x11257550` (`0x10f3d08a`). Это связано с `gamemodemask 128`
  в `worlds/downloadcontent/mp_dlc1/toc.rml` (обе co-op карты) — именно такую форму сравнивают поле
  записи `+0x24` и маска вызывающего `[obj+0xc4]`.
- [ ] Расшифровать, что разрешает аргумент `-class` в co-op ветке `0x11a8c8b0` (`0x11a8cac6..0x11a8cb17`)
- [ ] Найти вызывающих конструктор co-op режима `0x11a5a6e0` и кто пишет в `[0x12162beb]` / `[0x128042fc]`
  (его шлюзы)
- [x] Часть решено 2026-11-06: этот участок — цепочка поорежимной установки менеджера режимов (см.
  раздел «Объект-менеджер режимов и реестр режимов» ниже). Ещё не прочитано: начало функции
  `0x1119fa50` (400 инструкций, уже даётся в `local/scratch/probe36.txt`) и поорежимные конструкторы
  объектов `0x11a5a0b0`, `0x11a5a290`, `0x11a5a560`, `0x11a7a2e0`, `0x11a7aa00`, `0x11a7ab80`,
  `0x11a7ad00`, `0x11a7ae80`, `0x11a7aff0`
- [x] Решено 2026-11-06: это тот же объект, он устанавливается в `0x117567ce`; см. раздел
  «Объект-менеджер режимов и реестр режимов» ниже. Предположение о разных формах (регистрация кладёт
  **адреса** id-глобал вроде `push 0x1216aee4`, а co-op проверка сравнивает **значение** id)
  оказалась неверной: `+0x68` — указатель на реестр, а `+0x6c` — флаг владения.

## Объект-менеджер режимов и реестр режимов (2026-11-06, FC3.dll 1.06, `local/scratch/probe53.py` .. `probe56.py`)

- **`[0x12804acc]` устанавливается в `0x117567ce mov dword ptr [0x12804acc], esi`** (`FC3.dll
  0x117567c0 .. 0x11756810`): основной vtable `0x11f16a3c` (`0x117567c4`), второй vtable `0x11f1bdc0`
  в `+4` (`0x117567d4`) и в `+0x30` (`0x117567ed`), вложенные объекты строятся в `+8`
  (`lea ecx,[esi+8]` + `call 0x106bdde0` в `0x117567cb/0x117567db`) и в `+0x34`
  (`lea ecx,[esi+0x34]` + `call 0x106bdde0` в `0x117567ea/0x117567f4`), поля `+0x24/+0x28/+0x2c` и
  `+0x50..+0x64` обнуляются (`0x117567e0..0x11756808`), третий вложенный объект в `+0x70`
  (`lea ecx,[esi+0x70]`, `0x1175680b`). Ссылка обнуляется дважды: `0x11756f37 mov dword ptr
  [0x12804acc], ebx` после `call 0x106bde10` (`0x11756f31`) и `0x10048100 mov dword ptr [0x12804acc],
  0` внутри удаляющего деструктора `0x100480f0` (`mov dword ptr [esi], 0x11f169dc` в `0x100480fa`).
- **API регистрации `0x1029eb00` получает карту `obj+0x30`**: `mov eax, dword ptr [ebp+8]` +
  `add eax, 0x30` + `call 0x1029eb00` (`0x117567ab..0x117567b1`). Регистрация launch-режимов
  `0x1119dec0` использует то же API с `add eax, 4`; разница смещений пока не объяснена.
- **`engine\gamemodes\GameModesConfig.xml` (`0x11f16940`) имеет ровно две ссылки, обе в одной функции
  `0x117568a0`**: `mov ecx, dword ptr [0x12804308]` (`0x117568a6`) → `mov eax,[ecx]` +
  `mov edx,[eax+0x10]` (`0x117568ae`) → `push 0x11f16940` (`0x117568b5`) → `call edx` (`0x117568ba`);
  ветка «не найдено» пишет `mov dword ptr [edi], 0` (`0x117568c0`), ветка «найдено» — `push 0;
  push 0x11f16940; lea eax,[esp+0xc]; push eax` (`0x117568cd..0x117568d8`).
- **У контейнера реестра `[0x128048c0]` один создатель и один освобождающий**: создаёт `0x10048230`
  (`mov dword ptr [eax], 0x11f224c4` в `0x10048271`, поля `+4..+0x18` = 0, публикация в `0x10048289`
  и `0x10048297`); освобождается в деструкторе менеджера `0x11756e00`: `mov ecx,
  dword ptr [0x128048c0]` (`0x11756e03`) → при nonzero `mov eax,[ecx]` + `mov edx,[eax]` + `push 1` +
  `call edx` (`0x11756e1a..0x11756e20`) → `mov dword ptr [0x128048c0], ebx` (`0x11756e25`);
  собственный указатель `+0x68` освобождается через `push eax; call 0x11701470`
  (`0x11756e34/0x11756e35`) только при установленном флаге `[esi+0x6c]`
  (`cmp byte ptr [esi+0x6c], bl` в `0x11756e2f`, сброс в `0x11756e3a`). Это подтверждает путь co-op
  проверок `+0x68` и что `+0x6c` — флаг, а не указатель.
- **Установщик режима `0x11756dc0 .. 0x11756dfc` (`ret 0x10`)**: `mov edx, dword ptr [ebp+0x10]`
  (id) + `mov esi, dword ptr [ebp+8]` (менеджер) + `lea eax,[esi+0x30]` + `call 0x118cea00`
  поиск по карте (`0x11756ddc/0x11756ddf`); при попадании `push esi; lea ecx,[esp+0x18];
  call 0x11756a50` (`0x11756de8..0x11756ded`); результат — `mov al, byte ptr [esp+0x13]` (`0x11756df2`).
- **В цепочке установки `0x1119fa50 .. 0x111a0a6b` три группы** (22 вызовов `call 0x11756a50` в
  `0x111a0870..0x111a0a6a`): группа A `call 0x103d2b20` для 11 id (`0x111a0880..0x111a0966`, co-op
  `0x1216aee4` в `0x111a0944`), группа B `call 0x103d2b90` для 5 id (`0x111a097d..0x111a09d9`:
  `0x1216aedc`, `0x1216af0c`, `0x1216aeec`, `0x1216aef4`, `0x1216aefc`), группа C `call 0x103d2c00`
  для 6 id (`0x111a09f0..0x111a0a63`, добавляет `0x1216ca9c`). Co-op встречается только в группе A.
- Сервисы групп: `0x103d2b20` кладёт имя класса `CFCXConsoleService` (`0x11fc5760`) и фабтуру режима
  `0x11a746b0`; `0x103d2b90` кладёт `CFCXMultiPlayerConsoleService` (`0x11fc5568`) и фабтуру
  `0x10fad580`; `0x103d2c00` — третий вариант (`mov al, bl; ret 8` в `0x103d2c63..0x103d2c6b`). Все
  три возвращают булев результат и вызываются только из этой цепочки.
- **`0x11756c50` (`ret 0x10`) отказывает в установке, если id отсутствует в `[0x128048c0]`**
  (поле записи `+0x20`, поиск `cmp dword ptr [ebx+0x20], edi` в `0x11756c82`, промах —
  `xor al,al; ret 0x10` в `0x11756ca5..0x11756cad`); при успехе проходит шлюзы
  `cmp byte ptr [0x128042fc],0` + `call 0x11789a00` (`0x11756cb0..0x11756cb9`) и
  `cmp byte ptr [0x12162beb],0` (`0x11756cc5`), затем выделяет память
  `mov edx,0x10; lea ecx,[edx+0x10]; call 0x1177f7c0`.
- **Регистрация режимов перезапускается драйвером останова/повторной инициализации `0x111f3b30`**
  с началом `mov edi,ecx; call 0x111f22c0; cmp byte ptr [edi+0xb3],0; je 0x111f3c52`: он вызывает
  `0x1119f920` (`0x111f429b`), `0x1119fa50` (`0x111f42a0`), `0x1119dfa0` (`0x111f42a5`) после цикла
  освобождения (`lock xadd dword ptr [eax], edx` + `mov eax,[ecx]` + `mov edx,[eax+4]` + `call edx`
  в `0x111f42dd..0x111f4327`, `mov edi, dword ptr [0x12804b88]` (`0x111f42eb`), `call 0x11938250`
  (`0x111f42f5`), `call 0x1039c5b0` (`0x111f4304`)).
- Маленький аксессор `0x111f3ae0 .. 0x111f3b2b`: `mov eax,[0x12804acc]` + `cmp byte ptr [eax+0x6c],0` +
  `mov eax,[eax+0x68]` + `push eax; call 0x1039c880` + `push eax; call 0x1116d760`, далее
  `[0x12805450]` → `0x1135e720`, `[0x1280564c]` → `0x111e1c00`, `[0x128055ec]` vtable слот `+0xc`,
  `[0x1280563c]` → `0x111e44d0`.
- У `0x111a0860` и `0x111a0a70` **нет** прямых вызывающих по E8/E9: `0x111a0860` достигается
  переходом внутри `0x1119fa50`; `0x111a0a70` — thunk (`push ecx; call 0x1119f920; call 0x1119fa50;` +
  `call 0x1119dfa0; pop ecx; ret` в `0x111a0a70..0x111a0a81`), и его внутренние sites вызова
  `0x111a0a71/0x111a0a76/0x111a0a7b` не являются внешными входами, поэтому вход в него идёт через
  таблицу переходов или ссылку в данных (открыто).
- Родственный конструктор режима `0x11a746b0 .. 0x11a74745` (используется `0x103d2b20`): те же шлюзы
  (`cmp byte ptr [0x128042fc],0` → `call 0x11789a00`; `cmp byte ptr [0x12162beb],0`), выделение
  `mov edx,0x10; lea ecx,[edx+0x40]` (0x50) `call 0x1177f7c0`, инициализация `[obj] = 0x11f169e8`,
  `[+0x1c] = 0xffffffff`, `[+0x48] = 7`, итоговый vtable `mov dword ptr [eax], 0x1200cf78`
  (`0x11a74732`).
- Открыто: кто вызывает `0x117567c0` / `0x117568a0`; что хранит карта `obj+0x30` (`0x118cea00`); кто
  пишет `[obj+0x68]` и устанавливает `[obj+0x6c]`; почему `0x1119dec0` использует `add eax,4`, а
  `0x117567ae` — `add eax,0x30`; потребители регистраторов колбеков `0x1068f590` / `0x11851c40` в
  `0x103d2c6e` / `0x103d2ce0`.


## Объект реестра: выделение, цепочка конструкторов и таблица из тринадцати записей (2026-11-06, FC3.dll 1.06, `local/scratch/probe73.py` .. `probe81.py`)

- `0x111f3b30` — **не** цель таблицы переходов: единственная ссылка на него — слот `+0x44` vtable
  `0x11fba838` (`0x11fba87c`). Сканы в форме RVA для `0x011f3ae0`, `0x011f3b30` и `0x0039c250` дают
  ноль совпадений, значит реестр достигается через vtable и цепочки конечных переходов, а не через
  таблицы переходов.
- `0x11fba838` — общая vtable тринадцати подузлов реестра: ровно тринадцать sites записи, все внутри
  строителя `0x1039c250 .. 0x1039c580`, со смещениями `0x20, 0xb0, 0x140, 0x1d0, 0x260, 0x2f0, 0x380,
  0x410, 0x4a0, 0x530, 0x5c0, 0x650, 0x6e0` (шаг `0x90`). Среди её слотов — скалярный удаляющий
  деструктор `0x1039c580` в `+0x00` и `+0xac`, а также серединонные записи `0x111f3d10` (`+0x24`) и
  `0x111f3b30` (`+0x44`).
- Конструктор каждого подузла — `0x100013c0`: `mov ecx,esi; call 0x106dfa60` +
  `[esi+0x10] = 0x10238ce0` + инициализация маски из глобальной `0x1217c698`
  (`mov eax,[0x1217c698]; mov [esi+4],eax`, далее цикл по битам маски с `shl ecx,cl` / `not eax` /
  `and` / `or` / `add ecx,2` / `shr eax,1`). `0x106dfa60` — базовая инициализация: `[esi+8] = -1`,
  `[esi+0xc] = 0`, `[esi+4] = 0`, затем `0x1217b420` + `call 0x106f6fb0`, `[eax+4] = 0`, `[ecx] = 0`,
  `mov [esi],eax`, `lock xadd dword ptr [ecx],1`.
- Адреса обработчиков, которые передаются в конструкторы тринадцати записей: `0x111f1070`,
  `0x111edbd0`, `0x111f22c0`, `0x111f26c0`, `0x111f22a0`, `0x111f26e0`, **`0x111f3ae0` для записи
  `+0x380`** (site `push` — `0x1039c3e4`), `0x111f20a0`, `0x111f0a90`, `0x111f2140`, `0x111f0a30`
  трижды. Запись `+0x380` — та, у которой обработчик является аксессором кооператива `0x111f3ae0`
  (`mov eax,[0x12804acc]` + `cmp byte ptr [eax+0x6c],0` + `mov eax,[eax+0x68]`).
- Конструктор менеджера `0x1039c030 .. 0x1039c250` (`this` в `esi`): `call 0x106db0d0`; четыре группы
  инициализации (`lea edi,[esi+0x15c]` дважды с шагом `0x24` → `0x106ce850`; `lea ebx,[esi+0x1a8]`
  дважды с шагом `0x48` → `0x106ce8b0`; `lea edi,[esi+0x280]` дважды с шагом `0x84` → `0x106d3ed0`);
  затем `lea edi,[esi+0x388]; call 0x1039c250` — таблица из тринадцати записей лежит на смещении
  менеджера `+0x388`, а её указатели опубликованы в `+0x128..0x158` как `&esi+0x3a8, 0x438, 0x4c8,
  0x558, 0x5e8, 0x678, 0x708, 0x798, 0x828, 0x8b8, 0x948, 0x9d8, 0xa68`. Конечные регистраторы:
  `push 0xd; push edi; mov ecx,esi; call 0x106bc890`, `push 2; lea eax,[esi+0x280]; call 0x106bc860`,
  `push 2; lea eax,[esi+0x15c]; call 0x106bc8b0`, `push 3; push ebx; call 0x106bc8e0`; финал
  `mov eax,esi; ret`. Пары встраиваемых колбеков `(0x111edac0, 0)`, `(0x1039cc20, …)`,
  `(0x1039b990, 0)` регистрируются через `call 0x106bc7b0` со `ecx` = `&esi+0x1f0` и `&esi+0x238`.
- Перепись ссылок по всем исполняемым секциям (`local/scratch/calls.json` хранит 721 985 rel32
  call/jmp ссылок, 178 197 различных целей): у `0x1039c250` один вызывающий (`0x1039c09c`), у
  `0x1039c030` один (`0x1039b719`), у `0x1039b710` один (`0x111f4fcf`), у `0x111f3ae0` один
  (`0x111f3c54`), а у финала `0x1039d020` тринадцать (`0x1039b877..0x1039b97c`).
- Site выделения (`0x111f4f9f..0x111f501d`): после `call 0x1039a420`, `call 0x1039ab60` и общей
  пусковой проверки `cmp byte ptr [0x128042fc],0` → `call 0x11789a00` (`0x111f4fa9`, `0x111f4fb2`)
  объект создаётся через **`mov eax,0x10; mov edi,0xaf0; call 0x118db560`** (`0x111f4fb7`) —
  **размер 0xaf0**, выравнивание 0x10 — и сохраняется как `mov esi,[esp+0x14]` …
  `mov dword ptr [esi+0xd4], eax` (`0x111f4fc6`, `0x111f4fd8`) после `push esi; call 0x1039b710`
  (`0x111f4fce..0x111f4fcf`).
- Записи используются сразу же: `mov ecx,[0x12807738]; push ecx; mov ecx,eax; call 0x106eac80`
  (`0x111f4fde..0x111f4fe7`), затем `mov eax,[esi+0xd4]; mov edx,[0x12807740]; mov ecx,[eax+0x12c];` +
  `mov ecx,[ecx+0x24]; push edx; call 0x106e96c0` (`0x111f4fec..0x111f5002`) и тот же шаблон для
  следующей записи через `[eax+0x130]` с глобальной `[0x12807744]` (`0x111f5007..0x111f501d`):
  опубликованный массив `+0x128..0x158` и есть таблица диспетчера, а поле записи `+0x24` передаётся
  как `this` в `0x106e96c0` с отдельной глобальной из `[0x12807738 + n*4]`.
- Исправления к прежним заметкам: утверждение «thunk `0x11010ee0` → `0x12163234`» неверно (байты
  `0x11010ee0` — это `0c 00`, а дальше padding `0xcc`). У `0x12162beb` 14 218 совпадений в сырых
  байтах, но это immediates вида `cmp eax,0x12162beb`, а не ссылки на память, поэтому реальная
  проверка остаётся `cmp byte ptr [0x12162beb],0` внутри `0x11756c50`.
- Замечание по инструментарию: сырые сканы должны использовать raw-указатель секции
  (`pe.d[ptr:ptr+rawsz]`); прежний `probe72.py` использовал виртуальное смещение как смещение в файле
  (`pe.d[sv:sv+vsz]`), из-за чего все сканы давали ноль. `FC3.exe` отсутствует в
  `H:\FarCryCoop\TestFC3\bin\` (есть только `FC3.dll`, `FC3_d3d11.dll`, `SandBar.dll`, `SandDock.dll`,
  `binkw32.dll`, `FC3Editor.*`, `FC3Updater.exe`); рабочие копии лежат в `E:\FarCryCoop\Work\fc3\FC3.exe`.
- Открыто: кто выделяет контейнер, в поле `+0xd4` которого попадает менеджер (`mov esi,[esp+0x14]` в
  `0x111f4fc6`); что именно делают `0x118db560`, `0x106eac80` и `0x106e96c0`; какая из тринадцати
  записей является кооперативной в ран-тайме (запись `+0x380` — единственная, у которой обработчик
  является аксессором кооператива); потребители `[0x12807738]`, `[0x12807740]`, `[0x12807744]`;
  является ли пара `[0x128042fc]` / `0x11789a00` проверкой стартовых аргументов, которую нужно обойти.
## Сессия агента (2026-11-06, RU): как 1.06 превращает switch в id режима и как ищет сессию по id

`local\scratch\probe101.py`..`probe110.py` (scratch, в репозиторий не попадает). Все адреса —
`FC3.dll` версии 1.06 (`H:\FarCryCoop\TestFC3\bin\FC3.dll`, sha256
`71d58a7a4503c5c67dcd6f4b9445b5e7ac34c12043987f4c7285e13f2ed51c9b`).

### Параметр опций запуска выбирает id режима

`0x11a8c440` — парсер сетевых опций запуска (пролог
`55 8bec 83ec2c 53 8b5d14 56 8b750c 57`, `[ebp+0xc]` = argv). Использует
`mov edi,[0x11bcb5c0]` (аналог `strcmp` из C-runtime) как `push <адрес строки>; push argv;
call edi` и пишет **константу id режима** в выходной параметр. Цепочка проверена дословно:
`-deathmatch` -> `mov eax,[0x1216aedc]` (`0x11a8c529`); **`-coop` (`push 0x11fb81fc` в
`0x11a8c533`) -> `mov ecx,[0x1216aee4]` (`0x11a8c542`) -> `mov [ebp+8],ecx` (`0x11a8c548`)**;
`-domination` -> `[0x1216aeec]`; `-headquarters` -> `[0x1216aef4]`; `-firestorm` -> `[0x1216aefc]`;
`-dash` -> `[0x1216af04]`; **если ни один switch не найден — `mov eax,[0x1216af0c]`
(`0x11a8c5b1`) -> `mov [ebp+8],eax` (`0x11a8c5b6`)**. Ранее в той же функции проверяются
`-login`, `-ip` (строка ошибки `Missing -login or -ip for -client.` в `0x11fb8258`), `-online`,
`-password`, `-bfname`, `-sessionuid`. Копия той же цепочки находится в `0x11a8c8b0` (`-host` в
`0x11a8c90b`, `-coop` в `0x11a8c95c` -> `mov [ebp+0xc],ecx`), и она же — единственный вызывающий
`0x11a8c440` (вынос в `0x11a8d000`); саму `0x11a8c8b0` вызывают из `0x11a8cf45` и `0x11a8d045`.
Значит `0x11a8c440` выполняется только при запуске с сетевыми аргументами: обычный запуск
одиночной игры к нему не доходит.

### Id режимов — 32-битные хеши, они лежат в .data рядом с `0x1216a00c`

Дамп `FC3.dll .data` (после каждого id стоит `0xffffffff`), проверено `probe106.py`:
`0x1216aedc = 0x8b40f692` (-deathmatch), `0x1216aee4 = 0x43521e6f` (**-coop**),
`0x1216aeec = 0xab4fc8e2` (-domination), `0x1216aef4 = 0x9991daec` (-headquarters),
`0x1216aefc = 0xf66ffb87` (-firestorm), `0x1216af04 = 0x2100ced7` (-dash),
`0x1216af0c = 0x6c2b3868` (**режим по умолчанию, когда switch не указан**),
`0x1216aed0/0x1216aed4 = 0x9ba82025`, `0x1216af14 = 0x1577a45d`, `0x1216af18 = 0xfe80eafc`,
`0x1216af1c = 0xa9fc2652`. Соседние слоты `0x1216aeb8`, `0x1216aeb0`, `0x1216aea8` хранят
указатели на строки (`World` `0x11f0f5c4`, `PersistenceDB` `0x11fbad38`, `CampaignSave`
`0x11fbad28`), то есть блок — реестр именованных сессий «имя + хеш», та же форма регистрации,
которая уже зафиксирована для world/session-записей.
### Где используется хеш кооператива (probe107-probe112)

- `0x1133fb80` — 50-байтная проверка (`0x1133fb80`..`0x1133fbb2`, проверена byte-by-byte в
  `local\scratch\p111.txt`): `mov eax,[eax]`, затем `cmp` с `0xab4fc8e2`, `0x43521e6f` (coop),
  `0x9991daec`, `0x6c2b3868`, `0xf66ffb87`, `0x2100ced7`, после каждого `je 0x1133fbaf`; путь
  отказа `xor al,al; ret` в `0x1133fbac`, путь успеха `mov al,1; ret` в `0x1133fbaf`. Отвечает на
  вопрос "is this id one of the network mode ids" и **`0x8b40f692` (-deathmatch) в этот список не
  входит**. `0x113403e0` повторяет те же шесть сравнений в `0x113405a4`, `0x11340623`, `0x11340a26`
  и читает реестр `[0x128048c0]` в `0x113408fa`, `0x11340bdd`. После `ret` у обеих функций идёт
  `int3`-выравнивание, а `0x11204b00` и `0x10fa1710` — крупные функции (`sub esp,0xa94` у
  `0x11204b00`), то есть это полноценная настройка режима по сессии, а не один switch.
- `0x11a7f9d0` запрашивает у аксессора сессию по id: `mov ecx,[0x12804acc]; mov eax,[ecx];
  mov eax,[eax+4]; lea edx,[esp+0x10]; mov [esp+0x14],0x80608f71; push edx; call eax`, а если это
  вернёт false — `mov ecx,[0x12804acc]; mov dword ptr [esp+0xc],0x43521e6f; mov edx,[ecx];
  mov edx,[edx+4]; lea eax,[esp+0xc]; push eax; call edx`, то есть **виртуальный слот +4 объекта
  `[0x12804acc]` — это "find/lookup session by hash id"**, сначала с `0x80608f71` (ещё одно
  зарегистрированное имя, значение пока не атрибутировано), затем с хешем кооператива. Если любой
  поиск удался, код продолжает загрузку `engine\toolsconfig\AlwaysLoaded.xml`
  (`push 0x11f2fd78` в `0x11a7fa2b`). Это место, где игра может запросить coop-сессию вообще без
  аргментов запуска.
- `0x10fa1710` — это не plain mode switch, а **цепочка "имя сессии -> имя загрузочного экрана"**
  (`local\scratch\p112.txt`): имена проверяются относительно `[ebp+0xc]` через `call 0x1011cb30`,
  сопоставляя `'coop_vidar'` (`0x1200e400`) -> `'COOP_Vidar_Loading'` (`0x1200e40c`), `'coop_sleipnir'`
  (`0x1200e420`) -> `'COOP_Sleipner_Loading'` (`0x1200e430`), `'mpdlc1_aegir'` (`0x11f8a1b4`) ->
  `'COOP_Freya_Loading'` (`0x1200e3cc`), `'mpdlc1_idun'` (`0x1200e448`) -> `'Coop_Fenrir_Loading'`
  (`0x1200e38c`), все эти ветки прыгают в общий хвост `0x10fa1c22`. И только после проверок имён
  сравниваются хеши: `0x10fa1bb9` `0xab4fc8e2` (-> `push 0x1200e454 'PvP_Dom_Loading'`,
  `call 0x10a00be0`, `push 0x1200e454`, `call 0x10001ed0`), `0x10fa1bde` `0x9991daec` (->
  `'PvP_Pump_Loading'` `0x1200e464`), `0x10fa1bec` `0x8b40f692` (-> `'PvP_DeathMatch_Loading'`
  `0x1200e478`), `0x10fa1bfa` `0x6c2b3868` (-> `'PvP_TeamDeath_Loading'` `0x1200e490`), `0x10fa1c08`
  `0xf66ffb87` (-> `'PvP_FireStorm_Loading'` `0x1200e4a8`), `0x10fa1c16` `0x2100ced7` (->
  `'COOP_Dash_Loading'` `0x1200e4c0`); режим dash обрабатывается в coop-семействе (у него имя с
  префиксом `COOP_`). Цепочка заканчивается в `0x10fa1c2b`: `push edx; push ebx; call 0x110252d0`,
  затем `mov eax,[0x12804b3c]; cmp dword ptr [eax+0x70],1; jne 0x10fa1ca0` и второй блок сравнения
  id (`0x10fa1c45`..`0x10fa1c66`, где domination / headquarters / deathmatch / default / firestorm
  ведут в `0x10fa1c68`), а там `mov esi,[esp+0x14]; mov edx,[esi+0xa0]; mov eax,[edx+0x64];
  lea ecx,[esi+0xa0]; call eax` — виртуальный вызов по смещению объекта `+0xa0`, слот `+0x64`. Перед
  сравнениями хешей `0x10fa1710` использует `-online` (`push 0x11fb8234`) как строковую проверку.
- `0x11204b00` — второй диспетчер режимов: читает `[esi+0x10]` и разветвляет `0` -> `0x1120590d`,
  `1` -> fall-through, иначе -> `0x11205d99`, и сравнивает id в `0x11205732`, `0x11205753`,
  `0x11205775`, `0x11205793`, `0x112057b1`. Обе функции (`0x10fa1710` и `0x11204b00`) начинаются с
  одного и того же аксессора (`mov eax,[0x12804acc]; cmp byte ptr [eax+0x6c],0; mov eax,[eax+0x68]`).
- `0x11341dc0` (31 rel32-вызовов, например `0x10ce93c9`, `0x10edd676`, `0x10f3a0f9`, `0x10fcc522`) —
  общий getter режима/сессии: `mov eax,[edi]; mov edx,[eax+0x10]; mov ecx,edi; call edx;
  mov ecx,edi; call 0x10056690; push eax; call 0x11774ae0; mov [ebx],esi` — принимает указатель на
  id режима в `[ebp+8]` и возвращает объект (после чего вызывает его виртуальный слот +4 с `this`).
- `0x11a7f820` начинается с `cmp byte ptr [0x12804a09],0` и обходит вектор
  `[0x12820c04]`/`[0x12820c08]` (`lea edi,[ecx+eax*4]`), и `0x11a7fa12` содержит хеш
  кооператива; это ближайший проверенный цикл "обойти зарегистрированные сессии и выбрать coop".
- Отрицательные результаты: у `0x113403e0`, `0x10fa1710`, `0x11204b00`, `0x10fad5e0` и `0x10f0c770`
  нет ни одного rel32-вызывающего site — они вызываются косвенно (таблица функций или таблица
  импорта). `0x11997e50` содержит `0x8b40f692` (`0x119980f8`) и `0x6c2b3868` (`0x1199811a`) и
  вызывается ровно один раз (`0x1199837a` внутри `0x119982e0`); там нет ни `0x43521e6f`, ни
  `0x80608f71`, поэтому это branch одиночной сессии, а не кооп-ветка. Ни один из этих id не появляется
  в `engine\mp\game_mode_coop.lua` (строка `'coop'` не является mode id).
- Ловушка `cstr`-в-середине-строки: `0x11fb8218` — не начало имени switch (`0x11fb8214` — это
  `0x11fb8210 '-online'`, поэтому `0x11fb8218` читается как `rters` = хвост `...multiplayers`);
  `0x11fb8228` — тоже не строка. Из этой области реальна только `-online` (`0x11fb8234`).

### Что из этого следует для мода

Coop-режим — это не флаг, который ASI переключает в одном месте: игра разрешает **hash id** через
объект `[0x12804acc]` (виртуальный слот +4) и реестр `[0x128048c0]`, а хеш кооператива `0x43521e6f`
зашит в исполняемый файл и в таблицу данных. Самые компактные одиночные точки из найденного:
заставить ветку "switch не указан" в `0x11a8c5b1` загрузить хеш кооператива вместо `0x6c2b3868`, либо
вызвать find-by-id у аксессора прямо с `0x43521e6f` (этот шаблон уже есть в `0x11a7fa08`). Ничто из
этого не требует аргментов запуска: `0x11a7f9d0` показывает, что игра способна запросить coop-сессию
у аксессора в любой момент. Имена coop-сессий, которые использует сама игра (`coop_vidar`,
`coop_sleipnir`, `mpdlc1_aegir`, `mpdlc1_idun` и их пары `*_Loading`) — второй, основанный на именах,
путь к тем же coop-сессиям. `0x11341dc0` — тот getter, который возвращает объект сессии по указателю
на id режима.

- Script-fragile детали уже записаны в других разделах: выборщик switch->mode id `0x11a8c440` /
  `0x11a8c8b0`, таблица хешей в `FC3.dll .data` по `0x1216aedc`..`0x1216af1c`, зеркало в
  `FC3_d3d11.dll` (`0x1218ffe4`).
- Следующие шаги: разрешить, чем является виртуальный слот +4 объекта `[0x12804acc]` (REA
  pseudocode), разрешить объект по `[0x12804b3c]` со смещением `+0x70` и вызов `+0xa0`/слот `+0x64`
  в `0x10fa1710`, перечислить зарегистрированные строки имён рядом с хешами по `0x1216aeb8` /
  `0x1216aeb0` / `0x1216aea8` и выяснить, какая из них регистрируется одиночным запуском; затем
  динамический эксперимент, который запросит у аксессора coop-сессию (`0x43521e6f`) в запущенном
  процессе 1.06.
### Функции режимов запуска — виртуальные слоты, а не case в switch (probe132-probe137)

У `0x113403e0`, `0x10fa1710`, `0x11204b00`, `0x10fad5e0`, `0x10f0c770` нет вызывающего `E8`,
потому что они являются записями vftable классов в `.rdata`. `xlib.verified_xrefs` по началу
таблицы даёт 0 попаданий; из кода ссылаются только адреса слотов.

Сквозный поиск dword по всему образу (`local/scratch/p131.py`, `.text` + `.rdata` + `.data`) —
 пять владельцев и индекс слота каждой цели:

- `0x11f910c0` -> слот 5 `0x11341340`, слот 8 `0x1133fb30`, слот 12 `0x113403e0`,
  слот 13 `0x1133fe20`, слот 20 `0x1133fbd0`, слот 22 `0x11340d90`. Слот 0 `0x1030f8c0`,
  слот 1 `0x10007480`, слоты 16-19 и 21 заполнены заглушкой `0x1061fee0`.
  Устанавливает её `0x11341070` (там же `mov dword ptr [ebx+0x3c],0x11f35e98`), которая ищет
  сериализуемое имя `0x11f75a78` = `'CGameLoadingScreen'` (`push 0x11f75a78; call 0x10a00be0`,
  затем `push 0x11f75a78; call 0x10001ed0`) и делает пару `lock xadd` на счётчик ссылкой по
  `[ebx+0x3c]`/`[ebx+0x40]`.
- `0x11f91128` (устанавливается в `[esi+8]` в `0x11340ec0`, там же `mov dword
  ptr [esi+0x3c],0x11f91130`) -> слот 0 `0x1030fbf0`, слот 1 `0x1030f850`, слот 2 `0x1030fc00`,
  дальше purecall-заглушки `0x1008a7e0` / `0x1008a830` / `0x10089f80` и маленький класс
  `0x1165c760`..`0x1165c960`.
- `0x1200e608` -> слот 4 `0x1133fb30`, слот 8 `0x113403e0`, слот 9 `0x1133fe20`,
  **слот 11 `0x10fa1710`**, слот 1 `0x10fa1f30`, слоты 12-18 `0x10fa13b0 0x10fa1200
  0x10fa12e0 0x10fa11c0 0x10fa1160 0x10fa1530 0x10fa1d10`, слот 20 `0x11340560`, слот 21
  `0x10fa1170`, слоты 22-23 `0x104df010` / `0x104de9d0`. Слоты 0-10 общие с `0x11f910c0`
  (`0x10c478a0`, далее `0x10390f00 0x1005bf70 0x1133fb30 0x10c471a0 0x10c47bf0 0x10c47600
  0x113403e0 0x1133fe20 0x10c471b0`), расходятся только с 11-го слота, то есть это соседний
  класс той же иерархии — специфичный для режима вариант, несущий весь кластер методов
  `0x10fa1xxx` (набор обработчиков режима запуска).
- `0x11fb5914` (внутренность семейства vftable, заголовок которого `0x11fb5844`,
  устанавливается в `[esi]` в `0x112046a0`/`0x11204a00`) -> слот 0 `0x10390fd0`, слот 2
  `0x112073d0`, слот 3 `0x112029e0`, **слот 8 `0x11204b00`**, слот 9 `0x10391480`, слот 20
  `0x11204980`, слот 17 `0x11202bd0`. `0x11204a00` инициализирует объект: `[esi+4]=0`,
  `[esi+8]=3`, `[esi+0xc]=0`, `[esi+0x14]=0`, `[esi+0x18]=-1`, `[esi+0x1c]=-1`, `[esi+0x20]=0`,
  `[esi+0x24]=0`, заголовок самолинкующегося списка по `[esi+0x28]` (`mov [eax],eax;
  mov [eax+4],eax; mov [eax+8],0`), `push 0x400; lea ecx,[esi+0x3c]`.
- `0x1200ac6c` -> **слот 8 `0x10f0c770`**, слот 1 `0x10fcad00`, слот 7 `0x10507f50`, слот 9
  `0x10f0cbe0`, слот 10 `0x10f0baf0`, слот 12 `0x10f0c0b0`, слот 13 `0x10f0cb60`, слот 15
  `0x104cedd0`, слот 16 `0x104cedc0`, слот 18 `0x11a58e90`. Слоты 2-6, 17 и 20 заполнены
  заглушкой `0x104833e0`.
- `0x1200d820` -> **слот 8 `0x10fad5e0`**, слот 5 `0x116fb950`, слот 6 `0x10fb0570`, слот 7
  `0x10fb0ee0`, слот 9 `0x104da2a0`, слот 10 `0x10faf980`, слот 11 `0x10fad310`, слоты 12-14
  `0x10fae850 0x10fae7d0 0x10fae740`, слот 17 `0x10faf910`, слот 18 `0x10fad340`, слот 19
  `0x10fb0510`, слоты 20-21 `0x10fad330` / `0x10fad320`, слот 23 `0x104da280`.
- Исправление: `0x11f902b0`, `0x12009e48`, `0x1200c9f0`, `0x1200d7f8`, `0x11fb4a34` — это НЕ
  таблицы: `0x11f902b0` — это строка имени класса `'CGOBuilderNode'`, а `0x12009e48` — середина
  строки (`'lities'`). И `0x11f0ebec` (пишется в `0x1030d7b6 mov dword ptr [edi],0x11f0ebec`)
  тоже не vftable: это описательная область узла Builder, хвостая часть которой читается как
  `'CVideoBuilderObject'`.
- `0x11f5ad34` (73 dword-попаданий, 65 из них в `.text`, 56 разных функций, например
  `0x10048c80`, `0x10117620`, `0x111f5fe0`, `0x112046a0`, `0x11783510`) — широко общий базовый
  запись, но дамп её вывода залезает в ASCII (`'NONE'`, `'Start'`, `'Stop'`, `'Form'`, `'Spot'`,
  `'Soun'`), поэтому это запись рефлексии/сериализуемого имени, а не обычный vftable.
- Ссылки на слоты, найденные в коде (`local/scratch/p133.py`): `0x11f910c0+0x58` (слот 22) из
  `0x11340ed4` (внутри `0x11340ec0`); `0x11fb5914` слот 0 из `0x10390fdc` (`0x10390f60`), слот
  18 из `0x11204a50` (`0x11204a00`), слот 36 из `0x10fc3fd8` (`0x10fc3f30`), слот 63 из
  `0x112027d8` (`0x112027a0`); `0x1200ac6c` слот 11 из `0x10fcac1c` (`0x10fcab70`), слот 15 из
  `0x104cedd8` (`0x104cedd0`) и `0x11a58df4` (`0x11a58d70`), слот 59 из `0x11a585bc`
  (`0x11a585a0`); `0x1200d820` слот 17 из `0x10fad4b0` и слот 21 из `0x10fad54c`, оба внутри
  `0x10fad4a0`. У `0x1200e608` (той vftable, где лежит co-op) ни начала, ни одного слота из
  кода не ссылкается, значит объект создаётся конструктором, который сохраняет адрес через
  `mov [reg+disp],imm32` с регистром.
- Следствие: чтобы статически до­йти до обработчика co-op режима запуска, надо найти код,
  который сохраняет `0x1200e608` в поле объекта (форма `c7`: `mov dword ptr [reg+disp],
  0x1200e608`), а не вызов `0x10fa1710`. Гипотеза, ещё не проверена: объект co-op режима
  создаётся фабрикой, выбираемой из реестра id режимов, а не явным названным конструктором.
- `0x11789a00` — единственный найденный писатель `[0x128042fc]`: это самоохраняемая
  инициализация — `cmp byte ptr [0x128042fc],0; jne 0x11789b63`, она сохраняет три обработчика
  `0x11789840`, `0x117899e0`, `0x11789990` в таблицу по `0x121a7eb0`, вызывает функциональный
  указатель из `[0x1217bc8c]`, делает `mov dword ptr [0x120931c8],0x121a7eb0`, затем `0x11789a45
  mov byte ptr [0x128042fc],1`, и OR-ит `[0x1288c800]`. У `[0x12162beb]` есть чистый геттер в
  `0x1177eba0` (`mov al,byte ptr [0x12162beb]; ret`), а единственная запись байта — в
  `0x11774c4a` внутри `0x11774b70`, которая обходит список строк (`push 0x11f15148;
  call 0x10a00be0` -> `0x10004080`, затем `0x11f1514c`) и читает `[obj+0x14]` и `[obj+0x20]`;
  саму запись пока достичь не удалось.

- Открытые цели (без изменений): найти, кто пишет в `[0x12162beb]` / `[0x128042fc]`; закрыть
  остальные открытые пункты NOTES (владелец контейнера, принимающего менеджер в `0x111f4fc6`,
  `0x118db560`, `0x106eac80`, `0x106e96c0`, coop-запись среди тринадцати, потребители
  `[0x12807738]`/`[0x12807740]`/`[0x12807744]`); двуязычные NOTES + commit на каждое новое наблюдение.
