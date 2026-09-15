@echo off
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\run-release-candidate-readiness.ps1" %*
exit /b %ERRORLEVEL%
