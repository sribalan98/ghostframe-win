$ErrorActionPreference = "Stop"

Write-Host "=======================================================================" -ForegroundColor Cyan
Write-Host "         GHOSTFRAME - Stream Capture Cloaker Build Script              " -ForegroundColor Cyan
Write-Host "=======================================================================" -ForegroundColor Cyan

# 0. Close any currently running instances so DLLs/EXEs are not locked by Windows
Get-Process -Name "ghostframe-gui", "ghostframe-test", "ghostframe-helper64", "hidder-gui", "hidder-test", "hidder-helper64", "notepad", "WidgetBoard" -ErrorAction SilentlyContinue | Stop-Process -Force

# Release file lock on ghostframe-payload64.dll by terminating any remaining process that loaded it
Get-Process -ErrorAction SilentlyContinue | ForEach-Object {
    try {
        $locked = $_.Modules | Where-Object { $_.FileName -like "*ghostframe-payload64.dll*" -or $_.FileName -like "*hidder-payload64.dll*" }
        if ($locked) {
            Write-Host "[*] Releasing DLL lock from: $($_.ProcessName) (PID: $($_.Id))" -ForegroundColor Yellow
            Stop-Process -Id $_.Id -Force -ErrorAction SilentlyContinue
        }
    } catch {}
}

$vsVcVars = "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
$cmakeDir = "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"

if (-not (Test-Path $vsVcVars)) {
    Write-Host "[ERROR] Could not find vcvars64.bat at $vsVcVars" -ForegroundColor Red
    exit 1
}

# 1. Download Dear ImGui if not already present
$imguiDir = Join-Path $PSScriptRoot "third_party\imgui"
if (-not (Test-Path (Join-Path $imguiDir "imgui.h"))) {
    Write-Host "`n[*] Downloading Dear ImGui library..." -ForegroundColor Cyan
    New-Item -ItemType Directory -Path (Join-Path $PSScriptRoot "third_party") -Force | Out-Null
    $zipPath = Join-Path $PSScriptRoot "third_party\imgui.zip"
    
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    Invoke-WebRequest -Uri "https://github.com/ocornut/imgui/archive/refs/heads/master.zip" -OutFile $zipPath
    
    Write-Host "[*] Extracting Dear ImGui..." -ForegroundColor Cyan
    Expand-Archive -Path $zipPath -DestinationPath (Join-Path $PSScriptRoot "third_party") -Force
    
    $extractedDir = Join-Path $PSScriptRoot "third_party\imgui-master"
    if (Test-Path $extractedDir) {
        if (Test-Path $imguiDir) { Remove-Item $imguiDir -Recurse -Force }
        Move-Item $extractedDir $imguiDir -Force
    }
    Remove-Item $zipPath -Force
    Write-Host "[+] Dear ImGui ready at $imguiDir" -ForegroundColor Green
}

# 2. Add CMake to PATH only if not already present
if ($env:PATH -notlike "*$cmakeDir*") {
    $env:PATH = "$cmakeDir;$env:PATH"
}

# 3. Import MSVC environment variables only if cl.exe is not already in session
if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    Write-Host "`n[1/3] Initializing MSVC x64 Environment..." -ForegroundColor Yellow
    $tempFile = [System.IO.Path]::GetTempFileName()
    cmd.exe /c "call `"$vsVcVars`" && set" > $tempFile
    Get-Content $tempFile | ForEach-Object {
        if ($_ -match '^(.*?)=(.*)$') {
            Set-Item -Force -Path "env:\$($matches[1])" -Value $matches[2]
        }
    }
    Remove-Item $tempFile -Force
} else {
    Write-Host "`n[1/3] MSVC Environment already active." -ForegroundColor Green
}

# 1.5 Generate multi-resolution Windows ICO from PNG icon if needed
$iconPng = Join-Path $PSScriptRoot "src\icon\ghostframe.png"
$iconIco = Join-Path $PSScriptRoot "resources\ghostframe.ico"
$convertScript = Join-Path $PSScriptRoot "scripts\convert_icon.ps1"

if (Test-Path $iconPng) {
    if (-not (Test-Path $iconIco) -or ((Get-Item $iconPng).LastWriteTime -gt (Get-Item $iconIco).LastWriteTime)) {
        Write-Host "`n[*] Generating application icon from $iconPng..." -ForegroundColor Cyan
        & $convertScript -SourcePng $iconPng -DestIco $iconIco
    }
}

Set-Location $PSScriptRoot

Write-Host "`n[2/3] Configuring CMake project..." -ForegroundColor Yellow
& "$cmakeDir\cmake.exe" -B build -S .

if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] CMake configuration failed." -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host "`n[3/3] Compiling Release Binaries..." -ForegroundColor Yellow
& "$cmakeDir\cmake.exe" --build build --config Release

if ($LASTEXITCODE -eq 0) {
    Write-Host "`n=======================================================================" -ForegroundColor Green
    Write-Host " BUILD SUCCESSFUL!" -ForegroundColor Green
    Write-Host "=======================================================================" -ForegroundColor Green
    Write-Host " Binaries generated in build\bin\Release\`n"
    Write-Host " To run the GhostFrame GUI application, execute:" -ForegroundColor Cyan
    Write-Host "   .\build\bin\Release\ghostframe-gui.exe`n" -ForegroundColor White
} else {
    Write-Host "`n[ERROR] Compilation failed." -ForegroundColor Red
}
