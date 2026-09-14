@echo off
setlocal
set "ROOT=%~dp0"
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%ROOT%scripts\open-unreal-project.ps1" %*
set "EXITCODE=%ERRORLEVEL%"
echo.
if not "%EXITCODE%"=="0" (
  echo World Makers did not open Unreal Editor because native readiness is blocked.
  echo Read artifacts\unreal-readiness\native-failure-summary.json and readiness-result.json.
)
exit /b %EXITCODE%
