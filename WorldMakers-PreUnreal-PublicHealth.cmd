@echo off
setlocal
cd /d "%~dp0"
python scripts\probe-pre-unreal-public-health.py --repository-commit unknown --observer windows-release-ops
exit /b %ERRORLEVEL%
