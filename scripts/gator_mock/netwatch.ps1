# netwatch.ps1 - log every TCP/UDP endpoint + DNS name the FC3 game process touches, so we learn
# which host it dials AFTER it gets the online-config (the LoggingIn / RV step). Run in a SECOND
# normal PowerShell window (no admin needed - it watches your own session). Ctrl+C to stop.
#   powershell -ExecutionPolicy Bypass -File netwatch.ps1
$ErrorActionPreference = "SilentlyContinue"
$out = Join-Path $PSScriptRoot "netwatch_log.txt"
"== netwatch start $((Get-Date)) ==" | Out-File -Append -Encoding utf8 $out
$seen = @{}
while ($true) {
  $procs = Get-Process farcry3, farcry3_d3d11 -EA SilentlyContinue
  foreach ($p in $procs) {
    foreach ($c in (Get-NetTCPConnection -OwningProcess $p.Id -EA SilentlyContinue)) {
      $key = "TCP|$($c.State)|$($c.LocalPort)|$($c.RemoteAddress)|$($c.RemotePort)"
      if (-not $seen[$key]) {
        $seen[$key] = 1
        $name = (Resolve-DnsName $c.RemoteAddress -Type PTR -DnsOnly -EA SilentlyContinue).NameHost
        "[TCP] $($c.State) local:$($c.LocalPort) remote:$($c.RemoteAddress):$($c.RemotePort) ptr=$name t=$([int]((Get-Date) -f [DateTime]'2026-01-01').TotalSeconds)" | Out-File -Append -Encoding utf8 $out
      }
    }
    foreach ($u in (Get-NetUDPEndpoint -OwningProcess $p.Id -EA SilentlyContinue)) {
      $key = "UDP|$($u.LocalAddress)|$($u.LocalPort)"
      if (-not $seen[$key]) {
        $seen[$key] = 1
        "[UDP-bind] local:$($u.LocalAddress):$($u.LocalPort)" | Out-File -Append -Encoding utf8 $out
      }
    }
  }
  foreach ($r in (Get-DnsClientCache -EA SilentlyContinue | Where-Object { $_.Name -like "*ubi*" -or $_.Name -like "*quazal*" })) {
    $key = "DNS|$($r.Name)|$($r.Data)"
    if (-not $seen[$key]) {
      $seen[$key] = 1
      "[DNS] $($r.Name) -> $($r.Data)" | Out-File -Append -Encoding utf8 $out
    }
  }
  Start-Sleep -Milliseconds 400
}
