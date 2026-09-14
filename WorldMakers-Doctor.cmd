@echo off
setlocal
set "ROOT=%~dp0"
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%ROOT%scripts\diagnose-unreal-workstation.ps1" %*
set "EXITCODE=%ERRORLEVEL%"
echo.
if not "%EXITCODE%"=="0" (
  echo World Makers Doctor found one or more blockers.
  echo Read artifacts\unreal-readiness\workstation-doctor.json for exact causes.
)
exit /b %EXITCODE%
