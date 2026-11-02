# Workspace

Russian version: [workspace.ru.md](workspace.ru.md)

Canonical layout of this project: the owner's tree under `E:\FarCryCoop\` (see golden rule 2 in
[AGENTS.md](../AGENTS.md)). That layout and its loader `E:\FarCryCoop\env.ps1` do not exist on this PC;
the paths actually in use here are in the section "Paths on this PC" below.

| Folder | Contents |
|---|---|
| `Downloads\<name>\` | Original archives, one folder each, SHA-256 checked |
| `Tools\` | Unpacked portable tools |
| `Data\` | Tool settings and caches (Ghidra settings/cache, npm cache, npm global prefix) |
| `Temp\` | Temporary files (`TEMP`/`TMP` point here) |
| `FarCryCoop\`, `FarCry<N>Coop\` | Repositories, one folder per game |

## Paths on this PC (verified 2026-11-02)

| What | Path |
|---|---|
| Repos (this game repo, the multi-game hub, the coop-anything hub) | `D:\Projects\Co-op Anything\` |
| Test copy of the game (1.06) | `H:\FarCryCoop\TestFC3\` |
| Read-only install (Ubisoft Connect) | `H:\Uplay\Far Cry 3\` |
| Scratch and private state | `<repo>\local\`, `<repo>\local\scratch\` (gitignored) |
| Agent harness temp | `C:\Users\<user>\AppData\Roaming\pi-desktop\tmp` |

Every `E:\FarCryCoop\` path kept in this document and in NOTES.md is a historical record from the owner's
PC (October 2026); drive `E:` does not exist here. Use the table above for live paths.

Note (2026-11-02): the table below and the Ghidra paragraph record the owner's install from
October 2026 (drive `E:`). On this PC Ghidra, JDK, Node and `env.ps1` are absent.
## Installed tools (2026-10-18)

| Tool | Version | Source | SHA-256 (archive) |
|---|---|---|---|
| Node.js | v24.22.0 LTS (win-x64 zip) | https://nodejs.org/dist/v24.22.0/ | `f9683a19877e12a9b167305c8db71040d812876359de21f0d2e97c3f321f80c3` |
| Eclipse Temurin JDK | 21.0.12.1+1 (x64 zip) | https://github.com/adoptium/temurin21-binaries/releases/tag/jdk-21.0.12.1%2B1 | `f9d6e191ab098c0d416e7d588a24420a8621cd2f4720dab2459b8b7b2d2d8b4e` |
| Ghidra | 12.1.4 (`ghidra_12.1.4_PUBLIC_20260921.zip`) | https://github.com/NationalSecurityAgency/ghidra/releases/tag/Ghidra_12.1.4_build | `ddac49f903da9d5bac833e5cc79395098b9c33cfd3279be5f31bd00387d2d4db` |

Checksums came from nodejs.org `SHASUMS256.txt`, the Adoptium API and the Ghidra GitHub release;
all three downloads matched.

Ghidra `support\launch.properties` was changed (original kept as `launch.properties.orig`):
`JAVA_HOME_OVERRIDE`, `application.settingsdir`, `application.cachedir`, `application.tempdir`,
`java.io.tmpdir` point into `E:\FarCryCoop`. Check run: `analyzeHeadless.bat` printed its usage and
created its settings/cache folders under `E:\FarCryCoop\Data`.

Historical evidence (2026-10-18): the tool installs and the Ghidra check above were recorded on the
owner's PC with drive `E:`. On this PC Ghidra, JDK and Node are not installed, `env.ps1` is absent and
`E:` does not exist, so these folder references here are history, not live paths.

## Known limits of "never touch C:"

- Claude Code itself, Windows, PowerShell and the system `git.exe` (`C:\Program Files\Git`) live on C:.
  They are used as installed; nothing new is written there by our tools.
- `claude mcp add --scope local` (the REA setup route) writes to `%USERPROFILE%\.claude.json` on C:.
  Not done here; the REA MCP server is not installed on this PC (no `claude` CLI, no Ghidra).
- The agent instruction file is `AGENTS.md` (with `AGENTS.ru.md`); `CLAUDE.md` is kept only as a shim that
  points to it, because some harnesses read that file name. The previous version is preserved as
  `CLAUDE.md.keep`.
- The agent harness writes its own temp work to `C:\Users\<user>\AppData\Roaming\pi-desktop\tmp` (delegate outputs, sandbox scripts).
  That is not redirectable through our settings and contradicts golden rule 2 — flagged, not fixed
  (owner decision).
  Not done; needs the owner's decision.
