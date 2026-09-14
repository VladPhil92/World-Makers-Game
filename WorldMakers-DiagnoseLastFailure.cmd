@echo off
setlocal
set "ROOT=%~dp0"
python "%ROOT%scripts\classify-unreal-build-log.py" --log "%ROOT%artifacts\unreal-readiness\build.log" --output "%ROOT%artifacts\unreal-readiness\native-failure-summary.json" --repo-root "%ROOT%" --print
set "EXITCODE=%ERRORLEVEL%"
echo.
if not "%EXITCODE%"=="0" (
  echo Unable to classify the last Unreal build log.
)
exit /b %EXITCODE%
