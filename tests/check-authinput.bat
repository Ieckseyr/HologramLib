@echo off
rem Offline decode of the AuthInput capture (logs\authinput.hex) with the protocol library.
rem Usage: tests\check-authinput.bat   (reads the capture written by MeowTradeTest)
setlocal
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VCVARS="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do set "VCVARS=%%i\VC\Auxiliary\Build\vcvars64.bat"
if not defined VCVARS (echo vswhere could not locate a Visual Studio C++ toolset. & exit /b 1)
call "%VCVARS%" >nul 2>&1
set "ROOT=%~dp0.."
set "PROTO=%ROOT%\..\BedrockProtocol-main\install"
set "OUT=%ROOT%\work\authinput"
if not exist "%OUT%" mkdir "%OUT%"

python "%~dp0analyze-authinput.py" "%ROOT%\..\..\logs\authinput.hex" "%OUT%"
if errorlevel 1 (echo capture conversion failed. & exit /b 1)

cl /nologo /std:c++latest /EHsc /MD /utf-8 /W4 ^
   "/I%PROTO%\include" ^
   "%~dp0authinput_decode_fixture.cpp" ^
   "/Fe:%OUT%\authinput_decode.exe" "/Fo:%OUT%\\" ^
   /link "%PROTO%\lib\Protocol.lib"
if errorlevel 1 (echo decode fixture compilation failed. & exit /b 1)

"%OUT%\authinput_decode.exe" "%OUT%\packets.bin" > "%OUT%\decoded.txt"
if errorlevel 1 (echo decode failed. & exit /b 1)
echo decoded to %OUT%\decoded.txt
