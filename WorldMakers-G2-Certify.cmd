@echo off
setlocal
set "ROOT=%~dp0"
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%ROOT%scripts\run-g2-authored-certification.ps1" %*
set "EXITCODE=%ERRORLEVEL%"
echo.
if not "%EXITCODE%"=="0" (
  echo G2 certification is blocked. Inspect artifacts\g2-authored\g2-authored-vertical-slice.json and related evidence.
)
exit /b %EXITCODE%
