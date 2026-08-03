@echo off

set CONFIG=%~1
if "%CONFIG%"=="" set CONFIG=Debug

set OUTDIR=Bin\x64\%CONFIG%
rmdir /s /q "%OUTDIR%/"

msbuild Engine/BroccoliEngine.vcxproj /t:Build /p:Configuration=%CONFIG% /p:Platform=x64 /m
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

msbuild BroccoliEngine.slnx /t:Build /p:Configuration=%CONFIG% /p:Platform=x64 /m
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%