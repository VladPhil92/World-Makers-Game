@echo off
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\run-production-release-readiness.ps1" %*
exit /b %ERRORLEVEL%
