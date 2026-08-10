@echo off
setlocal

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Debug"

if /i "%CONFIG%"=="Debug" set "BUILD_PRESET=debug-local"
if /i "%CONFIG%"=="Editor" set "BUILD_PRESET=editor-local"
if /i "%CONFIG%"=="Release" set "BUILD_PRESET=release-local"
if not defined BUILD_PRESET (
  echo [error] Unsupported configuration: %CONFIG%
  exit /b 1
)

set "CMAKE_COMMAND=cmake"
where cmake >nul 2>nul
if errorlevel 1 goto find_visual_studio_cmake
goto configure

:find_visual_studio_cmake
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
  for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -property installationPath`) do (
    set "VS_INSTALL_DIR=%%i"
  )
)
if defined VS_INSTALL_DIR (
  set "CMAKE_COMMAND=%VS_INSTALL_DIR%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
)

if not exist "%CMAKE_COMMAND%" (
  echo [error] CMake was not found. Add cmake to PATH or install Visual Studio CMake tools.
  exit /b 1
)

:configure
if exist "CMakeUserPresets.json" (
  findstr /C:"{YOUR_VCPKG_ROOT_DIRECTORY}" "CMakeUserPresets.json" >nul 2>&1
  if not errorlevel 1 (
    echo [error] CMakeUserPresets.json still contains {YOUR_VCPKG_ROOT_DIRECTORY}.
    echo [error] Please update VCPKG_ROOT in CMakeUserPresets.json to point to your vcpkg installation.
    exit /b 1
  )
)

"%CMAKE_COMMAND%" --preset windows-x64-local
if errorlevel 1 exit /b %errorlevel%

"%CMAKE_COMMAND%" --build --preset %BUILD_PRESET%
if errorlevel 1 exit /b %errorlevel%

endlocal
