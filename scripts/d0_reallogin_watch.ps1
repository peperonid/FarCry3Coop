# d0_reallogin_watch.ps1 - PASSIVE control watcher for the "genuine Ubisoft Connect login" experiment
# (NOTES 28f0 follow-up to D0). This does NOT touch the game install and does NOT deploy proxy/shim.
# Run the game the NORMAL way (genuine Ubisoft Connect login, NO mods, NO our proxy/shim), then run
# THIS while attempting Multiplayer -> Co-op -> Host, and it records whether FC3 ever opens OUTBOUND
# connections to non-LAN (i.e. Ubisoft) servers. That is the discriminator:
#   - any PUBLIC remote endpoint during co-op  => FC3's RV/lobby client IS reachable; it dials Ubisoft
#     and the only blocker is the dead backend (so RV emulation would be theoretically meaningful).
#   - ZERO public remote endpoints             => the gate is above RendezVous even with a real login
#     (confirms NOTES 28dq/28f0); Strategy D is dead regardless of login fakeness.
#
# Usage (NO admin needed; netstat -ano is fine):
#   1) launch the game normally and log in with your real Ubisoft account,
#   2) start this:  powershell -NoProfile -ExecutionPolicy Bypass -File scripts\d0_reallogin_watch.ps1
#   3) in-game go Multiplayer -> Co-op -> Host and wait ~60s on whatever it shows,
#   4) the script runs for $DurationSec (default 90s) then prints a verdict; log: scripts\d0_reallogin_watch.log

$DurationSec = if ($args[0]) { [int]$args[0] } else { 90 }
$Repo = Split-Path -Parent $PSScriptRoot
$log = Join-Path $Repo "scripts\d0_reallogin_watch.log"
"=== real-login watch start $(Get-Date -Format 'HH:mm:ss') (duration ${DurationSec}s) ===" | Set-Content $log

$public = @{}
$private = @{}
$t0 = Get-Date
while (((Get-Date) - $t0).TotalSeconds -lt $DurationSec) {
    $procs = Get-Process farcry3_d3d11 -ErrorAction SilentlyContinue
    if ($procs) {
        foreach ($proc in $procs) {
            $pid2 = $proc.Id
            $conns = netstat -ano | Select-String -Pattern "\s$pid2\s*$"
            foreach ($c in $conns) {
                $line = $c.ToString().Trim()
                # last two tokens: proto local foreign state pid ; foreign is 3rd column
                $parts = ($line -split '\s+')
                if ($parts.Count -ge 4) {
                    $foreign = $parts[2]
                    if ($foreign -match "^([0-9]+(\.[0-9]+){3}):(\d+)$") {
                        $ip = $matches[1]; $port = $matches[2]
                        $isPrivate = ($ip -like "127.*" -or $ip -like "192.168.*" -or $ip -like "10.*" -or `
                                      $ip -like "169.254.*" -or $ip -match "^172\.(1[6-9]|2[0-9]|3[01])\.")
                        if ($isPrivate) { $private["$ip`:$port"] = $true } else { $public["$ip`:$port"] = $true }
                        $tag = if ($isPrivate) { "LAN " } else { "PUBLIC" }
                        "$(Get-Date -Format 'HH:mm:ss') [$tag] $line" | Add-Content $log
                    }
                }
            }
        }
    } else {
        "$(Get-Date -Format 'HH:mm:ss') (game not running)" | Add-Content $log
    }
    Start-Sleep -Seconds 2
}

Write-Host ""
Write-Host "================ REAL-LOGIN WATCH RESULT ================"
Write-Host ("distinct PUBLIC (non-LAN) remote endpoints seen : {0}" -f $public.Count)
Write-Host ("distinct LAN/private remote endpoints seen      : {0}" -f $private.Count)
if ($public.Count -gt 0) {
    Write-Host "PUBLIC remotes:"
    $public.Keys | Sort-Object | Select-Object -First 30 | ForEach-Object { Write-Host ("  "+$_) }
    Write-Host ""
    Write-Host ">>> FC3 DID dial external (Ubisoft) servers. The RV/lobby client is REACHABLE;"
    Write-Host "    the blocker is the DEAD backend, not the code path. (RV emulation would be"
    Write-Host "    theoretically meaningful, but needs every Ubisoft service the handshake wants.)"
} else {
    Write-Host "LAN/private remotes:"
    $private.Keys | Sort-Object | Select-Object -First 30 | ForEach-Object { Write-Host ("  "+$_) }
    Write-Host ""
    Write-Host ">>> ZERO external dial even with a real login => the gate is ABOVE RendezVous"
    Write-Host "    (NOTES 28dq/28f0). Strategy D is dead regardless of credential fakeness."
}
Write-Host ("full log: {0}" -f $log)
