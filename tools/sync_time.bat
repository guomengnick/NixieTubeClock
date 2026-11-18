@echo off
rem Double-click to sync the nixie clock time over USB (logic is in sync_time.ps1)
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0sync_time.ps1" %*
pause
