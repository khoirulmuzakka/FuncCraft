@echo off
setlocal

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
set "BUILD_DIR=%ROOT%\build"

cmake -S "%ROOT%" -B "%BUILD_DIR%" -DFUNCCRAFT_BUILD_EXAMPLES=ON -DFUNCCRAFT_LINK_MINION=ON
if errorlevel 1 exit /b %errorlevel%

cmake --build "%BUILD_DIR%" --config Release -- /m:8
if errorlevel 1 exit /b %errorlevel%

endlocal
