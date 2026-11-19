# Read-only: did the FC3 Local/LAN HOST net engine come up?
# Run this AFTER you reached "Host" in-game. It lists UDP ports 9000-9005 owned
# by farcry3_d3d11.exe. Seeing 9000 (OnlineEnginePort) and/or 9004 (LanHostBroadcastPort)
# bound by the game PID = the Local/LAN net engine actually engaged (host started).
# No admin, no game patch. Ctrl+C to stop polling.
$ports = 9000..9005
$deadline = (Get-Date).AddSeconds(180)
Write-Host "Watching farcry3_d3d11.exe for UDP 9000-9005 (host/LAN engine). Ctrl+C to stop."
while ((Get-Date) -lt $deadline) {
    $procs = Get-Process farcry3_d3d11 -ErrorAction SilentlyContinue
    if ($procs) {
        $pids = $procs.Id
        $eps = Get-NetUDPEndpoint -ErrorAction SilentlyContinue |
               Where-Object { $pids -contains $_.OwningProcess -and $ports -contains $_.LocalPort }
        if ($eps) {
            Write-Host ("[{0}] LAN/HOST ENGINE UP - game bound:" -f (Get-Date -Format HH:mm:ss))
            $eps | Sort-Object LocalPort | ForEach-Object {
                Write-Host ("   PID {0}  UDP {1}:{2}" -f $_.OwningProcess, $_.LocalAddress, $_.LocalPort)
            }
            if ($eps.LocalPort -contains 9000 -or $eps.LocalPort -contains 9004) {
                Write-Host "VERDICT: PASS - the Local/LAN host engine is listening. Ready to test a client join."
            } else {
                Write-Host "VERDICT: PARTIAL - game has UDP sockets in 9000-9005 but not 9000/9004 yet. Keep the Host session open."
            }
            break
        } else {
            Write-Host ("[{0}] game running, engine ports not bound yet (still at menu? go to Co-op>Host)" -f (Get-Date -Format HH:mm:ss))
        }
    } else {
        Write-Host ("[{0}] farcry3_d3d11.exe not running yet" -f (Get-Date -Format HH:mm:ss))
    }
    Start-Sleep -Seconds 3
}
Write-Host "NOTE: if this stayed empty while a Host session was open, the CLI -host did NOT start the LAN engine on this build; use the in-game Host menu (mod-262) and re-check."
