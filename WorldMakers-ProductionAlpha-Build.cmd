@echo off
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\build-production-alpha.ps1" %*
exit /b %ERRORLEVEL%
