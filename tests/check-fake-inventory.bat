@echo off
rem Fake-inventory wire-format check against REAL BDS captures:
rem   the item descriptor's User Data uses a DIFFERENT NBT dialect (u16 name lengths /
rem   4-byte ints / u32 list counts) than sculk's CompoundTag (the varint dialect used by
rem   the trade offers and block-actor NBT). Hand-writing descriptors therefore writes the
rem   wrong bytes, which froze the client during the first test - so the library now lets
rem   BDS serialize the items (src\fakeinv\FakeInventoryPackets.h) and this check pins the
rem   layout facts it relies on (container id 0, 36 slots, container-name byte 0, empty
rem   storage item, LE-dialect user data fully consumed, no leftover bytes).
rem Usage: tests\check-fake-inventory.bat [captureDir] [pkt49.bin] [pkt50.bin]
rem NOTE: keep this file ASCII-only -- cmd reads it in the ANSI codepage and multibyte
rem       characters in comments break line parsing.
setlocal
set "ROOT=%~dp0.."
set "CAPDIR=%~1"
if not defined CAPDIR set "CAPDIR=%ROOT%\..\..\logs\fullpkts"
if not exist "%CAPDIR%" (echo capture dir not found: %CAPDIR% & exit /b 1)

python "%~dp0check-fake-inventory.py" "%CAPDIR%" "%~2" "%~3"
exit /b %errorlevel%
