@echo off
rem Release-invariant check: HologramLib is a pure dependency library and its official
rem artifact must not carry any diagnostic logging.
rem   1) macro polarity: with diagnostics off the marker must NOT reach the object file,
rem      with -DHOLOGLIB_DIAG_LOG=1 it MUST
rem   2) artifact invariant: the built DLL must contain no library log prefix
rem Usage: tests\check-no-diagnostics.bat [HologramLib.dll]
setlocal enabledelayedexpansion
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
set "OUT=%ROOT%\work\no-diag"
if not exist "%OUT%" mkdir "%OUT%"

set "DLL=%~1"
if not defined DLL set "DLL=%ROOT%\bin\HologramLib\HologramLib.dll"
if not exist "%DLL%" (echo DLL not found: %DLL% & exit /b 1)

rem locate the levilamina / fmt packages that actually have the headers
set "LLINC="
for /d %%d in ("%LOCALAPPDATA%\.xmake\packages\l\levilamina\26.40.0\*") do (
    if exist "%%d\include\ll\api\io\Logger.h" set "LLINC=%%d\include"
)
if not defined LLINC (echo levilamina include dir not found & exit /b 1)

set "FMTINC="
for /d %%a in ("%LOCALAPPDATA%\.xmake\packages\f\fmt\*") do (
    for /d %%b in ("%%a\*") do (
        if exist "%%b\include\fmt\base.h" set "FMTINC=%%b\include"
    )
)
if not defined FMTINC (echo fmt include dir not found & exit /b 1)

echo compiling diag off / on ...
cl /nologo /std:c++latest /EHsc /MD /utf-8 /W4 /c ^
   "/I%ROOT%\src" "/I%LLINC%" "/I%FMTINC%" ^
   "%~dp0diag_marker.cpp" "/Fo:%OUT%\off.obj"
if errorlevel 1 (echo diag-off compile failed. & exit /b 1)

cl /nologo /std:c++latest /EHsc /MD /utf-8 /W4 /c /DHOLOGLIB_DIAG_LOG=1 ^
   "/I%ROOT%\src" "/I%LLINC%" "/I%FMTINC%" ^
   "%~dp0diag_marker.cpp" "/Fo:%OUT%\on.obj"
if errorlevel 1 (echo diag-on compile failed. & exit /b 1)

python "%~dp0check-no-diagnostics.py" "%OUT%\off.obj" "%OUT%\on.obj" "%DLL%"
exit /b %errorlevel%
