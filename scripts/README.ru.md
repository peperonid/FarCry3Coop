


## Live-харнесс co-op (D0 / захват RendezVous + OnlineConfig)

PowerShell-харнесс-скрипты, запускающие игру под proxy+шим ради захвата живого co-op/RV. Запускать из **повышенной (admin)** PowerShell (config_server занимает :80).

| Скрипт | Что делает |
|---|---|
| `run_d0.ps1` | Стратегия-D этап D0 (docs/strategy-d-rv-emulation-plan.md): net-on конфиг (без FAKE_ONLINE) + шим RV-PROBE/MGR_WATCH + `rv_server.py :50000` + `config_server.py :80` (SandboxUrl -> prudp://127.0.0.1:50000). Владелец доводит игру до Co-op->Host; цель - поймать >=1 PRUDP SYN (D реализуема) или доказать ноль дозвона RV (D мертва). |
| `d0_status.ps1` | Читает логи D0 и печатает вердикт PASS/FAIL/UNKNOWN (rv_capture SYN/ESTABLISHED + шим RV-PROBE + OnlineConfig GET + UDP:9000). |
| `run_a2_livecap.ps1` | Прежний net-on RV/LAN-захват (тот же рецепт, без RV-PROBE/MGR_WATCH). |
| `stop_a2_livecap.ps1` | Откат обоих: убивает игру + серверы (:80,:50000), снимает шим, возвращает чистый mod-262 загрузчик. |
| `run_a2_rvcapture.bat` | Более старый однострочник (rv_server + config_server + шаг hosts). |
