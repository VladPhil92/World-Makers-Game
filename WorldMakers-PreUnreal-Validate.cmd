@echo off
setlocal
set ROOT=%~dp0
powershell -NoProfile -ExecutionPolicy Bypass -File "%ROOT%scripts\run-pre-unreal-readiness.ps1" %*
exit /b %ERRORLEVEL%
