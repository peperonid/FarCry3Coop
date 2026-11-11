# Офлайн-кооперация кампании: путь A (эмуляция сервиса) против пути B (патч клиента)

Статус: решение зависит от одного динамического факта. Источники: NOTES 28r-28y (источник истины), upstream
`Bantila/FarCry3Coop` NOTES, PR #1. Целевая сборка: Uplay 1.06, рабочий модуль `FC3_d3d11.dll` md5 `46bcc097` (x86, base `0x10000000`).

## Установленные факты
- HOST/JOIN через **меню** кампании жёстко привязан к онлайн-стеку: диспетчер `FUN_10441fb6` -> `FUN_1044123b`/`FUN_10443663`
  -> гейт RendezVous `FUN_11279ba0` -> RV `BackEndServices` сессия (`FUN_112536a0`). Ветки LAN в меню-пути нет (28t/28v).
- При этом в движке живёт слой **`storm::echo` LAN/loopback**: зарегистрированы `lan::inEvent::DiscoverSession` /
  `SetDiscoveryHandler` / `BroadcastLanDiscovery2`, тунables `LanMatchmakingDiscoveryOnLoopbackEnable`/`ReuseAddress`
  записываемы в рантайме, и опсы сессий `CFCXCreate/JoinSessionOperation`, `CSessionSearchIPOperation`. Параметры входа
  (`NetEngineType` id `0x3c`, `SessionType`, `SearchIdAddress`, `JoinSessionId`, `MatchMakingEnabled`) — **задаваемые данные** (28w).
- **`SessionTicket` / `GameSessionKey` выпускает СЕРВИС (RendezVous `JobTicketManager` + `TicketGrantingProtocol`), не хост** (28x).
  Функции, релевантные патчу (28y): PrepareCall `FUN_1124e5e0`, ProcessResponse `FUN_1124d660` (инжект тикета
  `**(param_1+0x78)=*(local_c+0x10)`), ValidateKey `FUN_1124d7b0` (гейт `DAT_1174d35a`), DeriveKey `FUN_1124e670`;
  крипто диспетчеризуется через **vtable-бэкенд `PTR_PTR_117531f4`** (подключаемый).

## Единственный факт, определяющий путь
**Требует ли путь LAN/loopback-джойна `storm::echo` выпускаемого сервисом `SessionTicket`, или LAN-доверие его обходит?**
- Если LAN-джойн **свободен от тикета** -> самый дешёвый путь: шим установления сессии (вести `NetEngineType=LAN` +
  `CFCXCreate/JoinSessionOperation`/`SearchIP`, включить `LanMatchmakingDiscoveryOnLoopbackEnable`). Тикетная работа не нужна.
- Если LAN-джойн **требует тикет** -> ниже пути A или B.
Решающий тест: `scripts/fc3_lan_coop_launch.bat` (`-coop`-хост + `-client -ip`, loopback-тунable включён), читать на любую
ошибку `*TICKET*`/`INVALIDSESSIONKEY`. (Живой прогон — на стороне владельца; Frida-attach блокирует анти-инъекция игры.)

## Путь A — эмуляция онлайн-сервиса Ubisoft
Поднять локальный сервис, отвечающий на Rendezvous/NetPlay-джобы клиента: `JobTicketManagerAcquireTicket`
(TicketGrantingProtocol), login (`JobTicketManagerLogin` DeriveKey/ValidateKey), `SandboxUrl`/OnlineConfig, GameSessionKey.
- Трудозатраты: **большие, многопротокольные** (PRUDP-транспорт со своим крипто-фреймингом, подпись тикетов, семантика сессионного сервера).
- Риск: высокий (много неизвестного в протоколе; upstream отметил это как тяжёлый вариант).
- Плюс: без патча игрового кода; сервер-приложение, которое игрок запускает сам.

## Путь B — узкий патч клиента (основной кандидат, если LAN не свободен от тикета)
Заставить клиента удовлетворять себя без сервиса:
1. Не дать `PrepareCall` (`FUN_1124e5e0`) уходить в сеть; заполнить out-буфер тикета в `ProcessResponse` (`FUN_1124d660`)
   локально (из id сессии / общего пароля).
2. Заставить `ValidateKey` (`FUN_1124d7b0`) / `DeriveKey` (`FUN_1124e670`) принять его: нейтрализовать крипто-проверку
   `FUN_112547c0` либо подменить бэкенд `PTR_PTR_117531f4` на детерминированный локальный.
- **Почему работает для кооперации среди своих:** крипто тикета должна лишь СОВПАСТЬ между пирами (оба выводят один
  `GameSessionKey`), а не быть подлинной Ubisoft. Все игроки ставят один мод, поэтому ключ совпадёт у всех.
- Трудозатраты: **малые, локализованные** (4 функции + 1 vtable-бэкенд), всё в рабочем модуле.
- Риск: средний (надо покрыть все пути тикета/ключа; мод должен стоять у каждого пира).
- Следующая копка: снять vtable-бэкенд `PTR_PTR_117531f4`, чтобы выбрать точную подмену / единственную check'у для нейтрализации.

## Рекомендация
1. Сначала прогнать LAN-эксперимент (возможно, тикетная работа вообще не понадобится).
2. Если на LAN тикет нужен, предпочесть **путь B** (патч клиента), и откатиться к пути A только если B окажется слишком широким.
3. Работу держать на ведущем upstream 1.05-пути; адреса кросс-картировать через `scripts/fc3_addrmap.py`.
