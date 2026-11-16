# rv_server - эмулятор PRUDP/RendezVous (условие B плана ко-опа)

> Двуязычно: английская версия - [English here](README.md)
> Статус: **этап S1 готов** (офлайн кодек PRUDP v1 + handshake). **Этап S0 (live-захват) заперт
> условием (A)** - игра набирает RV лишь когда FCX дойдёт до `LoggingIn` (NOTES 28cw/28cv).

Это локальный эмулятор RendezVous (RV) / PRUDP: узел, через который два клиента FC3 сделали бы
rendezvous, чтобы их P2P-неткод ко-опа соединился по LAN/loopback. Это условие **B** в
`docs/coop-gator-emulation-plan.md`; голова блокера по-прежнему условие **A** (онлайн-логин FCX),
которое лежит выше этого сервера.

## Файлы
- `prudp.py` - кодек PRUDP **v1** (вариация RendezVous): `PRUDPPacket.decode/encode` + опции
  `id,(type<<4|size),value`. Little endian. Терпимый парсер сохраняет каждую опцию дословно.
- `rv_server.py` - UDP-сервер с машиной состояний handshake `SYN -> CONNECT -> (CONNECT ack) -> DATA`
  и hex- логом каждого пакета (`rv_capture.log`). Повторяет RV-опции клиента и вставляет опцию
  connection-id, чтобы клиент принял ответ.
- `test_prudp.py` - офлайн-доказательство: round-trip кодека + синтетический PRUDP-клиент,
  завершающий handshake И request->response RMC поверх established-канала. Печатает `ALL OK`.
- `config_server.py` - отдаёт скроенный OnlineConfig с `SandboxUrl -> prudp://address=127.0.0.1;port=50000`
  (шов конфигурации), чтобы клиент, начав набор, попал в rv_server.
- `../run_a2_rvcapture.bat` - единый конвейер: запускает rv_server (:50000) + config_server (:8081)
  и печатает шаги для владельца (перенаправление hosts + запуск игры).

## Запуск
```
python -I scripts/rv_server/test_prudp.py          # самотест (офлайн), печатает ALL OK
python -I scripts/rv_server/rv_server.py --host 127.0.0.1 --port 50000
```
Затем отдайте OnlineConfig, у которого `SandboxUrl` = `prudp://address=127.0.0.1;port=50000`
(шов конфигурации, NOTES 28bs). Используйте схему `prudp` (не `prudps`), чтобы избежать шифрования
полезной нагрузки на этапе захвата.

## Что работает сейчас (офлайн, без игры, без C:)
- Кодирование/декодирование пакетов PRUDP v1 (SYN/CONNECT/DATA/PING/DISCONNECT, опции, нагрузка).
- Сервер принимает Connection Request (SYN), отвечает CONNECT с connection id, доходит до
  `established`, подтверждает PING и надёжный DATA. Полный hex-захват в `rv_capture.log`.

## Слой RMC (скаффолдинг, офлайн-тест)
После handshake нагрузки надёжного DATA разбираются как `RMCMessage(call_type, protocol_id,
method_id, session_id, body)` и диспетчеризуются через `RVServer.services` по ключу
`(protocol_id, method_id)`. Синтетический клиент делает request->response в тесте. Это НАША
самосогласованная рамка; реальное Quazal-RendezVous RMC-кодирование и protocol/method ID RV надо
подтвердить живым захватом (S0), прежде чем обработчики станут осмысленными.

## Ещё нет (следующие этапы)
- **S0 live-захват**: нужно условие (A), чтобы настоящий клиент FC3 набрал нас; пока тот же путь
  проходит синтетический клиент. Конвейер: `../run_a2_rvcapture.bat` (rv_server + config_server +
  шаг hosts). Ожидаемый результат сегодня: **ноль входящего UDP** = чистое доказательство, что
  ворота это логин FCX (условие A), а не слой сокета/конфига.
- **prudps** (шифрование RC4 + HMAC нагрузки/подпись): FC3 может использовать его; контрольная
  сумма заголовка в `prudp.py` - заглушка, править по реальному захвату.
- **Реальные ID RV-опций**: номера опций connection-id / pid / cid - догадки по таблице NEX;
  подтвердить по реальному Connection Request и исправить.
- **RMC / сервисные сообщения RV** после коннекта (фактические create/join/get-ticket вызовы):
  этот протокольный слой - следующий рубеж.

## Честно
Это доказывает, что движок транспорта/handshake работает и тестируется без игры. Это НЕ значит, что
две игры уже соединяются - по-прежнему нужны условие (A) (логин FCX) плюс слой RMC-сервисов поверх
handshake. Относитесь к S1 как к фундаменту, а не к финалу.
