@echo off
rem Virtual-container (GMLIB scheme) offline check:
rem   1. build every packet with the library's own src\container\ContainerPackets.h
rem   2. ContainerOpen is compared byte-for-byte against a real capture (pkt46_165803_011.bin)
rem   3. UpdateBlock / BlockActorData / ContainerClose are decoded field-by-field (26.40 wire format)
rem Usage: tests\check-container-packets.bat [captured pkt46_*.bin]
rem NOTE: keep this file ASCII-only -- cmd reads it in the ANSI codepage and multibyte
rem       characters in comments break line parsing.
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
set "OUT=%ROOT%\work\container-packets"
if not exist "%OUT%" mkdir "%OUT%"

rem default sample = the capture the fixture input was copied from (deterministic)
set "CAP=%~1"
if not defined CAP if exist "%ROOT%\..\..\logs\fullpkts\pkt46_165803_011.bin" set "CAP=%ROOT%\..\..\logs\fullpkts\pkt46_165803_011.bin"
if defined CAP echo capture: %CAP%

cl /nologo /std:c++latest /EHsc /MD /utf-8 /W4 ^
   "/I%ROOT%\src" "/I%ROOT%\include" "/I%PROTO%\include" ^
   "%~dp0container_packets_fixture.cpp" ^
   "/Fe:%OUT%\container_packets_fixture.exe" "/Fo:%OUT%\\" ^
   /link "%PROTO%\lib\Protocol.lib"
if errorlevel 1 (echo C++ container fixture compilation failed. & exit /b 1)

"%OUT%\container_packets_fixture.exe" "%OUT%"
if errorlevel 1 (echo container fixture failed. & exit /b 1)

if defined CAP (
    python "%~dp0check-container-packets.py" "%OUT%" "%CAP%"
) else (
    echo WARNING: no captured pkt46 sample found, skipping the byte-for-byte ContainerOpen check
    python "%~dp0check-container-packets.py" "%OUT%"
)
exit /b %errorlevel%
