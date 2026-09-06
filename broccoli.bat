@echo off
setlocal

pushd "%~dp0"
uv run --project "Tools\Build" --frozen python -m broccoli_build %*
set "ExitCode=%ERRORLEVEL%"
popd

exit /b %ExitCode%
