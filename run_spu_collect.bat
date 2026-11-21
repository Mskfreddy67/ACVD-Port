@echo off
cd /d "%~dp0"
echo Launching ARMORED CORE: VERDICT DAY with SPU collection (--spu-collect)...
start "" "%~dp0local\build\release\acvd.exe" --disc "D:\ACVD_disc" --windowed --spu-collect --verbose-diag
