@echo off
cd /d "%~dp0"
echo Launching ARMORED CORE: VERDICT DAY (Windowed Mode)...
start "" "%~dp0local\build\release\acvd.exe" --disc "D:\ACVD_disc" --windowed
