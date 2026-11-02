# Рабочая папка

English version: [workspace.md](workspace.md)

Каноническая схема проекта — дерево владельца под `E:\FarCryCoop\` (см. золотое правило 2 в
[AGENTS.ru.md](../AGENTS.ru.md)). Это дерево и его загрузчик `E:\FarCryCoop\env.ps1` на данном ПК
отсутствуют; фактические пути этого ПК — в разделе «Пути на этом ПК» ниже.
Сначала загрузить окружение: `. E:\FarCryCoop\env.ps1`.

| Папка | Содержимое |
|---|---|
| `Downloads\<имя>\` | Исходные архивы, у каждого своя папка, SHA-256 проверен |
| `Tools\` | Распакованные портативные инструменты |
| `Data\` | Настройки и кэши инструментов (настройки/кэш Ghidra, кэш npm, глобальный префикс npm) |
| `Temp\` | Временные файлы (сюда указывают `TEMP`/`TMP`) |
| `FarCryCoop\`, `FarCry<N>Coop\` | Репозитории, по одной папке на игру |

## Пути на этом ПК (проверено 2026-11-02)

| Что | Путь |
|---|---|
| Репозитории (эта игровая копия, многоигровой конвейер, конвейер coop-anything) | `D:\Projects\Co-op Anything\` |
| Тестовая копия игры (1.06) | `H:\FarCryCoop\TestFC3\` |
| Установка только для чтения (Ubisoft Connect) | `H:\Uplay\Far Cry 3\` |
| Черновики и личные данные | `<репозиторий>\local\`, `<репозиторий>\local\scratch\` (вне git) |
| Временные файлы агентской программы | `C:\Users\<user>\AppData\Roaming\pi-desktop\tmp` |

Все пути `E:\FarCryCoop\` в этом документе и в NOTES.ru.md — историческая запись с ПК владельца
(октябрь 2026); диска `E:` на этом ПК нет. Живые пути — в таблице выше.

## Установленные инструменты (2026-10-18)

| Инструмент | Версия | Источник | SHA-256 (архив) |
|---|---|---|---|
| Node.js | v24.22.0 LTS (zip win-x64) | https://nodejs.org/dist/v24.22.0/ | `f9683a19877e12a9b167305c8db71040d812876359de21f0d2e97c3f321f80c3` |
| Eclipse Temurin JDK | 21.0.12.1+1 (zip x64) | https://github.com/adoptium/temurin21-binaries/releases/tag/jdk-21.0.12.1%2B1 | `f9d6e191ab098c0d416e7d588a24420a8621cd2f4720dab2459b8b7b2d2d8b4e` |
| Ghidra | 12.1.4 (`ghidra_12.1.4_PUBLIC_20260921.zip`) | https://github.com/NationalSecurityAgency/ghidra/releases/tag/Ghidra_12.1.4_build | `ddac49f903da9d5bac833e5cc79395098b9c33cfd3279be5f31bd00387d2d4db` |

Контрольные суммы взяты из `SHASUMS256.txt` на nodejs.org, API Adoptium и релиза Ghidra на GitHub;
все три загрузки совпали.

В Ghidra изменён `support\launch.properties` (оригинал сохранён как `launch.properties.orig`):
`JAVA_HOME_OVERRIDE`, `application.settingsdir`, `application.cachedir`, `application.tempdir`,
`java.io.tmpdir` указывают в `E:\FarCryCoop`. Проверка: `analyzeHeadless.bat` вывел справку и
создал папки настроек/кэша в `E:\FarCryCoop\Data`.

Примечание (2026-11-02): раздел «Установленные инструменты» ниже и пассбажн вышеованая информация — запись с ПК владельца (октябрь 2026), где был диск `E:`. На этом ПК Ghidra, JDK, Node и загрузчик `env.ps1` отсутствуют.

Историческая запись (2026-10-18): установки инструментов и проверка Ghidra записаны на ПК владельца,
где был диск `E:`. На данном ПК Ghidra, JDK и Node не установлены, `env.ps1` отсутствует, диска `E:`
нет, поэтому такие пути в этом документе — история, а не живые пути.

## Известные пределы правила «не трогать C:»

- Сам Claude Code, Windows, PowerShell и системный `git.exe` (`C:\Program Files\Git`) находятся на C:.
  Они используются как установлены; наши инструменты туда ничего нового не пишут.
- `claude mcp add --scope local` (способ подключения REA) пишет в `%USERPROFILE%\.claude.json` на C:.
  На этом ПК не делалось: сервер REA не установлен (нет CLI `claude`, нет Ghidra).
- Файл правил агента — `AGENTS.md` и `AGENTS.ru.md`; `CLAUDE.md` оставлен только как заглушка на них,
  потому что часть окружения читает именно это имя. Прежний текст сохранён как `CLAUDE.md.keep`.
- Агентская программа пишет свои временные файлы в `C:\Users\<user>\AppData\Roaming\pi-desktop\tmp` (вывод субагентов, песочные скрипты).
  Это нельзя перенастроить нашими настройками, и это противоречит золотому правилу 2 — помечено,
  не исправлено (решение владельца).
