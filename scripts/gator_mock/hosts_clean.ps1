# hosts_clean.ps1 - remove the leftover FC3 "redirect Ubisoft hosts -> 127.0.0.1" entries from the
# Windows hosts file (a capture leftover that silently blocks real online logins; NOTES 28f2 follow-up).
# The shipped hosts_patch.py `restore` is USELESS here because its own hosts.backup was captured AFTER
# the redirect was applied (it contains the same 4 lines), so we comment the entries out in place.
#
# SAFETY: this is a C:\Windows system file. Run from an ELEVATED PowerShell only. It:
#   1) copies the current hosts to H:\FarCryCoop\hosts.backup-<timestamp> first,
#   2) comments out (with #) only the FC3 ubi.com 127.0.0.1 redirect lines + the gator MARK,
#   3) ipconfig /flushdns, 4) prints any remaining ACTIVE ubi.com lines (should be none).
#
#   powershell -NoProfile -ExecutionPolicy Bypass -File scripts\gator_mock\hosts_clean.ps1

$ErrorActionPreference = "Stop"
$hosts = Join-Path $env:SystemRoot "System32\drivers\etc\hosts"
if (-not (Test-Path $hosts)) { throw "hosts not found at $hosts" }

$stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$bak = "H:\FarCryCoop\hosts.backup-$stamp"
Copy-Item $hosts $bak -Force
Write-Host ("[backup] current hosts -> {0}" -f $bak)

$lines = Get-Content $hosts
# an FC3 redirect line = a 127.0.0.1 (or 0.0.0.0) mapping to a *.ubi.com host, or our MARK comment
$new = foreach ($l in $lines) {
    if ($l -match "^\s*#") { $l; continue }                                  # already a comment
    if ($l -match "FC3Coop gator redirect") { "# " + $l.Trim(); continue }   # our MARK
    if ($l -match "^\s*127\.0\.0\.1\s+\S*\.ubi\.com" -or $l -match "^\s*0\.0\.0\.0\s+\S*\.ubi\.com") {
        "# [hosts_clean] " + $l.Trim()
    } else { $l }
}
Set-Content -Path $hosts -Value $new -Encoding UTF8
ipconfig /flushdns | Out-Null

Write-Host "[done] commented out the ubi.com redirect lines. Remaining ACTIVE ubi.com mappings:"
$rem = (Get-Content $hosts) | Where-Object { $_ -match "^\s*\S+\s+\S*\.ubi\.com" }
if ($rem) { $rem | ForEach-Object { Write-Host ("  " + $_.Trim()) } ; Write-Host ">>> still present - investigate" }
else { Write-Host "  (none) - hosts is clean for real online login" }
Write-Host ("full backup kept at: {0}" -f $bak)
