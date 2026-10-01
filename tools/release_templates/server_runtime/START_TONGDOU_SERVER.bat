@echo off
setlocal
cd /d "%~dp0"
title TongDou Server v0.1.3-rc.1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\start_tongdou_server.ps1"
set "EXIT_CODE=%ERRORLEVEL%"
echo.
if "%EXIT_CODE%"=="0" (
  echo SUCCESS
) else (
  echo ERROR - TongDou Server did not start correctly.
)
echo.
pause
exit /b %EXIT_CODE%

