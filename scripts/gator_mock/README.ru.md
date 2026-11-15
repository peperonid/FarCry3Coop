# gator_mock - перехват онлайн-бустрапа FC3 (первый слой A2)

FC3 (бинарники 1.05) обращается к `gconnect.ubi.com` по **открытому HTTP на порту 80**, чтобы
проверить онлайн-сервис; сегодня этот хост жив, но возвращает состояние «service unavailable», и
игра показывает «Служба Far Cry 3 недоступна». Этот инструмент пишет точные запросы, чтобы изучить
протокол и начать на него отвечать (A2: локальный эмулятор GATOR/gconnect; см. docs/coop-gator-emulation-plan.ru.md).

Как ведёт себя онлайн-путь игры (NOTES 28br): TCP к gconnect.ubi.com:80 (HTTP), затем игра занимает
UDP 9000 (движок Storm) + 3074 (GATOR/консоль); сессия не образуется, т.к. gconnect возвращает «down».

Шаги (всё из **повышенного** терминала - hosts-файл и порт 80 требуют админа):
1. `python -I hosts_patch.py apply`      # бэкап hosts, редирект gconnect/onlineconfig на 127.0.0.1
2. `python -I http_logger.py`            # логирующий HTTP-сервер на 0.0.0.0:80 (оставить работать)
3. Запустить FC3 штатно, Multiplayer -> Co-op -> попытаться создать/захостить приватную сессию.
4. Прочитать `gator_http_log.txt` (рядом с этими файлами) - записаны все method/path/заголовки/тело.
5. `python -I hosts_patch.py restore`    # вернуть hosts обратно по завершении

Далее после захвата: подготовить готовые ответы (GATOR_CANNED=<файл>), чтобы пройти ворота «unavailable»,
затем браться за слой Quazal RV/PRUDP по UDP (порты 3074/9000), где живёт криптография.

Одноразовый захват (предпочтительно): из админ-терминала `python -I run_capture.py` - отвечает на GetOnlineConfig файлом fc3_config.json (RV SandboxUrl -> 127.0.0.1:50000) И пишет PRUDP-пакеты на этот порт в gator_udp_log.txt. Правь fc3_config.json, чтобы менять отдаваемый конфиг (напр. RV-порт).

## Эксперимент LAN co-op (28bu/28bv)
`lan_mode.py lan|online|restore|status|find` переключает в GamerProfile.xml `<NetworkProfile SessionProvider="Lan|">` (автобэкап). В FC3 есть встроенный LAN-провайдер сессий (вещание 9004 / обнаружение 9005); если LAN-ко-оп-сессия joining офлайн, ко-оп получен без Ubisoft/RV/PRUDP. Обратимо; оригинал хранится как GamerProfile.xml.bak-<ts>.
