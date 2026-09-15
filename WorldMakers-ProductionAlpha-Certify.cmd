@echo off
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\run-production-alpha-readiness.ps1" %*
exit /b %ERRORLEVEL%
