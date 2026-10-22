@echo off
rem Whole-packet check for UpdateTrade (26.40 / protocol 2168):
rem builds the full packet with the C++ writer, then compares it field-by-field and
rem byte-for-byte against a real BDS capture (logs\fullpkts\pkt80_194951_007.bin).
rem Usage: tests\check-trade-packet.bat [captured pkt80_*.bin]
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
set "OUT=%ROOT%\work\trade-packet"
if not exist "%OUT%" mkdir "%OUT%"

rem default sample = the capture the fixture was derived from (deterministic)
set "CAP=%~1"
if not defined CAP if exist "%ROOT%\..\..\logs\fullpkts\pkt80_194951_007.bin" set "CAP=%ROOT%\..\..\logs\fullpkts\pkt80_194951_007.bin"
if not defined CAP (
    for /f "delims=" %%f in ('dir /b /o-d "%ROOT%\..\..\logs\fullpkts\pkt80_*.bin" 2^>nul') do (
        if not defined CAP set "CAP=%ROOT%\..\..\logs\fullpkts\%%f"
    )
)
if not defined CAP (echo no captured pkt80_*.bin found; pass one explicitly & exit /b 1)
echo capture: %CAP%

cl /nologo /std:c++latest /EHsc /MD /utf-8 /W4 ^
   "/I%ROOT%\src" "/I%PROTO%\include" ^
   "%~dp0trade_packet_fixture.cpp" ^
   "/Fe:%OUT%\trade_packet_fixture.exe" "/Fo:%OUT%\\" ^
   /link "%PROTO%\lib\Protocol.lib"
if errorlevel 1 (echo C++ packet fixture compilation failed. & exit /b 1)

"%OUT%\trade_packet_fixture.exe" "%OUT%\update_trade.bin"
if errorlevel 1 (echo packet fixture failed. & exit /b 1)

python "%~dp0check-trade-packet.py" "%CAP%" "%OUT%\update_trade.bin"
exit /b %errorlevel%
