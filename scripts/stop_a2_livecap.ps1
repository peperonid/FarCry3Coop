# stop_a2_livecap.ps1 - stop the A2 live-capture harness and roll the game back to clean mod-262
# (NOTES 28di). Kills the test game, the rv_server/config_server python processes (by listening
# port), removes the shim, restores the real mod-262 loader. Idempotent.
#   powershell -NoProfile -ExecutionPolicy Bypass -File scripts\stop_a2_livecap.ps1

$Repo = Split-Path -Parent $PSScriptRoot
$Game = if ($env:GAME_DIR) { $env:GAME_DIR } else { "H:\FarCryCoop\TestFC3_105" }
$Bin  = Join-Path $Game "bin"

taskkill /F /IM farcry3_d3d11.exe 2>$null | Out-Null

# kill python processes that own :80 and :50000
$netstat = netstat -ano
foreach ($line in $netstat) {
    if ($line -match "LISTENING" -and $line -match ":50000\b" -or ($line -match "LISTENING" -and $line -match ":80\s")) {
        $pid2 = ($line -split '\s+')[-1]
        $owner = tasklist /FI "PID eq $pid2" /NH 2>$null | Out-String
        if ($owner -match "(?i)python") { taskkill /F /PID $pid2 2>$null | Out-Null }
    }
}

python -I "$Repo\scripts\deploy_proxy_only.py" restore
Remove-Item (Join-Path $Bin "coop_shim.dll") -Force -ErrorAction SilentlyContinue
Write-Host "Stopped and rolled back to clean mod-262 (proxy removed, shim removed, servers killed)."
