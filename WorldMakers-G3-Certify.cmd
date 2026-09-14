@echo off
setlocal
cd /d "%~dp0"

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\run-g3-visual-certification.ps1" %*
set EXIT_CODE=%ERRORLEVEL%

if not "%EXIT_CODE%"=="0" echo World Makers G3 certification did not certify. Review artifacts\g3-visual\g3-visual-fidelity.json.
exit /b %EXIT_CODE%
