@echo off
setlocal
set "ROOT=%~dp0"
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%ROOT%scripts\run-g2-authored-certification.ps1" -AuthorMap %*
set "EXITCODE=%ERRORLEVEL%"
echo.
if not "%EXITCODE%"=="0" (
  echo G2 map authoring did not complete. Inspect artifacts\g2-authored\g2-author-report.json.
)
exit /b %EXITCODE%
