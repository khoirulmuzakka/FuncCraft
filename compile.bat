@echo off
setlocal

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
set "BUILD_DIR=%ROOT%\build"
set "BUILD_TYPE=Release"

:parse_args
if "%~1"=="" goto done_parse
if /I "%~1"=="--release" (
    set "BUILD_TYPE=Release"
    shift
    goto parse_args
)
if /I "%~1"=="--debug" (
    set "BUILD_TYPE=Debug"
    shift
    goto parse_args
)
if /I "%~1"=="--help" goto help
echo Unknown option: %~1
goto usage

:done_parse
echo Configuring FuncCraft (%BUILD_TYPE%)...
cmake -S "%ROOT%" -B "%BUILD_DIR%" ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DBUILD_LIBRARY=ON ^
    -DBUILD_PYTHON=ON ^
    -DBUILD_TEST=ON ^
    -DBUILD_EXAMPLES=ON
if errorlevel 1 exit /b %errorlevel%

echo Building FuncCraft (%BUILD_TYPE%)...
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% -- /m:8
if errorlevel 1 exit /b %errorlevel%

endlocal
exit /b 0

:usage
echo Usage: compile.bat [--release^|--debug]
echo.
echo   --release   Build Release configuration (default)
echo   --debug     Build Debug configuration
exit /b 2

:help
echo Usage: compile.bat [--release^|--debug]
echo.
echo   --release   Build Release configuration (default)
echo   --debug     Build Debug configuration
exit /b 0
