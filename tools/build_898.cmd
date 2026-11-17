@echo off
REM 1.21.132.3 / 协议 898 线构建脚本（离线）
REM   LeviLamina 1.9.7 SDK  <- .llsdk/liteldev-repo 的 on_fetch 指向本机 xmake 包缓存
REM   bedrock-protocol v898 <- .llsdk/endstone-proto/install
REM   prelink v0.7.1 + bedrockdata v1.21.132-server.8 <- 项目内联规则自动定位
chcp 65001 >nul
setlocal

cd /d "%~dp0.."

set "HOLO_LL_VERSION=1.9.7"
set "XMAKE_GLOBALDIR=%~dp0..\..\.xmake_g"

if not exist "%XMAKE_GLOBALDIR%\\.xmake\\xmake.conf" (
    echo [warn] 没有找到 %XMAKE_GLOBALDIR%\\xmake.conf —— 见 docs/internal/回移-898.md 的「离线构建」
)

xmake f -c -y --target_type=server -p windows -a x64 -m release || goto :fail
xmake build -j 16 || goto :fail

echo.
echo [ok] HologramLib.dll 已生成并部署到 %~dp0
exit /b 0

:fail
echo.
echo [fail] 构建失败，见上面的输出
exit /b 1
