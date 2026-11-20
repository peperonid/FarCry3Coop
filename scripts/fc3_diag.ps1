# fc3_diag.ps1 - diagnose why the FC3 co-op overlay crashes on a 2nd PC.
# Run on the 2nd PC AFTER the crash (normal or admin PowerShell). Paste the path
# to the Far Cry 3 install folder when asked, then send the printed output back.
$ErrorActionPreference = "SilentlyContinue"
$game = Read-Host "Far Cry 3 install folder (the one with bin\farcry3_d3d11.exe)"
$game = $game.Trim('"')
Write-Host "`n=== game folder: $game ==="

function MD5($p){ if(Test-Path $p){ (Get-FileHash -Algorithm MD5 $p).Hash.Substring(0,8).ToLower() } else { "MISSING" } }

# expected values from the KNOWN-GOOD working install (first 8 of MD5)
$expExec = @{
  "FC3_d3d11.dll"="03ff4a9b"; "FC3.dll"="5ff04533"; "farcry3.exe"="3cf63ffd";
  "farcry3_d3d11.exe"="df7fc8a3"; "uplay_r1_loader.dll"="dacd14c0";
  "ubiorbitapi_r2_loader.dll"="ba6015e5"; "upc_r1_loader.dll"="82baf960"; "pbsvc_fc3.exe"="8fbe6d6d"
}
# expected data_win32 sizes (bytes) - catch a different downloaded game version
$expData = @{
  "common.dat"=241898794; "common.fat"=62012; "ige.dat"=432239930; "ige.fat"=275332;
  "sound.dat"=429372130; "sound.fat"=8692; "shadersobj.dat"=71791330; "shadersobj.fat"=691152;
  "patch.dat"=212002032; "patch.fat"=8652
}

Write-Host "`n=== overlay applied? (_coop_backup_*) ==="
$bak = Get-ChildItem $game -Directory -Filter "_coop_backup_*" | Select-Object -First 1
if ($bak) { Write-Host "  YES: $($bak.Name)" } else { Write-Host "  NO - INSTALL.bat was not run on this folder!" }

Write-Host "`n=== bin executables/loaders MD5 (expected = working install) ==="
foreach ($k in $expExec.Keys) {
  $h = MD5 (Join-Path $game "bin\$k")
  $st = if ($h -eq $expExec[$k]) { "OK" } else { "*** MISMATCH ***" }
  Write-Host ("  {0,-26} {1}  {2}" -f $k, $h, $st)
}

Write-Host "`n=== data_win32 sizes (expected = working install; mismatch => different game version) ==="
foreach ($k in $expData.Keys) {
  $p = Join-Path $game "data_win32\$k"
  $s = if (Test-Path $p) { (Get-Item $p).Length } else { "MISSING" }
  $st = if ($s -eq $expData[$k]) { "OK" } else { "*** MISMATCH ***" }
  Write-Host ("  {0,-20} {1,-12} (exp {2,-12}) {3}" -f $k, $s, $expData[$k], $st)
}

Write-Host "`n=== key runtime DLLs present in bin (mod dependencies) ==="
foreach ($d in "d3dx9_43.dll","d3dcsx_43.dll","XAudio2_7.dll","xinput1_3.dll","MSVCR100.dll","MSVCP100.dll","MSVCR110.dll","MSVCP110.dll","MSVCR120.dll","vcruntime140.dll","mfc100.dll") {
  $b = Test-Path (Join-Path $game "bin\$d")
  $s = Test-Path (Join-Path "$env:SystemRoot\System32" $d)
  $so = Test-Path (Join-Path "$env:SystemRoot\SysWOW64" $d)
  Write-Host ("  {0,-18} bin={1} SysWOW64={2} System32={3}" -f $d, $b, $so, $s)
}

Write-Host "`n=== game version markers ==="
$exe = Join-Path $game "bin\farcry3_d3d11.exe"
if (Test-Path $exe) { (Get-Item $exe).VersionInfo | Select-Object FileVersion,ProductVersion | Format-List }
$man = Join-Path $game "uplay_install.manifest"
if (Test-Path $man) { Write-Host ("  uplay_install.manifest size: " + (Get-Item $man).Length + " bytes (binary; not dumped)") }

Write-Host "`n=== recent Application Error / Hang events for farcry3 (faulting module tells the cause) ==="
Get-WinEvent -FilterHashtable @{LogName='Application'; ProviderName='Application Error','Application Hang','Application Hang','Windows Error Reporting'} -MaxEvents 40 |
  Where-Object { $_.Message -match 'farcry3|FC3' } | Select-Object -First 6 | ForEach-Object {
    Write-Host "---- $($_.TimeCreated) [$($_.Provider.Name)] ----"
    ($_.Message -split "`n" | Select-Object -First 12) | ForEach-Object { Write-Host "  $_" }
  }

Write-Host "`n=== FC3 crash logs / dumps ==="
$doc = Join-Path $([Environment]::GetFolderPath('MyDocuments')) "My Games\Far Cry 3"
Get-ChildItem $doc -Recurse -Include *.log,*.dmp -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending | Select-Object -First 5 | ForEach-Object {
  Write-Host "  $($_.FullName)  $($_.LastWriteTime)  $($_.Length) B"
}
Get-ChildItem (Join-Path $game "bin") -Include *.dmp -Recurse -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending | Select-Object -First 3 | ForEach-Object { Write-Host "  $($_.FullName)  $($_.LastWriteTime)" }
Write-Host "`n=== done. Send this whole output back ==="
