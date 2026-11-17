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
    $mgr  = ($txt | Select-String -Pattern "MGR-WATCH" | Where-Object { $_.Line -notmatch "thread started" }).Count
    $login = ($txt | Select-String -Pattern "LOGIN-EV").Count
    Write-Host ""
    Write-Host "================ REAL-LOGIN RV-PROBE RESULT ================"
    Write-Host ("LOGIN-EV lines         : {0}" -f $login)
    Write-Host ("MGR-WATCH polls        : {0}" -f $mgr)
    Write-Host ("RV-PROBE install banner: {0}" -f $install)
    Write-Host ("RV-PROBE RUNTIME hits  : {0}" -f $probe)
    if ($probe -ge 1) {
        Write-Host ">>> FC3 ENTERED RendezVous on a real Ubisoft login (RV client reachable)."
    } else {
        Write-Host ">>> FC3 never entered RendezVous even with a real login (gate is above RV)."
    }
} else {
    Write-Host "coop_shim.log missing at $shim (observer never loaded - inconclusive run)."
}

foreach ($proc in @("farcry3_d3d11","farcry3")) { Get-Process $proc -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue }

$cmdPy = Get-Command python.exe -ErrorAction SilentlyContinue | Where-Object { $_.Source -notlike "*\WindowsApps\*" } | Select-Object -First 1
if (-not $cmdPy) { $cmdPy = Get-Command python.exe -ErrorAction SilentlyContinue | Select-Object -First 1 }
$Py = if ($cmdPy) { $cmdPy.Source } else { "python" }
& $Py -I "$Repo\scripts\deploy_proxy_only.py" restore $Game
Remove-Item (Join-Path $Bin "coop_shim.dll") -Force -ErrorAction SilentlyContinue
Write-Host "Restored clean loader on $Game (shim removed)."
