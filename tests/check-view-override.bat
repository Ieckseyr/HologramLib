@echo off
rem View-override offline check (1.25.0):
rem   compiles tests\view_override_fixture.cpp against the library's own
rem   src\view\ViewOverrideLogic.h (no BDS / Protocol.lib needed) and runs it.
rem   Cases: block-pos packing round-trips (incl. negative / 26-bit bounds),
rem   chunk ownership, dimension filter, idempotence of the packet rewrite,
rem   nametag metadata plan, type-name normalisation, packet-id constants.
rem Usage: tests\check-view-override.bat
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
set "OUT=%ROOT%\work\view-override"
if not exist "%OUT%" mkdir "%OUT%"

cl /nologo /std:c++latest /EHsc /MD /utf-8 /W4 ^
   "/I%ROOT%\src" "/I%ROOT%\include" ^
   "%~dp0view_override_fixture.cpp" ^
   "/Fe:%OUT%\view_override_fixture.exe" "/Fo:%OUT%\\"
if errorlevel 1 (echo fixture compilation failed. & exit /b 1)

"%OUT%\view_override_fixture.exe"
exit /b %errorlevel%
