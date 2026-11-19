# FC3 co-op — LAN / Local-Session two-instance test checklist
EN / RU below. Goal: empirically verify whether two mod-262 instances can HOST and JOIN
a Local/LAN co-op session over a virtual LAN. This is the decisive step for 2+ human
co-op after the Ubisoft online-server shutdown (see docs/project-status.md).

## Why this test (short)
- Static RE (NOTES 28i11–28i13) proved FC3 ships a complete LAN/Storm co-op path:
  runtime connection-mode byte `mgr+0xbc` (1 Offline / 2 Online / 3 Lan-Local / 4 Split / 5 Single),
  `storm::echo` master/replica protocol, and LAN broadcast discovery on **UDP 9004 / 9005**
  (`LanHostBroadcastPort` / `LanClientBroadcastPort` in `GamerProfile.xml`).
- The mod-262 build already unlocks STARTING a "Local Session" co-op (per its READ.ME).
- The RV/online server is dead (2024-03-06) but that does NOT affect LAN broadcast discovery.
- The one unproven thing: can a second instance JOIN a hosted Local/LAN session.

## Prerequisites
- Two Far Cry 3 installs with the mod-262 recipe applied
  (clean 1.06 → Far CRy3 FIX Co-oP → The Dependencies-262 → patch_solo_offline_coop_vanilla-262).
  Working copy `H:\FarCryCoop\TestFC3_105` is instance A. Instance B is a second copy
  (agent can clone it on request — it is a large copy, needs owner OK).
- A virtual LAN both instances join: **Radmin VPN** (community-proven for FC3) or Hamachi.
  Both PCs (or both instances on one PC) get IPs on the same Radmin network.
- Firewall: allow the game both directions on UDP **9000-9005**.

## Procedure
1. Start Radmin VPN; both instances' hosts are on one network (e.g. 26.x.x.x).
2. Confirm the virtual-LAN MTU is not fragmenting; keep default.
3. Instance A: launch (mod-262), reach main menu → Multiplayer → **Co-op → Local Session → Host**.
   - Pick "Local/LAN" not "Online". Create the session. Note it appears in the LAN list.
4. Instance B: launch (mod-262) → Co-op → Local Session → **Browse / Join**. The A session
   should appear via LAN broadcast (ports 9004/9005).
5. If B sees and joins A → SUCCESS: 2-player co-op works offline. Then test 3rd/4th slots.
6. Capture the session logs for both instances (coop_shim_last.log if a shim is deployed;
   otherwise the in-game behavior + a screenshot).

## If B does NOT see A
- Both on the same virtual LAN? Same subnet, firewall off for UDP 9004/9005.
- LAN discovery on loopback (one PC, two instances): FC3 has tunables
  `LanMatchmakingDiscoveryOnLoopbackEnable` and `LanMatchmakingReuseAddress`
  (option registry FUN_107a227d). If a config/ini can set them, enable both. If not settable
  by config, the agent will add a tiny shim to force them + mode=3.
- If discovery is the only blocker, fallback shim plan (agent, offline):
  inline-hook `FUN_1043e645` (MOVZX EAX,[ECX+0xBC];RET) behind a co-op-session gate so it
  returns 3 only in a session, plus force the two Lan* tunables. See NOTES 28i13.

## Report back (for the agent to log in NOTES)
- Does the LAN/Local host start on A?
- Does the session appear in B's LAN browser? (yes/no)
- If it joins: does gameplay sync (both players move/shoot/see each other)?

---

# FC3 кооп — чек-лист теста двух инстансов по LAN / Local-сессии
Цель: эмпирически проверить, могут ли два инстанса mod-262 создать HOST и JOIN Local/LAN
co-op-сессию по виртуальному LAN. Это решающий шаг к коопу 2+ человек после отключения
онлайн-серверов Ubisoft (см. docs/project-status.ru.md).

## Зачем (кратко)
- Статический RE (NOTES 28i11–28i13) показал: в FC3 есть полный LAN/Storm co-op-путь —
  рантайм-байт режима `mgr+0xbc` (1 Offline / 2 Online / 3 Lan-Local / 4 Split / 5 Single),
  протокол `storm::echo` master/replica и LAN-broadcast на **UDP 9004 / 9005**
  (`LanHostBroadcastPort`/`LanClientBroadcastPort` в `GamerProfile.xml`).
- Сборка mod-262 уже разблокировала СТАРТ «Local Session» (по её READ.ME).
- Мёртвый RV/онлайн-сервер (2024-03-06) НЕ влияет на LAN-broadcast.
- Не проверено лишь одно: может ли второй инстанс JOIN к хосту.

## Требования
- Два установленных Far Cry 3 с рецептом mod-262
  (чистая 1.06 → Far CRy3 FIX Co-oP → The Dependencies-262 → patch_solo_offline_coop_vanilla-262).
  Рабочая копия `H:\FarCryCoop\TestFC3_105` — инстанс A. Инстанс B — вторая копия
  (агент клонирует по запросу — копия большая, нужно согласие владельца).
- Виртуальный LAN для обоих: **Radmin VPN** (проверен сообществом для FC3) или Hamachi.
  Оба хоста получают адреса одной сети Radmin.
- Фаервол: разрешить игру в обе стороны по UDP **9000-9005**.

## Порядок
1. Запустить Radmin VPN; оба инстанса в одной сети (например 26.x.x.x).
2. Не дробить MTU; оставить по умолчанию.
3. Инстанс A: запуск (mod-262), главное меню → Multiplayer → **Co-op → Local Session → Host**.
   Выбрать «Local/LAN», не «Online». Создать сессию. Убедиться, что она видна в LAN-списке.
4. Инстанс B: запуск (mod-262) → Co-op → Local Session → **Browse / Join**. Сессия A должна
   появиться через LAN-broadcast (порты 9004/9005).
5. Если B видит и заходит к A → УСПЕХ: офлайн-кооп на 2 игрока работает. Дальше тест 3-го/4-го слота.
6. Собрать логи обоих инстансов (coop_shim_last.log, если стоит шим; иначе поведение в игре + скриншот).

## Если B не видит A
- Оба в одном виртуальном LAN? Одна подсеть, фаервол открыт для UDP 9004/9005.
- Loopback-обнаружение (один пк, два инстанса): у FC3 есть опции
  `LanMatchmakingDiscoveryOnLoopbackEnable` и `LanMatchmakingReuseAddress` (реестр FUN_107a227d).
  Если задаются конфигом — включить обе. Если нет — агент добавит шим для форса обоих + mode=3.
- Если единственное препятствие — обнаружение, запасной шим-план (агент, офлайн): inline-хук
  `FUN_1043e645` (MOVZX EAX,[ECX+0xBC];RET) под гейтом co-op-сессии, чтобы возвращал 3 только
  в сессии, плюс форс двух Lan*-опций. См. NOTES 28i13.

## Что сообщить агенту (для NOTES)
- Стартует ли LAN/Local-хост на A?
- Появляется ли сессия в LAN-браузере B? (да/нет)
- Если зашло: синхронизируется ли геймплей (оба игрока двигаются/стреляют/видят друг друга)?
