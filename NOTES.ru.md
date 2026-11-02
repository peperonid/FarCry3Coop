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
  прежней заметки тоже относилась не к этому билду и её нужно перемерить по 48-байтным записям.

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
