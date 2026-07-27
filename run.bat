@echo off
setlocal

set "TARGET=Debug"
set "ARGS="

if /i "%~1"=="Debug" (
    set "TARGET=Debug"
    shift
) else if /i "%~1"=="Release" (
    set "TARGET=Release"
    shift
)

:loop
if "%~1"=="" goto continue
set "ARGS=%ARGS% %1"
shift
goto loop
:continue

set "EXE_PATH=%~dp0Publish\%TARGET%\Launcher.exe"

if exist "%EXE_PATH%" (
    echo [run] %EXE_PATH%%ARGS%
    start "" "%EXE_PATH%"%ARGS%
) else (
    echo [error] not found
    echo path: %EXE_PATH%
    exit /b 1
)

endlocal