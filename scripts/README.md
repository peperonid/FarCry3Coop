# scripts/

Read-only analysis tooling for Far Cry 3 builds 1.05 (`H:\FarCryCoop\105\game-105`) and
1.06 (`H:\FarCryCoop\TestFC3`), plus the Ubisoft Connect install at
`H:\Uplay` and the retail install `H:\Uplay\Far Cry 3`. Every script here is meant to
**read** PE files and, in two cases, to **run** the game; `patch_dll.py` and
`fc3_trampoline.py` are the only ones that can write bytes, and `fc3_trampoline.py
install --apply` has never been run on this machine. Run them with `python -I`
(isolated mode: no user site-packages, no stray `python*.dll` on PATH - required because
the game directory contains `python3.dll`, which otherwise breaks `python.exe`).

## Shared helper

| Script | What it does |
|---|---|
| `fc3_addrmap.py` | RVA -> file-offset map for one build (`PE` class, `va2off()`, string/dref lookup). Used by `fc3_func_dump.py`, `fc3_gate_probe.py`, `fc3_iat_find.py`, `fc3_iat_scan.py`, `fc3_reexec_probe.py`, `fc3_reg_keys.py`. |

## PE basics

`fc3_pe_info.py` (header/sections/exports/imports), `fc3_imports.py` (import directory),
`fc3_dump.py` (hex/ASCII dump of a VA range + prologue detection), `fc3_vinfo_probe.py`
and `fc3_version_probe.py` (file version + sha256 of the current install),
`fc3_rsrc_probe.py` (resource directory of `farcry3_d3d11.exe`).

## Strings and IAT

`fc3_strings.py` (launcher/re-exec strings + readers), `fc3_strgrep.py` (grep .rdata/.data
strings), `fc3_args_scan.py` (marker scan in a directory or whole install),
`fc3_args_context.py` (readable context around marker hits), `fc3_iat_scan.py`,
`fc3_iat_map.py`, `fc3_iat_find.py` (IAT slot <-> imported name <-> call sites),
`fc3_iat_reverse.py` (thunk address -> imported name).

## Command-line / launch analysis

`fc3_cmdline_cluster.py` (switch-parsing cluster from a switch-string VA),
`fc3_switchtable.py` (jump table behind the launch switch), `fc3_quicklaunch_map.py`,
`fc3_switch_context.py` (find every site that pushes a switch string and calls the parser:
raw `68 <switch VA> ... e8 <rel32>` scan plus a function-prologue walk that reports where the
function body lands - `--mode prologue|raw|both`), `fc3_reloc_xref.py` (cross references of an absolute
data VA - premise corrected 2026-11-03: disp32 fields in `.text` hold the default image-base VA
verbatim, so the tool scans the ModRM disp32 forms and verifies each hit by backward decoding, while
relocation patch sites are reported separately - `--mode code|reloc|both`),
`fc3_param_records.py` (enumerate session-descriptor records: `push <name VA>; push <type code>;
call <creator>` -> name, type code, call site. Creator VAs are POSITIONAL arguments (not `--creator`);
`--window 64` is the look-back distance and `--ident-only` drops cross-function false positives. Use the
corrected registrar families: `FC3_d3d11.dll` `0x11acaba0`/`0x11acac50`/`0x11acad00`/`0x11acb320`/
`0x11acaf10`, `FC3.dll` `0x11ab6750`/`0x11ab6800`/`0x11ab68b0`/`0x11ab6ac0`/`0x11ab6b70`/`0x11ab6e20`/
`0x11ab6ed0`, and the exec-record creators `0x1003b4b0` (FC3.dll) / `0x10638fb0` (FC3_d3d11.dll) -
see NOTES.md "Session-descriptor record tables"),
`fc3_func_dump.py` (function body: embedded strings + data references),
`fc3_callers.py` (reverse callers of a function), `fc3_dataslot.py` (which data slot a
absolute data reference points at), `fc3_reexec_probe.py` (1.06 re-exec bootstrap),
`fc3_drm_probe.py` (DRM/UPC bootstrap), `fc3_gate_probe.py` (gate/lock check),
`fc3_reg_keys.py` (which code reads `HKCU\Software\Ubisoft\Far Cry 3`),
`fc3_dis.py` (annotating capstone disassembler: callers, imported names, string
references - the main RE tool of the 1.06 findings).

## Launch helpers (run things)

`fc3_launch_probe.py` (launch the 1.06 test copy with quick-launch arguments and capture
logs), `fc3_upc_probe.py` (ask Ubisoft Connect to launch our test copy and record the
exact command line), `fc3_test_startargs.py` (check whether the hub forwards the
`ExecutableName` registry value), `launch_105.ps1`, `build_testgame.ps1`,
`build_patch.ps1`, `reunpack.ps1`, `switch_variant.ps1`, `thread_eips.ps1`.

## Diffing / data tools (from the owner's Phase 1 work)

`bindiff.py`, `contentcompare.py`, `dupcheck.py`, `fat.py`, `fatcompare.py`,
`manifest_diff.py`, `edit_playlist.py`, `names.py`, `names_ext.py`, `find.py`,
`findpats.py`, `patchctx.py`, `reg_fields.py`, `peinfo.py`, `patch_dll.py` (byte patches
of a **copy** of FC3.dll / FC3_d3d11.dll), `fc3_trampoline.py` (14-byte jmp stub writer;
`install --apply` never run here - owner decision pending).

## Caches and ignored files

`*_dis_cache.json` (disassembly cache), `__pycache__/`, `../Temp/` (agent scratch) and
`../local/` are gitignored.

## Carried-over paths

The scripts copied from the owner's PC (October 2026) hardcoded the old layout: `DLL_106 = E:\SteamLibrary\steamapps\common\Far Cry 3\bin\FC3.dll`,
`DLL_105 = E:\FarCryCoop\Downloads\fc3-1.05\bin\FC3.dll`. Drive `E:` does not exist on this PC. These scripts are
kept as evidence and are NOT retargeted silently: re-pointing a script that writes into the game
install (patch / build / unpack) is an owner decision. On this PC the read-only install is
`H:\Uplay\Far Cry 3\`, the 1.06 test copy is `H:\FarCryCoop\TestFC3\`; analysis-only scripts here
already read those verified paths.
