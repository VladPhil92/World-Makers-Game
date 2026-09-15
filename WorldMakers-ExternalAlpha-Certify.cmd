@echo off
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\run-external-alpha-playtest-readiness.ps1" %*
exit /b %ERRORLEVEL%
