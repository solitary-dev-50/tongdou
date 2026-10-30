@echo off
setlocal
cd /d "%~dp0"
if not exist ".env" (
  echo TongDou Server has not been initialized in this folder.
  pause
  exit /b 1
)
for /f "tokens=1,* delims==" %%A in (.env) do if /i "%%A"=="COMPOSE_PROJECT_NAME" set "PROJECT_NAME=%%B"
if not defined PROJECT_NAME set "PROJECT_NAME=tongdou-v010"
docker compose --env-file .env -p "%PROJECT_NAME%" down
if errorlevel 1 (
  echo ERROR - Failed to stop TongDou Server.
  pause
  exit /b 1
)
echo SUCCESS - TongDou Server stopped. Persistent data was kept.
pause

