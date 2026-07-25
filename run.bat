@echo off

rem 
set TARGET=%~1
if "%TARGET%"=="" set TARGET=Debug

set "EXE_PATH=%~dp0Publish\%TARGET%\Launcher.exe"

rem 
if exist "%EXE_PATH%" (
    echo [run] %EXE_PATH%
    start "" "%EXE_PATH%"
) else (
    echo [error] not found
    echo path: %EXE_PATH%
    exit /b 1
)

endlocal