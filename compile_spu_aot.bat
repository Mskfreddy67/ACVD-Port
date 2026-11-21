@echo off
cd /d "%~dp0"
echo Recompiling SPU AOT...
powershell -ExecutionPolicy Bypass -File "%~dp0tools\spu_aot.ps1" -Config Release
pause
