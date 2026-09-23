@echo off
echo =======================================================================
echo          GHOSTFRAME - Stream Capture Cloaker Auto Build Script
echo =======================================================================

rem Terminate running instances so files are not locked
taskkill /f /im ghostframe-gui.exe >nul 2>&1
taskkill /f /im ghostframe-test.exe >nul 2>&1
taskkill /f /im ghostframe-helper64.exe >nul 2>&1
taskkill /f /im hidder-gui.exe >nul 2>&1
taskkill /f /im hidder-test.exe >nul 2>&1
taskkill /f /im hidder-helper64.exe >nul 2>&1

rem Ensure Dear ImGui is downloaded
if not exist "third_party\imgui\imgui.h" (
    echo [*] Downloading Dear ImGui library...
    if not exist "third_party" mkdir "third_party"
    powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri 'https://github.com/ocornut/imgui/archive/refs/heads/master.zip' -OutFile 'third_party\imgui.zip'; Expand-Archive -Path 'third_party\imgui.zip' -DestinationPath 'third_party' -Force; if (Test-Path 'third_party\imgui-master') { if (Test-Path 'third_party\imgui') { Remove-Item 'third_party\imgui' -Recurse -Force }; Move-Item 'third_party\imgui-master' 'third_party\imgui' -Force }; Remove-Item 'third_party\imgui.zip' -Force"
)

echo [1/3] Initializing MSVC x64 Environment...
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

set "PATH=C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;%PATH%"

cd /d "%~dp0"

echo.
echo [2/3] Configuring CMake project...
cmake -B build -S .

echo.
echo [3/3] Compiling Release Binaries...
cmake --build build --config Release

echo.
echo =======================================================================
echo  BUILD COMPLETE!
echo  To launch GhostFrame:
echo    .\build\bin\Release\ghostframe-gui.exe
echo =======================================================================
