@echo off
REM 本地跑一次冻结契约检查（CI 上由 .github/workflows/abi.yml 跑同一条）
REM 用法: tools\check-abi.bat            对 abi\ 下的最新基线
REM       tools\check-abi.bat -H other.h 对指定的头文件
cd /d "%~dp0.."
set PYTHONIOENCODING=utf-8
python tools\abi_tool.py check %*
