@echo off
setlocal

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Debug"

if /i "%CONFIG%"=="Debug" set "BUILD_PRESET=local-debug"
if /i "%CONFIG%"=="Editor" set "BUILD_PRESET=local-editor"
if /i "%CONFIG%"=="Release" set "BUILD_PRESET=local-release"
if not defined BUILD_PRESET (
  echo [error] Unsupported configuration: %CONFIG%
  exit /b 1
)

set "CMAKE_COMMAND=cmake"
where cmake >nul 2>nul
if errorlevel 1 goto find_visual_studio_cmake
goto configure

:find_visual_studio_cmake
set "CMAKE_COMMAND=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if not exist "%CMAKE_COMMAND%" (
  echo [error] CMake was not found. Add cmake to PATH or install Visual Studio CMake tools.
  exit /b 1
)

:configure
"%CMAKE_COMMAND%" --preset local-windows-x64
if errorlevel 1 exit /b %errorlevel%

"%CMAKE_COMMAND%" --build --preset %BUILD_PRESET%
if errorlevel 1 exit /b %errorlevel%

endlocal