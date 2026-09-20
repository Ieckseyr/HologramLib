@echo off
rem Byte-level check for the synthetic NPC carrier packets (26.40 / protocol 2168).
rem Compiles tests\npc_carrier_fixture.cpp, runs it, then decodes the produced bytes and
rem cross-checks them against a real BDS NPC spawn capture (logs\fullpkts\pkt13_*.bin).
rem Usage: tests\check-npc-carrier.bat [captured pkt13_*.bin]
setlocal
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VCVARS="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do set "VCVARS=%%i\VC\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS (
    echo vswhere could not locate a Visual Studio C++ toolset.
    exit /b 1
)
call "%VCVARS%" >nul 2>&1
if errorlevel 1 (echo vcvars64 failed: %VCVARS% & exit /b 1)

set "ROOT=%~dp0.."
set "PROTO=%ROOT%\..\BedrockProtocol-main\install"
set "OUT=%ROOT%\work\npc-carrier"
if not exist "%OUT%" mkdir "%OUT%"

rem default sample = largest captured pkt13_*.bin (the intact 2.8KB NPC spawn)
set "CAP=%~1"
if not defined CAP (
    for /f "delims=" %%f in ('dir /b /o-s "%ROOT%\..\..\logs\fullpkts\pkt13_*.bin" 2^>nul') do (
        if not defined CAP set "CAP=%ROOT%\..\..\logs\fullpkts\%%f"
    )
)

cl /nologo /std:c++latest /EHsc /MD /utf-8 /W4 ^
   "/I%ROOT%\src" "/I%PROTO%\include" ^
   "%~dp0npc_carrier_fixture.cpp" ^
   "/Fe:%OUT%\npc_carrier_fixture.exe" "/Fo:%OUT%\\" ^
   /link "%PROTO%\lib\Protocol.lib"
if errorlevel 1 (echo C++ carrier fixture compilation failed. & exit /b 1)

"%OUT%\npc_carrier_fixture.exe" "%OUT%\addactor.bin" "%OUT%\npcdialogue.bin"
if errorlevel 1 (echo carrier fixture failed. & exit /b 1)

if defined CAP (python "%~dp0check-npc-carrier.py" "%OUT%\addactor.bin" "%OUT%\npcdialogue.bin" "%CAP%") else (python "%~dp0check-npc-carrier.py" "%OUT%\addactor.bin" "%OUT%\npcdialogue.bin")
exit /b %errorlevel%
