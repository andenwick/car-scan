@echo off
REM build.bat — Configure, build, and test the OBD-II C library.
REM
REM This script:
REM   1. Calls vcvarsall.bat to set up the MSVC compiler environment
REM   2. Runs CMake to generate Ninja build files
REM   3. Builds the library and test executables
REM   4. Runs all tests via ctest

REM Set up MSVC environment for 64-bit builds
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1

REM Configure with CMake (generate Ninja build files)
cd /d "%~dp0build"
cmake .. -G Ninja
if %ERRORLEVEL% neq 0 (
    echo CMake configuration FAILED
    exit /b 1
)

REM Build everything
cmake --build .
if %ERRORLEVEL% neq 0 (
    echo Build FAILED
    exit /b 1
)

REM Run tests
ctest --output-on-failure
if %ERRORLEVEL% neq 0 (
    echo Tests FAILED
    exit /b 1
)

echo.
echo === BUILD AND TESTS SUCCESSFUL ===
