# stop_d0_reallogin.ps1 - tear down run_d0_reallogin.ps1: kill the game, remove the shim, restore the
# clean install's genuine loader from bin\real. Then prints the RV-PROBE / MGR-WATCH summary from
# coop_shim.log so you can copy it back. NOTES 28f0/28f3.
#   powershell -NoProfile -ExecutionPolicy Bypass -File scripts\stop_d0_reallogin.ps1

$Repo = Split-Path -Parent $PSScriptRoot
$Game = if ($env:GAME_DIR) { $env:GAME_DIR } else { "H:\FarCryCoop\FC3_clean" }
$Bin  = Join-Path $Game "bin"
$shim = Join-Path $Bin "coop_shim.log"

# summarize BEFORE teardown so a fresh launch doesn't clear the evidence
if (Test-Path $shim) {
    $txt = Get-Content $shim
    $install = ($txt | Select-String -Pattern "RV-PROBE observers installed" -SimpleMatch).Count
    $probe = ($txt | Select-String -Pattern "RV-PROBE" | Where-Object { $_.Line -notmatch "observers installed" }).Count
    $enc  = ($txt | Select-String -Pattern "shim: RV-ENC #" | Where-Object { $_.Line -match "connId=" }).Count
    $drv  = ($txt | Select-String -Pattern "shim: RV-DRV1" | Where-Object { $_.Line -match "connId=" }).Count
    $mgr  = ($txt | Select-String -Pattern "MGR-WATCH" | Where-Object { $_.Line -notmatch "thread started" }).Count
    $login = ($txt | Select-String -Pattern "LOGIN-EV").Count
    Write-Host ""
    Write-Host "================ REAL-LOGIN RV RESULT ================"
    Write-Host ("LOGIN-EV lines         : {0}" -f $login)
    Write-Host ("MGR-WATCH polls        : {0}" -f $mgr)
    Write-Host ("RV-PROBE install banner: {0}" -f $install)
    Write-Host ("RV-PROBE RUNTIME hits  : {0}" -f $probe)
    Write-Host ("RV-ENC captures        : {0}" -f $enc)
    Write-Host ("RV-DRV captures        : {0}" -f $drv)
    if (($enc -ge 1) -or ($drv -ge 1) -or ($probe -ge 1)) {
        Write-Host ">>> FC3 ENTERED RendezVous on a real Ubisoft login (RV client reachable) AND RV packets were captured."
        Write-Host "    NOTE 28g4: RV-ENC sits at FUN_11214630, BELOW the prudps cipher - the captured buffer is already-encrypted (driver=0). The plaintext lives upstream in the Quazal packet-object layer."
    } else {
        Write-Host ">>> FC3 never entered RendezVous even with a real login (gate is above RV)."
    }
} else {
    Write-Host "coop_shim.log missing at $shim (observer never loaded - inconclusive run)."
}

# persist the log so a fresh launch can't clear the evidence, then run the offline extractor (keystream + MTP) on it
if (Test-Path $shim) {
    $keep = Join-Path $Repo "local\scratch\coop_shim_last.log"
    try { New-Item -ItemType Directory -Force (Split-Path $keep) | Out-Null; Copy-Item $shim $keep -Force; Write-Host ("[save] coop_shim.log -> {0}" -f $keep) } catch { Write-Host ("[save] failed: {0}" -f $_.Exception.Message) }
    $cmdPy2 = Get-Command python.exe -ErrorAction SilentlyContinue | Where-Object { $_.Source -notlike "*\WindowsApps\*" } | Select-Object -First 1
    $Py2 = if ($cmdPy2) { $cmdPy2.Source } else { "python" }
    try { & $Py2 -I (Join-Path $Repo "scripts\rv_capture\rv_enc_extract.py") $shim } catch { Write-Host ("[extract] failed: {0}" -f $_.Exception.Message) }
}

foreach ($proc in @("farcry3_d3d11","farcry3")) { Get-Process $proc -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue }
Start-Sleep -Seconds 2
$loader = Join-Path $Bin "uplay_r1_loader.dll"
$real   = Join-Path $Bin "real\uplay_r1_loader.dll"
if ((Test-Path $loader) -and (Test-Path $real) -and ((Get-Item $real).Length -ne (Get-Item $loader).Length)) {
    # the python restore can race a lingering handle; force-clear read-only + retry a copy of the genuine loader
    for ($i = 1; $i -le 4; $i++) {
        try { attrib -r -a -s -h "$loader" 2>$null; Copy-Item $real $loader -Force; break } catch { Start-Sleep -Seconds 2 }
    }
    if ((Get-Item $real).Length -ne (Get-Item $loader).Length) { Write-Host "WARNING: loader still not restored to genuine ($( (Get-Item $loader).Length ) vs $( (Get-Item $real).Length )); a process may hold it - close Ubisoft Connect and re-run restore." }
}

$cmdPy = Get-Command python.exe -ErrorAction SilentlyContinue | Where-Object { $_.Source -notlike "*\WindowsApps\*" } | Select-Object -First 1
if (-not $cmdPy) { $cmdPy = Get-Command python.exe -ErrorAction SilentlyContinue | Select-Object -First 1 }
$Py = if ($cmdPy) { $cmdPy.Source } else { "python" }
& $Py -I "$Repo\scripts\deploy_proxy_only.py" restore $Game
Remove-Item (Join-Path $Bin "coop_shim.dll") -Force -ErrorAction SilentlyContinue
Write-Host "Restored clean loader on $Game (shim removed)."
