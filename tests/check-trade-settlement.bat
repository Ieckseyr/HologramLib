@echo off
rem Trade-settlement offline check (1.24.0):
rem   compiles tests\trade_settlement_fixture.cpp against the library's own
rem   src\trade\TradeSettlementLogic.h (no BDS / Protocol.lib needed) and runs it.
rem   Cases: action classification (pay-in / pay-out / result-out / deal) and the
rem   held-payment ledger coverage (exact / split / short / wrong-type / excess / free).
rem Usage: tests\check-trade-settlement.bat
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
set "OUT=%ROOT%\work\trade-settlement"
if not exist "%OUT%" mkdir "%OUT%"

cl /nologo /std:c++latest /EHsc /MD /utf-8 /W4 ^
   "/I%ROOT%\src" "/I%ROOT%\include" ^
   "%~dp0trade_settlement_fixture.cpp" ^
   "/Fe:%OUT%\trade_settlement_fixture.exe" "/Fo:%OUT%\\"
if errorlevel 1 (echo fixture compilation failed. & exit /b 1)

"%OUT%\trade_settlement_fixture.exe"
exit /b %errorlevel%
