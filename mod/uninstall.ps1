# Removes DualSense for RDR2 and restores the mods install.ps1 moved away (latest backup).
# Usage: powershell -ExecutionPolicy Bypass -File uninstall.ps1 [-GameDir "..."] [-KeepSettings]
param([string]$GameDir = "", [switch]$KeepSettings)
$ErrorActionPreference = "Stop"

function Find-GameDir {
    $cands = @()
    try {
        $steam = (Get-ItemProperty "HKCU:\Software\Valve\Steam").SteamPath -replace "/", "\"
        $cands += Join-Path $steam "steamapps\common\Red Dead Redemption 2"
        $vdf = Join-Path $steam "steamapps\libraryfolders.vdf"
        if (Test-Path $vdf) {
            Select-String -Path $vdf -Pattern '"path"\s+"(.+)"' | ForEach-Object {
                $cands += Join-Path ($_.Matches[0].Groups[1].Value -replace "\\\\", "\") "steamapps\common\Red Dead Redemption 2"
            }
        }
    } catch {}
    foreach ($rk in "HKLM:\SOFTWARE\WOW6432Node\Rockstar Games\Red Dead Redemption 2", "HKLM:\SOFTWARE\Rockstar Games\Red Dead Redemption 2") {
        try {
            $p = (Get-ItemProperty $rk -ErrorAction Stop).InstallFolder
            if ($p) { $cands += $p }
        } catch {}
    }
    $cands += "C:\Program Files\Rockstar Games\Red Dead Redemption 2", "C:\Program Files\Epic Games\RedDeadRedemption2"
    return $cands | Where-Object { Test-Path (Join-Path $_ "RDR2.exe") } | Select-Object -First 1
}

if (-not $GameDir) { $GameDir = Find-GameDir }
if (-not $GameDir -or -not (Test-Path (Join-Path $GameDir "RDR2.exe"))) { throw "Red Dead Redemption 2 not found - pass -GameDir" }
if (Get-Process -Name RDR2 -ErrorAction SilentlyContinue) { throw "Close Red Dead Redemption 2 first." }
$asi = Join-Path $GameDir "DualSenseRDR2.asi"
if (Test-Path $asi) { Remove-Item $asi; Write-Host "Removed DualSenseRDR2.asi" }
$logFile = Join-Path $GameDir "DualSenseRDR2.log"
if (Test-Path $logFile) { Remove-Item $logFile }
if (-not $KeepSettings) {
    $ini = Join-Path $GameDir "DualSenseRDR2.ini"
    if (Test-Path $ini) { Remove-Item $ini }
    $data = Join-Path $GameDir "DualSenseRDR2"
    if (Test-Path $data) { Remove-Item $data -Recurse }
} else {
    $coreDll = Join-Path $GameDir "DualSenseRDR2\DualSenseCore.dll"
    if (Test-Path $coreDll) { Remove-Item $coreDll }
}
# the Settings page (LML): our folder and archive (another mod's copy is put back if we kept one)
$lml = Join-Path $GameDir "lml"
$page = Join-Path $lml "DualSenseRDR2"
if (Test-Path $page) { Remove-Item $page -Recurse; Write-Host "Removed the Settings page" }
# the Settings page archive of every released version (their installs have no .dualsense-hash marker)
$knownPages = @("A713B23C1B71F6B8367688A420AEBCA373150EF7B4B07A6171774D47342BFEC3",   # 1.0.0
                "565CBC61F881F034F17C44B40ABC762881A71788862B3D1FAF0289FE9F3B5DFE")   # 1.1.0 - 1.3.0
foreach ($a in @("replace\update_4\x64\data\ui\startup_data.rpf")) {
    $p = Join-Path $lml $a
    $expectedHash = ""
    if (Test-Path "$p.dualsense-hash") {
        $raw = Get-Content "$p.dualsense-hash" -Raw -ErrorAction SilentlyContinue
        if ($raw) { $expectedHash = $raw.Trim() }
        Remove-Item "$p.dualsense-hash"
    }
    $curHash = if (Test-Path $p) { (Get-FileHash $p -Algorithm SHA256).Hash } else { "" }
    if (Test-Path "$p.dualsense-backup") { Move-Item "$p.dualsense-backup" $p -Force; Write-Host "Restored $a" }
    elseif ($curHash -and (($curHash -in $knownPages) -or $curHash -eq $expectedHash)) {
        Remove-Item $p
        Write-Host "Removed $a"
    }
}

$root = Join-Path $GameDir "DualSenseRDR2_backup"
$last = Get-ChildItem $root -Directory -ErrorAction SilentlyContinue | Sort-Object Name | Select-Object -Last 1
if ($last) {
    $movedList = Join-Path $last.FullName "moved.txt"
    if (Test-Path $movedList) {
        Get-Content $movedList | Where-Object { $_.Trim() -ne "" } | ForEach-Object {
            $item = $_.Trim()
            $src = Join-Path $last.FullName $item
            if (Test-Path $src) {
                Move-Item $src (Join-Path $GameDir $item) -Force
                Write-Host "Restored $item"
            }
        }
    }
    Remove-Item $last.FullName -Recurse
    if ((Test-Path $root) -and -not (Get-ChildItem $root -ErrorAction SilentlyContinue)) { Remove-Item $root }
}
Write-Host "Done."
