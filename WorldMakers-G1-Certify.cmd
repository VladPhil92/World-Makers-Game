@echo off
setlocal
set "ROOT=%~dp0"
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%ROOT%scripts\run-g1-native-certification.ps1" -CleanIntermediate %*
set "EXITCODE=%ERRORLEVEL%"
echo.
if not "%EXITCODE%"=="0" (
  echo World Makers G1 certification is BLOCKED.
  echo Inspect artifacts\g1-native\g1-native-readiness.json and supporting evidence.
) else (
  echo World Makers G1 certification command completed.
  echo Inspect artifacts\g1-native\g1-native-readiness.json for CERTIFIED or NON_CERTIFYING_PASS.
)
exit /b %EXITCODE%
