# Эмуляция Ubisoft co-op / RendezVous — разведка (FC3 1.05/1.06, PRUDP v1)

Цель: собрать известные open-source проекты, затрагивающие онлайн-стек Ubisoft / Quazal, оценить,
сколько из каждого переиспользуется для co-op кампании Far Cry 3 на этом билде (бинарь эпохи 1.05
`FC3_d3d11.dll`, md5 46bcc097, PRUDP v1, Quazal RendezVous), и дать оценку объёма/риска эмуляции
недостающего co-op-бэкенда как отдельного проекта. Все находки — только чтение / веб.

## 1. Что нужно FC3 (цель)

Из собственного статического + живого анализа репо (NOTES 28cw, 28dm-28dq):
- Co-op-сессии — это **Quazal RendezVous**, статически слинкованный в `FC3_d3d11.dll`, говорящий с
  Ubisoft через собственный PRUDP-стек сокетов (НЕ через API `uplay_r1_loader`).
- Каскад create/join: наличие менеджера онлайн-сессий -> `CFCXGatorLoginCheckOperation` ->
  `CRendezVousLoginOperation` -> `CRendezVousConfigureUPnPOperation` / `CRendezVousInitPunchOperation`
  -> `CSessionCreateOperation_RdV` -> хост-обработчик (`SubAccountClient`), причём **SessionTicket**
  выпускает сервис (RendezVous `JobTicketManager*`, `GameSessionKey`).
- Офлайн FC3 вообще не набирает RendezVous: висит «подключение к серверам», ожидая подлинного
  состояния *online-service connected + party/session*, которого офлайн-загрузчик дать не может.

Значит недостающее звено — не транспорт (PRUDP v1 мы уже понимаем), а **прикладной слой сервиса
RendezVous** для версии FC3: auth login, party/`Uparty`, matchmaking, GameSession create/join,
JobTicket (`AcquireTicket`/`ValidateKey`), вывод `GameSessionKey`, UPnP/NAT — как RMC/RendezVous
сообщения.

## 2. Транспорт — решённая общая задача

- **PRUDP** задокументирован и реализован независимо от Ubisoft:
  - wiki kinnay/NintendoClients «PRUDP Protocol» и 3dbrew «PRUDP» — полное фреймирование V0/V1:
    magic `0xEA 0xD0`, заголовок, сигнатура пакета, sequence id, типы SYN/CONNECT/DATA/DISCONNECT/PING,
    опции, согласуемые в SYN/CONNECT, кодировка опций RC4/AES.
  - AnthomStahl `prudp` (Python, на PyPI) — рабочая клиент/сервер-библиотека PRUDP v1/v2
    (извлечена из реверса Steam).
- FC3 использует тот же PRUDP (magic `EA D0` и handshake SYN/CONNECT идентичны тому, что уже
  реализует и тестирует наш `scripts/rv_server/prudp.py`). Вывод: **транспортный слой локального
  RV-эмулятора — низкорисковый и по большей части готов** (наш codec + handshake работают).

## 3. Реестр open-source и применимость к FC3

| Проект | Что эмулирует | Релевантность FC3 | Примечание |
|---|---|---|---|
| **Detanup01/UplayServer** (архив) -> **ServerEmus / Release.Uplay** (C#) | Uplay **Demux + API** (лаунчер), auth, DRM, ачивки, стор; DLL-шимы через именованные каналы как настоящий Uplay; правленые UplayKit/UbiServices/CoreLib | Средняя, но для слоя *boot/login/DRM* («Условие A»), а НЕ co-op-сессий | Ближайший эмулятор лаунчера/auth; доказывает, что boot-auth Uplay, который мы фейкаем на клиенте, можно ещё и отдавать с сервера. RendezVous party/lobby не делает. |
| **michal-kapala/ubi-gs** (Python) | Ubisoft **Game Service (GS)**, ~2000-2005 (`gsconnect.ubisoft.com`): auth, друзья, matchmaking, чат, валидация CD-ключей | Низкая: другой, более старый SDK — FC3 на Quazal RendezVous, не на GS | Полезно как архитектурный прецедент эмуляции сервиса «auth + друзья + matchmaking». |
| **Современная сцена эмуляторов Ubisoft online** — офлайн-эмулятор The Crew 1, форки PlayBlock / OpenCrew / SwallowBlock, Wild-Ones, инструмент для Ghost Recon/Breakpoint | **НОВЫЙ** стек Ubisoft Online (игры Anvil): login/UbiAuth, **party / Uparty**, сессии/matchmaking, PRUDP v4+ | Средняя (вдохновение архитектурой), но НЕ переиспользуется напрямую: FC3 — это Quazal RendezVous 6 / PRUDP v1, другое поколение протокола | Доказывает, что полный Ubisoft party/lobby/session-бэкенд СООБЩЕСТВО способно эмулировать, и публикует реальные потоки сообщений для современного стека. |
| **PretendoNetwork/nex-go** (Go), **pyprudp** (Python) | Сервер Nintendo **NEX** поверх PRUDP (транспорт + слой сервисов NEX RMI/RMC) | Низкая-средняя: тот же PRUDP-транспорт переиспользуется; схема сервисов NEX отличается от Ubisoft RendezVous | Хороший референс структуры PRUDP-сервера; но слой сервисов переносить не стоит. |

Итог: **ни один open-source проект не эмулирует конкретный Quazal RendezVous co-op-сервис FC3.**
Транспорт переиспользуется; для слоя лаунчера/auth есть крепкий референс (UplayServer); эмуляция
party/lobby-сессии существует только для *нового* стека Ubisoft, не для поколения FC3.

## 4. Оценка объёма / риска (как отдельный проект)

Пакеты работ для локального co-op-бэкенда FC3:
1. **Транспорт PRUDP v1 + RV handshake** — УЖЕ СДЕЛАНО здесь (codec + SYN/CONNECT + RMC-фрейминг).
   Объём: малый; риск: низкий.
2. **Щель Config / SandboxUrl** (отдавать OnlineConfig с `SandboxUrl` -> наш сервер; DNS-редирект) —
   задействовано здесь. Объём: малый; риск: низкий-средний.
3. **Довести FC3-клиент до реального набора RV** — нынешний твёрдый блокер (FC3 офлайн до RV-connect
   не доходит). Нужен либо клиентский патч, пропускающий ожидание состояния бэкенда, либо подача
   ровно тех событий, которые FCX опрашивает. Объём: средний; риск: ВЫСОКИЙ (много прошлых
   попыток провалилось).
4. **Прикладной слой сервиса RendezVous для FC3** — реализовать конкретные классы/методы RMC-сервисов,
   которые вызывает FCX (login backend-сервисов, JobTicketManager login/derive/validate, party,
   GameSession create/join, UPnP/NAT config) и их маршаллинг, А ТАКЖЕ крипто `GameSessionKey`/ticket.
   Объём: БОЛЬШОЙ (реверсить статически слинкованный Quazal, восстанавливать семантику); риск: НАИВЫСШИЙ.
5. **Синхронизация сессии/party + co-op в мире** — классическая неткод-поверхность после образования
   сессии. Объём: большой; риск: средний-высокий.

Убийственный риск (отдельно подчеркнуть): **нет эталонного трафика.** Co-op-сессию, работавшую против
Ubisoft, можно было снять только пока сервис был жив; теперь он мёртв. Без рабочей записи пакет 4
придётся восстанавливать чисто по статической дизассембле без оракула для сверки — а ошибки в
крипто/RMC-маршаллинге молчаливы и крашут. Этот единственный факт доминирует над стоимостью и
неопределённостью.

Реалистичный размер: это **многомесячный проект уровня команды** (реверс + эмуляция), а не правка
хука/прокси. Один человек (или агент) правдоподобно закрывает пакеты 1-2 (уже есть) и, возможно, 3
при удаче, но пакет 4 без эталонной записи — это тот класс задач, который исторически требует
сообщество из нескольких людей, разделяющих снятый трафик.

## 5. Рекомендация

- Держать mod-262 SOLO как выпускаемый результат (единственный рабочий FC3 co-op).
- Считать «эмуляцию co-op-бэкенда FC3» отдельным высокозатратным/высокорисковым проектом; не
  начинать без либо (a) снятой живой co-op-сессии FC3 до выключения сервиса, либо (b) команды,
  пулингующей такой трафик.
- Если когда-нибудь браться, переиспользуемые активы: наш PRUDP codec/rv_server/config_server,
  референсы PRUDP от kinnay и AnthomStahl, эмулятор лаунчера/auth Detanup01/ServerEmus, и сцена
  современных Ubisoft-online-эмуляторов ради архитектуры party/lobby.

## 6. Источники (прочитаны/изучены)

- wiki kinnay/NintendoClients, «PRUDP Protocol» (github.com/kinnay/NintendoClients)
- 3dbrew, «PRUDP»
- Python-библиотека AnthomStahl `prudp` (PyPI / github) — PRUDP v1/v2
- Detanup01/UplayServer (архив) и орг ServerEmus (Release.Uplay)
- michal-kapala/ubi-gs (эмулятор Ubisoft Game Service)
- PretendoNetwork/nex-go; pyprudp (Nintendo NEX поверх PRUDP)
- сцена современных эмуляторов Ubisoft online (офлайн The Crew 1; PlayBlock / OpenCrew / SwallowBlock /
  инструмент Wild-Ones для Ghost Recon Wildlands / Breakpoint)
- запись хаба coop-anything `games/far-cry-3` + issue «Far Cry 3 Co-op Support»
