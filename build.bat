@echo off

rem 
set CONFIG=%~1
if "%CONFIG%"=="" set CONFIG=Debug

msbuild Engine/BroccoliEngine.vcxproj /t:Build /p:Configuration=%CONFIG% /p:Platform=x64 /m
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

msbuild BroccoliEngine.slnx /t:Build /p:Configuration=%CONFIG% /p:Platform=x64 /m
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%