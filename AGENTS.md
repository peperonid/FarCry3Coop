# Project rules for the coding agent / Правила проекта для кодинг-агента

Canonical instruction file. The agent harness (Pi) auto-loads `AGENTS.md` from the working

directory and from every parent directory, and reads candidates in this order:

`AGENTS.override.md`, `AGENTS.md`, `AGENTS.MD`, `CLAUDE.md`, `CLAUDE.MD`. So this file is read

by the agent automatically in every session; `CLAUDE.md` in this repo is only a pointer to it.

## Golden rule / Золотое правило

**EN:** ALWAYS write documentation and every other human-readable project text (README, notes,

guides, comments in docs, issue/PR descriptions) in BOTH Russian and English. No exceptions.

Use a pair of files `NAME.md` (English) + `NAME.ru.md` (Russian), or both languages in one file

for short texts. When one language version changes, update the other in the same commit.

**RU:** ВСЕГДА пиши документацию и любые другие тексты проекта для людей (README, заметки,

руководства, комментарии в документах, описания issue/PR) на ДВУХ языках: русском и английском.

Без исключений. Используй пару файлов `NAME.md` (английский) + `NAME.ru.md` (русский) или оба

языка в одном файле для коротких текстов. Если меняется одна языковая версия, обнови другую в

том же коммите.

## Golden rule 2: workspace / Золотое правило 2: рабочая папка

**EN:** Work only inside the paths listed in "Workspace on this PC" below. NEVER read, write or

download anything on drive C: — no exceptions. NEVER download into `%TEMP%`. Never modify a game

install (`H:\Uplay\Far Cry 3\` is read-only); work on the copy `H:\FarCryCoop\TestFC3\`. Scratch

files go to ignored locations only (`local/scratch/`, `Temp/`, `*.scratch*`). If a step needs

drive C:, stop and ask the owner. One deviation is unavoidable and already accepted by the owner:

the agent harness writes its own temp files and delegate output under

`C:\Users\<user>\AppData\Roaming\pi-desktop\` — flagged, not fixed, see docs/LOCAL.md.

**RU:** Работать только в путях из раздела «Рабочая папка на этом ПК» ниже. НИКОГДА ничего не

читать, не писать и не скачивать на диск C: — без исключений. НИКОГДА не качать в `%TEMP%`.

Игровую установку не изменять (`H:\Uplay\Far Cry 3\` — только чтение); работать с копией

`H:\FarCryCoop\TestFC3\`. Черновые файлы — только в папках вне git (`local/scratch/`, `Temp/`,

`*.scratch*`). Если шаг требует диска C:, остановиться и спросить владельца. Одно отклонение

неустранимо и принято владельцем: агентская программа пишет свои временные файлы и выводы

делегатов в `C:\Users\<user>\AppData\Roaming\pi-desktop\` — отмечено, не исправлено, см.

docs/LOCAL.md.

## Commits / Коммиты

**EN:** Commit messages follow Conventional Commits (https://www.conventionalcommits.org/en/v1.0.0/)

and are written in English only: `<type>(<optional scope>): <description>`, e.g.

`docs(far-cry-6): add research notes`. Types: feat, fix, docs, chore, refactor, test, build, ci, perf, style, revert.

**RU:** Сообщения коммитов пишутся по Conventional Commits и только на английском:

`<тип>(<область, необязательно>): <описание>`, например `docs(far-cry-6): add research notes`.

## Tools / Инструменты

**EN:** Reverse engineering uses REA (https://github.com/morluto/rea) as an MCP server, registered

locally on the owner's PC: see [docs/tools/rea.md](docs/tools/rea.md). On this PC the REA MCP server

is not configured; analysis is done with the Python scripts in `scripts/` instead.

**RU:** Для реверс-инжиниринга используется REA (https://github.com/morluto/rea) как MCP-сервер,

подключённый локально на ПК владельца: см. [docs/tools/rea.ru.md](docs/tools/rea.ru.md). На данном

ПК REA не настроен; анализ идёт скриптами из `scripts/`.

## Communication / Общение

**EN:** Chat with the owner in Russian.

**RU:** С владельцем общаться по-русски.

## Session mechanics / Механика сессии

**EN:** Tool calls: one tool block per turn, one tool name per block, arguments under ~4 KB. Never

paste a long script into a tool call: write it to a file and run `python -I scripts/<name>.py`.

Read files by line ranges, not whole.

Model requests: launch ONE model-facing request at a time (sub-agent, web search, source check) and

wait for its result before starting the next one — model requests are processed sequentially.

Owner rules are never rewritten by the agent silently: report the gap and let the owner decide.

**RU:** Вызовы инструментов: один блок на ход, один инструмент в блоке, аргументы не больше

~4 КБ. Длинные probe-скрипты писать в файл и запускать короткой командой

(`python -I scripts/<имя>.py`), не вставлять код в вызов; читать файлы частями по номерам

строк, не целиком.

Запросы к модели: запускать ОДИН запрос к модели за раз (суб-агент, веб-поиск, проверка

источников) и ждать его результат перед следующим — запросы к модели обрабатываются

последовательно. Правила владельца не переписывать молча: сообщать о расхождении и решать

владельцу.

## Repo layout / Раскладка репозитория

- `NOTES.md` / `NOTES.ru.md` — findings log. The owner's granular Phase 1 sections stay first and

  are the authoritative Phase 1 record; agent-session sections are appended after them with a

  provenance heading. Do not rewrite the owner's Phase 1 text without telling the owner.

- `PLAN.md` / `PLAN.ru.md` — campaign-co-op plan. The owner's version is authoritative; the agent's

  plan lives in the hub clone (`D:\Projects\Co-op Anything\FarCryCoop\games\far-cry-3\PLAN.md`)

  until the owner merges it.

- `scripts/` — analysis scripts, tracked, indexed in `scripts/README.md` + `scripts/README.ru.md`.

  `tools/` — external binaries only (Ghidra, packer), ignored. Caches (`*_dis_cache.json`) ignored.

- `local/` — private state, never committed: `local/scratch/` (one-off drafts, Russian text fragments,

  commit-message files) and `local/archive/` (61 carried-over evidence files + `MANIFEST.md`, copied

  from the hub clone `FarCryCoop/Temp`, `FarCryCoop/local/scratch` and the scratch folder

  `H:\FarCryCoop`). The archive is a read-only record: do not edit it, read `local/archive/MANIFEST.md`

  to find an artifact.

- Remotes: `upstream` = https://github.com/Bantila/FarCry3Coop, `origin` = the agent fork

  https://github.com/peperonid/FarCry3Coop, `hub` = https://github.com/peperonid/FarCryCoop.

- Branch `backup` is the owner's own pre-append snapshot of `main`. Never delete, never sync, never

  merge into it.

- Stage explicit paths only: `git add -A` aborts on this machine with `error: invalid path 'NUL'`

  (undeletable file `D:\Projects\Co-op Anything\nul`). Commit message via `git commit -F

  .git/COMMIT_MSG.txt`. Commit text is English; Russian text lives only in `.ru.md` files.

## Workspace on this PC / Рабочая папка на этом ПК

| Path | Role |

|---|---|

| `D:\Projects\Co-op Anything\FarCry3Coop-bantila\` | This repo (agent fork of Bantila/FarCry3Coop) |

| `D:\Projects\Co-op Anything\FarCryCoop\` | Multi-game hub clone (per-game notes under `games/<slug>/`) |

| `D:\Projects\Co-op Anything\coop-anything\` | Upstream hub repo (issue tracker, playbook) |

| `H:\Uplay\Far Cry 3\` | Real Uplay 1.06 install — read-only (golden rule 2) |

| `H:\FarCryCoop\TestFC3\` | Working copy of the 1.06 build: disassembly probes, menu runs, test mods |

| `H:\FarCryCoop\` | Scratch working folder (not a git repo): `TOOLS.md`, screenshots, UI-automation helpers, `tools\fc3_stub_probe.py` |

| `D:\Projects\Co-op Anything\FarCryCoop\tools\`, `...\games\far-cry-3\tools\` | Superseded copies of the FC3 analysis scripts; this repo's `scripts\` is the canonical set |

| `C:\Users\<user>\AppData\Roaming\pi-desktop\tmp\` | Agent harness temp (unavoidable, see golden rule 2) |

Every `E:\FarCryCoop\...` and `E:\SteamLibrary\...` path in this repo is the original 2024 layout of

the owner's first PC; that drive does not exist here. Docs and script headers were retargeted to this

table on 2026-11-02; the historical record of what was found stays unchanged.

## Read first in every session / Что читать в начале сессии

**EN:** `NOTES.md` (findings log, including the TODO list at the end), `PLAN.md`, `scripts/README.md`,

`AGENTS.md`. Do not re-derive a finding that is already written in `NOTES.md`.

**RU:** `NOTES.md` (журнал находок, список TODO в конце), `PLAN.md`, `scripts/README.md`,

`AGENTS.md`. Не заново выводить то, что уже записано в `NOTES.md`.

## Tooling gotchas on this machine / Особенности инструментов на этом ПК

**EN:**

- `ctx_*` tools run under PowerShell: `&&` and `||` are parser errors, `>` writes UTF-16,

  `2>/dev/null` creates `D:\dev\null`. Multi-command work goes through bash.

- The edit tool corrupts Cyrillic text. Write Russian text with a bash heredoc plus a Python splice.

- Console output on this machine is cp1251: print Cyrillic with `sys.stdout.buff

