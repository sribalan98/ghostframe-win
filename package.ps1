$ErrorActionPreference = "Stop"

Write-Host "=======================================================================" -ForegroundColor Cyan
Write-Host "         GHOSTFRAME - Packaging & Installer Build Script               " -ForegroundColor Cyan
Write-Host "=======================================================================" -ForegroundColor Cyan

# 1. First run the build script to ensure Release binaries exist
$buildScript = Join-Path $PSScriptRoot "build.ps1"
Write-Host "`n[*] Verifying release compilation..." -ForegroundColor Yellow
& $buildScript

$releaseDir = Join-Path $PSScriptRoot "build\bin\Release"
$guiExe = Join-Path $releaseDir "ghostframe-gui.exe"
$helperExe = Join-Path $releaseDir "ghostframe-helper64.exe"
$payloadDll = Join-Path $releaseDir "ghostframe-payload64.dll"

if (-not (Test-Path $guiExe) -or -not (Test-Path $helperExe) -or -not (Test-Path $payloadDll)) {
    Write-Host "[ERROR] Compiled binaries missing in $releaseDir. Build failed." -ForegroundColor Red
    exit 1
}

# 2. Prepare Distribution Directory
$distDir = Join-Path $PSScriptRoot "dist"
$packageDir = Join-Path $distDir "GhostFrame-v1.0-win64"

if (Test-Path $packageDir) {
    Remove-Item -Path $packageDir -Recurse -Force
}
New-Item -ItemType Directory -Path $packageDir -Force | Out-Null

Write-Host "`n[*] Bundling files into $packageDir..." -ForegroundColor Cyan
Copy-Item $guiExe -Destination $packageDir -Force
Copy-Item $helperExe -Destination $packageDir -Force
Copy-Item $payloadDll -Destination $packageDir -Force

$testExe = Join-Path $releaseDir "ghostframe-test.exe"
if (Test-Path $testExe) {
    Copy-Item $testExe -Destination $packageDir -Force
}

$readme = Join-Path $PSScriptRoot "README.md"
if (Test-Path $readme) {
    Copy-Item $readme -Destination $packageDir -Force
}

$resourcesDir = Join-Path $PSScriptRoot "resources"
if (Test-Path $resourcesDir) {
    Copy-Item $resourcesDir -Destination $packageDir -Recurse -Force
}

# 3. Create Portable ZIP Archive
$zipFile = Join-Path $distDir "GhostFrame-v1.0-win64.zip"
if (Test-Path $zipFile) {
    Remove-Item $zipFile -Force
}

Write-Host "[*] Creating portable ZIP archive: $zipFile..." -ForegroundColor Cyan
Compress-Archive -Path "$packageDir\*" -DestinationPath $zipFile -Force
Write-Host "[+] Portable ZIP generated successfully!" -ForegroundColor Green

# 4. Check for Inno Setup Compiler (ISCC.exe)
$isccCandidates = @(
    "C:\Program Files (x86)\Inno Setup 6\ISCC.exe",
    "C:\Program Files\Inno Setup 6\ISCC.exe",
    "${env:LOCALAPPDATA}\Programs\Inno Setup 6\ISCC.exe"
)

$isccPath = $null
foreach ($cand in $isccCandidates) {
    if (Test-Path $cand) {
        $isccPath = $cand
        break
    }
}

if (-not $isccPath) {
    $cmd = Get-Command "ISCC.exe" -ErrorAction SilentlyContinue
    if ($cmd) {
        $isccPath = $cmd.Source
    }
}

if ($isccPath) {
    Write-Host "`n[*] Found Inno Setup Compiler at: $isccPath" -ForegroundColor Green
    Write-Host "[*] Compiling GhostFrame_Setup_v1.0.exe installer..." -ForegroundColor Cyan
    $issFile = Join-Path $PSScriptRoot "installer\GhostFrame_Setup.iss"
    & "$isccPath" "$issFile"
    if ($LASTEXITCODE -eq 0) {
        Write-Host "`n=======================================================================" -ForegroundColor Green
        Write-Host " INSTALLER GENERATED SUCCESSFULLY!" -ForegroundColor Green
        Write-Host "=======================================================================" -ForegroundColor Green
        Write-Host " Setup File: dist\GhostFrame_Setup_v1.0.exe" -ForegroundColor White
        Write-Host " Portable ZIP: dist\GhostFrame-v1.0-win64.zip`n" -ForegroundColor White
    }
    else {
        Write-Host "[WARNING] ISCC compilation failed." -ForegroundColor Yellow
    }
}
else {
    Write-Host "`n=======================================================================" -ForegroundColor Yellow
    Write-Host " PACKAGING COMPLETE (PORTABLE READY)" -ForegroundColor Yellow
    Write-Host "=======================================================================" -ForegroundColor Yellow
    Write-Host " Portable Bundle: dist\GhostFrame-v1.0-win64\" -ForegroundColor White
    Write-Host " Portable ZIP:    dist\GhostFrame-v1.0-win64.zip" -ForegroundColor White
    Write-Host "`n [*] Note: To generate the single-file Windows installer (GhostFrame_Setup_v1.0.exe):" -ForegroundColor Cyan
    Write-Host "     Download free Inno Setup 6 (https://jrsoftware.org/isdl.php) and compile:" -ForegroundColor White
    Write-Host "     installer\GhostFrame_Setup.iss`n" -ForegroundColor White
}
