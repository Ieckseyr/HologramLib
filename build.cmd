@echo off
REM 一条命令出两个产物：LeviLamina DLL + endstone wheel
REM 用法:
REM   build.cmd                      只构建
REM   build.cmd --deploy D:\srv\plugins   构建并把 wheel 丢进服务端
setlocal
cd /d "%~dp0"
python "%~dp0build.py" %*
if errorlevel 1 (
    echo.
    echo [build] 失败
    pause
    exit /b 1
)
endlocal
