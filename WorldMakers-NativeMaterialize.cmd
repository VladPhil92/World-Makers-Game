@echo off
setlocal
set "ROOT=%~dp0"
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%ROOT%scripts\enter-native-unreal-materialization.ps1" %*
set "EXITCODE=%ERRORLEVEL%"
echo.
if not "%EXITCODE%"=="0" (
  echo Native Unreal materialization is blocked. Inspect artifacts\native-materialization\native-materialization-entry.json.
) else (
  echo Native Unreal materialization entry completed. Review the generated map in UE 5.8.2 before committing binaries.
)
exit /b %EXITCODE%
