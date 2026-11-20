#!/usr/bin/env python
"""build_coop_package.py - build the PERSONAL 2nd-PC co-op overlay package (not for distribution).

Diff a working co-op install (MINE) against a pristine Uplay install (REF); every file that is
added or modified becomes the "payload" overlay (minus our scratch/logs/backups). Emit a folder +
zip with an INSTALL.ps1/.bat unpacker, a manifest (size+md5) and the bilingual co-op docs.
EN only; the Russian guide travels inside docs/fc3-coop-radmin-setup.ru.md.

Edit REF / MINE / OUT to match this PC. Run:  python -I scripts/build_coop_package.py
"""
import os, sys, shutil, hashlib, zipfile, datetime

REF  = r"H:\Uplay\Far Cry 3"                 # pristine licensed install (read-only reference)
MINE = r"H:\FarCryCoop\TestFC3_105"          # the working co-op install to clone
OUT  = r"H:\FarCryCoop"                       # where FC3Coop2PC_Package + .zip are written
REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PKG  = os.path.join(OUT, "FC3Coop2PC_Package")
ZIP  = os.path.join(OUT, "FC3Coop2PC_Package.zip")

JUNK_DIR  = {"_backup_106_before_solo262", "bin_backup_vanilla105"}
JUNK_BASE = {"uplay_r1_loader_vanilla105_backup.dll"}
JUNK_REL  = {"uplay_install.state", "dev_coop.ps1", "run_coop_shim.bat", "run_dev_client.bat"}


def is_junk(rel):
    rl = rel.replace("/", "\\")
    if rl.split("\\")[0].lower() in JUNK_DIR:
        return True
    base = os.path.basename(rl)
    if base.lower() in JUNK_BASE:
        return True
    if rl.lower() in JUNK_REL:
        return True
    if ".log" in base.lower() or ".orig" in base.lower() or ".patched" in base.lower():
        return True
    if set(base) <= set(".\"' "):          # stray dot/quote names
        return True
    if base.lower() in {"host"}:           # stray file the game drops from '-login Host'
        return True
    if "." not in base:                    # added payload files must be real .dll/.exe, not stray notes
        return True
    return False


def tree(root):
    d = {}
    for dp, dn, fn in os.walk(root):
        if os.path.basename(dp).lower() == "scratch":
            continue
        for f in fn:
            fp = os.path.join(dp, f)
            rel = os.path.relpath(fp, root).replace("/", "\\")
            try:
                sz = os.path.getsize(fp)
            except OSError:
                sz = -1
            d[rel.lower()] = (sz, rel, fp)
    return d


def md5(path):
    h = hashlib.md5()
    with open(path, "rb") as f:
        for blk in iter(lambda: f.read(1 << 20), b""):
            h.update(blk)
    return h.hexdigest()


INSTALL_PS1 = r'''# INSTALL.ps1 - personal FC3 co-op overlay. Copies payload/ onto a LEGITIMATE, freshly
# installed Far Cry 3 (Uplay) so it matches the working co-op install. NOT for distribution.
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$pay  = Join-Path $root "payload"
$game = Read-Host "Enter your Far Cry 3 install folder (the one with bin\farcry3_d3d11.exe)"
$game = $game.Trim('"')
if (-not (Test-Path (Join-Path $game "bin\farcry3_d3d11.exe"))) {
  Write-Host "ERROR: not a Far Cry 3 install folder." -ForegroundColor Red; exit 1
}
$bak = Join-Path $game ("_coop_backup_" + (Get-Date -Format "yyyyMMdd-HHmmss"))
$rows = @()
foreach ($l in (Get-Content (Join-Path $root "manifest.txt") -Encoding UTF8)) {
  if ($l -match '^rel path' -or $l -match '^FC3 co-op') { continue }
  $c = $l -split "`t"; if ($c.Count -ge 4) { $rows += [pscustomobject]@{ rel=$c[0]; md5=$c[2] } }
}
Write-Host "Backing up overwritten files into $bak ..." -ForegroundColor Yellow
foreach ($r in $rows) {
  $tgt = Join-Path $game ($r.rel -replace '/','\')
  if (Test-Path $tgt) { $dst = Join-Path $bak ($r.rel -replace '/','\'); New-Item -ItemType Directory -Force -Path (Split-Path $dst)|Out-Null; Copy-Item $tgt $dst -Force }
}
Write-Host "Overlaying payload ..." -ForegroundColor Yellow
$n = 0
foreach ($r in $rows) {
  $src = Join-Path $pay ($r.rel -replace '/','\'); if (-not (Test-Path $src)) { Write-Host "  MISSING $($r.rel)" -ForegroundColor Red; continue }
  $dst = Join-Path $game ($r.rel -replace '/','\'); New-Item -ItemType Directory -Force -Path (Split-Path $dst)|Out-Null; Copy-Item $src $dst -Force; $n++
}
$need = $rows | Where-Object { $_.rel -like '*FC3_d3d11.dll' } | Select-Object -First 1
if ($need) { $h=(Get-FileHash -Algorithm MD5 (Join-Path $game ($need.rel -replace '/','\'))).Hash.ToLower()
  if ($h -eq $need.md5) { Write-Host "Verified FC3_d3d11.dll md5 = $h (co-op patched, OK)" -ForegroundColor Green }
  else { Write-Host "WARN md5 mismatch; if Uplay repairs files, re-run INSTALL" -ForegroundColor Yellow } }
Write-Host "DONE ($n files). See docs\fc3-coop-radmin-setup.md / .ru.md for the Radmin 2-PC co-op (host + join via -client -ip)." -ForegroundColor Cyan
'''

README = '''# FC3 co-op 2nd-PC overlay package (PERSONAL USE ONLY - not for distribution)

Makes a second licensed Far Cry 3 (Uplay) install identical to the working co-op install.

INSTALL.bat / INSTALL.ps1 - point it at your game folder; it backs up touched files then
overlays payload/. payload/ = the files that differ from a clean install (modded 1.05-era binaries
already co-op-patched, loader, mod support files, data_win32\\patch.dat). tools/ = LAN helpers +
patch_dll.py. manifest.txt = size + MD5 of each payload file.

Steps (2nd PC): install from Uplay -> launch once -> quit -> run INSTALL.bat -> set up LAN co-op per
docs/fc3-coop-radmin-setup.md (Russian: docs/fc3-coop-radmin-setup.ru.md; join is via -client -ip,
there is no LAN browser). If Ubisoft later verifies/repairs files, re-run INSTALL.bat.
'''


def build():
    rt, mt = tree(REF), tree(MINE)
    payload = []
    for k, (sz, rel, fp) in mt.items():
        if is_junk(rel):
            continue
        if k not in rt:
            payload.append((rel, fp, "added"))
        elif rt[k][0] != sz:
            payload.append((rel, fp, "modified"))
    payload.sort()
    print("payload files:", len(payload))

    if os.path.isdir(PKG):
        shutil.rmtree(PKG)
    os.makedirs(PKG)
    pay = os.path.join(PKG, "payload")
    man = ["FC3 co-op overlay payload (personal use). md5 = MD5 of file as shipped.",
           "rel path\tsize\tmd5\tkind"]
    for rel, fp, kind in payload:
        dst = os.path.join(pay, rel)
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        shutil.copy2(fp, dst)
        man.append(f"{rel.replace(chr(92),'/')}\t{os.path.getsize(fp)}\t{md5(dst)}\t{kind}")
        print("  copied", rel)
    open(os.path.join(PKG, "manifest.txt"), "w", encoding="utf-8").write("\n".join(man) + "\n")

    for sub, names, src in (
        ("tools", ("patch_dll.py", "fc3_addrmap.py", "fc3_lan_radmin.bat", "fc3_lan_firewall.bat",
                   "fc3_lan_same_subnet.bat", "fc3_lan_host_only.bat", "fc3_host_engine_check.ps1",
                   "fc3_diag.ps1"),
         os.path.join(REPO, "scripts")),
        ("docs", ("fc3-coop-radmin-setup.md", "fc3-coop-radmin-setup.ru.md",
                  "fc3-coop-mod-setup.md", "fc3-coop-mod-setup.ru.md"),
         os.path.join(REPO, "docs")),
    ):
        d = os.path.join(PKG, sub); os.makedirs(d)
        for f in names:
            s = os.path.join(src, f)
            if os.path.exists(s):
                shutil.copy2(s, os.path.join(d, f))

    open(os.path.join(PKG, "INSTALL.ps1"), "w", encoding="utf-8").write(INSTALL_PS1)
    open(os.path.join(PKG, "INSTALL.bat"), "w", encoding="utf-8").write(
        '@echo off\r\npowershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0INSTALL.ps1"\r\npause\r\n')
    open(os.path.join(PKG, "README.md"), "wb").write(README.encode("utf-8"))

    if os.path.exists(ZIP):
        os.remove(ZIP)
    with zipfile.ZipFile(ZIP, "w") as z:
        for dp, dn, fn in os.walk(PKG):
            for f in fn:
                fp = os.path.join(dp, f)
                comp = zipfile.ZIP_STORED if f.lower() in ("patch.dat",) else zipfile.ZIP_DEFLATED
                z.write(fp, os.path.relpath(fp, PKG), compress_type=comp)
    print("built", PKG)
    print("zip", ZIP, os.path.getsize(ZIP), "bytes")


if __name__ == "__main__":
    build()
