# build.ps1 — Build and test the OBD-II C library using MSVC + Ninja

$ErrorActionPreference = "Stop"
$buildDir = "$PSScriptRoot\build"

# Import MSVC environment variables
$vcvars = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
# Run vcvarsall.bat and capture the resulting environment
$output = cmd /c "`"$vcvars`" x64 >nul 2>&1 && set" 2>&1
foreach ($line in $output) {
    if ($line -match "^([^=]+)=(.*)$") {
        [Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process")
    }
}

Write-Host "=== CMake Configure ===" -ForegroundColor Cyan
Push-Location $buildDir
try {
    & cmake .. -G Ninja 2>&1 | Write-Host
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }

    Write-Host "`n=== Build ===" -ForegroundColor Cyan
    & cmake --build . 2>&1 | Write-Host
    if ($LASTEXITCODE -ne 0) { throw "Build failed" }

    Write-Host "`n=== Tests ===" -ForegroundColor Cyan
    & ctest --output-on-failure 2>&1 | Write-Host
    if ($LASTEXITCODE -ne 0) { throw "Tests failed" }

    Write-Host "`n=== ALL PASSED ===" -ForegroundColor Green
} finally {
    Pop-Location
}
