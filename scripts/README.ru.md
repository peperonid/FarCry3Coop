


## Live-харнесс co-op (D0 / захват RendezVous + OnlineConfig)

PowerShell-харнесс-скрипты, запускающие игру под proxy+шим ради захвата живого co-op/RV. Запускать из **повышенной (admin)** PowerShell (config_server занимает :80).

| Скрипт | Что делает |
|---|---|
| `run_d0.ps1` | Стратегия-D этап D0 (docs/strategy-d-rv-emulation-plan.md): net-on конфиг (без FAKE_ONLINE) + шим RV-PROBE/MGR_WATCH + `rv_server.py :50000` + `config_server.py :80` (SandboxUrl -> prudp://127.0.0.1:50000). Владелец доводит игру до Co-op->Host; цель - поймать >=1 PRUDP SYN (D реализуема) или доказать ноль дозвона RV (D мертва). |
| `d0_status.ps1` | Читает логи D0 и печатает вердикт PASS/FAIL/UNKNOWN (rv_capture SYN/ESTABLISHED + шим RV-PROBE + OnlineConfig GET + UDP:9000). |
| `run_a2_livecap.ps1` | Прежний net-on RV/LAN-захват (тот же рецепт, без RV-PROBE/MGR_WATCH). |
| `stop_a2_livecap.ps1` | Откат обоих: убивает игру + серверы (:80,:50000), снимает шим, возвращает чистый mod-262 загрузчик. |
| `run_a2_rvcapture.bat` | Более старый однострочник (rv_server + config_server + шаг hosts). |

| `d0_reallogin_watch.ps1` | ПАССИВНЫЙ контроль: при НАСТОЯЩЕМ логине Ubisoft Connect (без модов, без proxy/shim) на оригинальном инсталле следит за исходящими соединениями farcry3_d3d11 во время Co-op->Host и ищет любые не-LAN (Ubisoft) адреса. Отвечает на единственную непроверенную переменную: дозванивается ли FC3 до RV/лобби вообще при реальном логине? (публичный адрес => RV-клиент достижим, мёртв только бэкенд; нет => гейт выше RV). Только чтение, без admin. Лог: `d0_reallogin_watch.log`. |

| `check_clean_install.py` | Только чтение: свежеразвёрнутый FC3_clean - это ПОДЛИННАЯ немодифицированная сборка для контроля real-login? Проверяет bin/FC3_d3d11.dll md5 == ванильный 1.05 (46bcc097) И побайтовое равенство локальному ванильному эталону (ловит 7-байтовый патч mod-262), ищет остатки модов/харнесса (coop_shim, uplay_api.log, real/, .orig), печатает загрузчики. `python -I scripts/check_clean_install.py [папка]`. exit 0=чисто. |

| `gator_mock/hosts_clean.ps1` | Из админа. Комментирует оставшиеся записи hosts `127.0.0.1 *.ubi.com` (след capture, молча блокирующий реальный Ubisoft online-логин и контаминящий real-login наблюдение). Сначала бэкапит hosts в H:\FarCryCoop, затем flushdns. ВАЖНО: `gator_mock/hosts_patch.py restore` БЕСПОЛЕЗЕН (его hosts.backup снят уже заражённым). |

| `run_d0_reallogin.ps1` / `stop_d0_reallogin.ps1` | РЕШАЮЩИЙ real-login RV-контроль на чистом инсталле (GAME_DIR, по умолчанию FC3_clean). Ставит прокси (прозрачный форвард к подлинному загрузчику) + шим-НАБЛЮДАТЕЛЬ (RV-PROBE/MGR_WATCH/login), БЕЗ фейков / БЕЗ DNS-редиректа / БЕЗ локальных серверов, запускает игру для НАСТОЯЩЕГО логина Ubisoft Connect и читает RV-PROBE (внутрипроцессные функции RV-клиента) - независимо от протокола, в отличие от netstat-наблюдателя, который не видит незакрытый UDP. stop_ возвращает чистый загрузчик и печатает вердикт RV-PROBE RUNTIME. |

| `pktmon_rv_capture.ps1` | Из админа, БЕЗ патча игры. Пишет реальный RendezVous-трафик, который FC3 отправляет при co-op на настоящем логине Ubisoft (NOTES 28f5: RV-клиент срабатывает). Через Windows pktmon; кладёт .etl/.pcap/.txt в `scripts/rv_capture/` (в git-ignore). Показывает RV UDP-адресата + данные = эталонный трафик для плана локального RV-эмулятора. |
| `rv_capture/rv_enc_extract.py` | Офлайн: разобрать лог RV-ENC шима (FC3COOP_RV_ENC=1), восстановить поточный ключ RC4 prudps на соединение из пары открытый/шифртекст (стим=pt^ct) и применить `--apply` к rv.pcap для дешифра RV-пакетов (NOTES 28g2). |

## Тест LAN / Local-Session коопа двумя инстансами

| Скрипт | Что делает |
|---|---|
| `fc3_lan_2inst_loopback.bat` | ОДИН ПК: запускает инстанс A как `-host` и инстанс B (клон `TestFC3_105_B`) как `-client -ip 127.0.0.1`, каждый со своим `-SavePath`. Рискованный путь; если B не видит A - используй скрипты Radmin. См. docs/fc3-lan-coop-test.md. |
| `fc3_lan_radmin.bat` | ДВА ПК в сети Radmin VPN: `... host` на хосте, `... client <Radmin-IPv4-хоста>` на подключающемся. Рецепт Bantila `-host/-client/-coop` + отдельный `-SavePath`. Разрешите UDP 9000-9005 в фаерволе. |

| `fc3_lan_same_subnet.bat` host \| `... client [ip]` | ДВА ПК в ОДНОЙ реальной LAN (без VPN), самый простой путь. Рецепт Bantila -host/-client/-coop + -SavePath; клиент может подключиться по LAN-IP хоста или через LAN-браузер в игре. |
| `fc3_lan_firewall.bat` | От админа, на обоих ПК: открывает UDP 9000-9005 в обе стороны для FC3 (правила netsh advfirewall + команды удаления). |

| `fc3_lan_host_only.bat` [map] | Запуск ТОЛЬКО хоста (инстанс A) по рецепту Bantila `-host`, чтобы сначала убедиться, что Local/LAN-хост стартует на этой сборке, ещё без клиента. |
| `fc3_host_engine_check.ps1` | Read-only поллер: после Co-op>Host в игре подтверждает, что LAN/net-движок поднялся, перечисляя UDP 9000-9005 процесса farcry3_d3d11.exe (9000 OnlineEnginePort / 9004 LanHostBroadcastPort). Без админа, без патча. Вердикт PASS/PARTIAL. |
