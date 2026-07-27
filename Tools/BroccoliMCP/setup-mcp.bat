@echo off
setlocal

cd /d "%~dp0"

where winget >nul 2>nul
if errorlevel 1 (
  echo [ERROR] winget was not found.
  exit /b 1
)

echo [1/3] Installing or updating uv...
winget install --id=astral-sh.uv -e --accept-package-agreements --accept-source-agreements

rem A running shell does not automatically receive PATH changes made by winget.
set "PATH=%PATH%;%LOCALAPPDATA%\Microsoft\WinGet\Links;%USERPROFILE%\.local\bin"

set "UV_EXE="
for /f "delims=" %%I in ('where uv 2^>nul') do if not defined UV_EXE set "UV_EXE=%%I"

if not defined UV_EXE (
  for /f "delims=" %%I in ('where /r "%LOCALAPPDATA%\Microsoft\WinGet\Packages" uv.exe 2^>nul') do if not defined UV_EXE set "UV_EXE=%%I"
)

if not defined UV_EXE (
  echo [ERROR] uv.exe was not found after winget installation.
  echo Close this terminal, open a new one, and run this script again.
  exit /b 1
)

echo [2/3] Installing Python 3.14...
"%UV_EXE%" python install 3.14
if errorlevel 1 exit /b 1

echo [3/3] Synchronizing dependencies from uv.lock...
"%UV_EXE%" sync --python 3.14 --extra dev --frozen
if errorlevel 1 exit /b 1

echo MCP Bridge setup completed successfully.
exit /b 0
