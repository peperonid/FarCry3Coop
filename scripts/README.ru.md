# scripts/

Инструменты анализа **только для чтения** сборок Far Cry 3 1.05 (`H:/FarCryCoop/105/game-105`)
и 1.06 (`H:/FarCryCoop/TestFC3`), установщика Ubisoft Connect (`H:/Uplay`) и розничной 1.06
(`H:/Uplay/Far Cry 3`). Почти все скрипты читают PE-файлы; байты могут менять только
`patch_dll.py` и `fc3_trampoline.py`, причём `fc3_trampoline.py install --apply` на этой машине
ни разу не запускался. Запускать нужно через `python -I` (изолированный режим: в каталоге игры
лежит `python3.dll`, который иначе ломает `python.exe`).

## Общий помощник

| Скрипт | Что делает |
|---|---|
| `fc3_addrmap.py` | карта RVA → файловое смещение для одной сборки (`PE`, `va2off()`, поиск строк и ссылок на данные). Нужен скриптам `fc3_func_dump.py`, `fc3_gate_probe.py`, `fc3_iat_find.py`, `fc3_iat_scan.py`, `fc3_reexec_probe.py`, `fc3_reg_keys.py`. |

## Основы PE

`fc3_pe_info.py` (заголовок, секции, экспорты, импорты), `fc3_imports.py` (директория импорта),
`fc3_dump.py` (hex/ASCII-дамп диапазона VA и поиск прологов функций), `fc3_vinfo_probe.py` и
`fc3_version_probe.py` (версия файла и sha256 текущей установки), `fc3_rsrc_probe.py`
(ресурсная директория `farcry3_d3d11.exe`).

## Строки и таблица импортов

`fc3_strings.py` (строки лагнера и перезапуска, их читатели), `fc3_strgrep.py` (поиск строк в
.rdata/.data), `fc3_args_scan.py` (поиск маркеров по каталогу или всей установке),
`fc3_args_context.py` (читаемый контекст вокруг найденных маркеров), `fc3_iat_scan.py`,
`fc3_iat_map.py`, `fc3_iat_find.py` (слот таблицы импорта ↔ импортируемое имя ↔ места вызова),
`fc3_iat_reverse.py` (адрес трюка ↔ импортируемое имя).

## Разбор командной строки и пути запуска

`fc3_cmdline_cluster.py` (кластер разбора ключа по VA строки-ключа), `fc3_switchtable.py`
(таблица переходов за разбором аргументов запуска), `fc3_quicklaunch_map.py`, `fc3_switch_context.py` (находит все места, которые кладут в стек
строку-ключ и вызывают разбор: сырой скан `68 <VA строки> ... e8 <rel32>` плюс проход по
el beginnings функций, который сообщает, куда попадает тело — `--mode prologue|raw|both`),
`fc3_reloc_xref.py` (перекрёстные ссылки адреса данных; поправка: disp32 в `.text` содержит VA как есть,
поэтому обход `.reloc` даёт только места патча загрузчика), `fc3_param_records.py` (перечисление
записей дескриптора сессии: `push <VA имени>; push <код типа>; call <создатель>` -> имя, код типа,
место вызова; аргументы-создатели передаются позиционно, не флагом), `fc3_func_dump.py`
(тело функции: встроенные строки и ссылки на данные), `fc3_callers.py` (обратные поиски
вызывающих), `fc3_dataslot.py` (на какой слот данных указывает абсолютная ссылка),
`fc3_reexec_probe.py` (перезапуск-бутстрап 1.06), `fc3_drm_probe.py` (бутстрап DRM/UPC),
`fc3_gate_probe.py` (проверка gates и mutex-ов), `fc3_reg_keys.py` (кто читает
`HKCU/Software/Ubisoft/Far Cry 3`), `fc3_dis.py` (разбирающий дизассемлер capstone с
аннотациями: вызывающие, импортируемые имена, ссылки на строки — главный инструмент всех
наблюдений по 1.06).

## Запуск (реально что-то запускают)

`fc3_launch_probe.py` (запуск тестовой копии 1.06 с аргументами quick-launch и сбор логов),
`fc3_upc_probe.py` (просим Ubisoft Connect запустить нашу тестовую копию и записываем точную
командную строку), `fc3_test_startargs.py` (проверяет, передаёт ли хаб реестровое значение
`ExecutableName` в командную строку запущенного процесса игры), `launch_105.ps1`,
`build_testgame.ps1`, `build_patch.ps1`, `reunpack.ps1`, `switch_variant.ps1`, `thread_eips.ps1`.

## Сравнение и данные (работа владельца Phase 1)

`bindiff.py`, `contentcompare.py`, `dupcheck.py`, `fat.py`, `fatcompare.py`, `manifest_diff.py`,
`edit_playlist.py`, `names.py`, `names_ext.py`, `find.py`, `findpats.py`, `patchctx.py`,
`reg_fields.py`, `peinfo.py`, `patch_dll.py` (правит байты **копии** FC3.dll / FC3_d3d11.dll),
`fc3_trampoline.py` (14-байтовый jmp-трамполин; `install --apply` здесь не запускался — решение
за владельцем).

## Кэши и игнорируемые файлы

`*_dis_cache.json` (кэш дизассемлирования), `__pycache__/`, `../Temp/` (черновики агента) и
`../local/` в git не попадают.

## Перенесённые пути

Скопированные с ПК владельца (октябрь 2026) скрипты содержали пути старой раскладки: `DLL_106 = E:\SteamLibrary\steamapps\common\Far Cry 3\bin\FC3.dll`, `DLL_105 = E:\FarCryCoop\Downloads\fc3-1.05\bin\FC3.dll`. Диска `E:` на этом ПК нет. Такие скрипты сохранены как запись и НЕ перенацеливаются молча: перенацелить скрипт, который пишет в папку игры (patch / build / unpack), — решение владельца. На этом ПК установка только для чтения — `H:\Uplay\Far Cry 3\`, тестовая копия 1.06 — `H:\FarCryCoop\TestFC3\`; скрипты только для анализа здесь уже читают проверенные пути.
