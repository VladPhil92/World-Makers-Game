@echo off
setlocal
set "ROOT=%~dp0"
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%ROOT%scripts\run-unreal-readiness-gate.ps1" -CleanIntermediate %*
set "EXITCODE=%ERRORLEVEL%"
echo.
if not "%EXITCODE%"=="0" (
  echo World Makers native readiness is blocked.
  echo Read artifacts\unreal-readiness\workstation-doctor.json first.
  echo Do not reinstall Unreal Engine unless the doctor explicitly identifies an Unreal installation defect.
)
exit /b %EXITCODE%
