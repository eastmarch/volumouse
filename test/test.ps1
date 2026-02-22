# Test Build Script
# PowerShell version

$ErrorActionPreference = "Stop"

# Paths
$msys2Bin = "C:\msys64\ucrt64\bin"
$buildDir = ".\build"
$outputExe = "$buildDir\test.exe"

# Add MSYS2 to PATH temporarily
$env:PATH = "$msys2Bin;$env:PATH"

# Create build directory if it doesn't exist
if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
    Write-Host "Created build directory: $buildDir" -ForegroundColor Cyan
}

# Step 1: Compile executable
Write-Host "Compiling executable..." -ForegroundColor Yellow
& "$msys2Bin\gcc.exe" -O2 test.c -o $outputExe -lgdi32 "-Wl,-subsystem,console"
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Compilation failed" -ForegroundColor Red
    exit 1
}

# Step 2: Check if executable was generated and run it
if (Test-Path $outputExe) {
    $fileInfo = Get-Item $outputExe
    Write-Host "Build successful!" -ForegroundColor Green
    Write-Host "  Output: $($fileInfo.FullName)" -ForegroundColor Cyan
    Write-Host "  Size: $([math]::Round($fileInfo.Length / 1KB, 2)) KB" -ForegroundColor Cyan
    Write-Host "  Modified: $($fileInfo.LastWriteTime)" -ForegroundColor Cyan
    
    Write-Host "`nStarting application..." -ForegroundColor Yellow
    & $outputExe
    Write-Host "`nApplication finished!" -ForegroundColor Green
} else {
    Write-Host "ERROR: Output executable not found at $outputExe" -ForegroundColor Red
    exit 1
}
