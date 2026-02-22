# Volumouse Build Script
# PowerShell version

$ErrorActionPreference = "Stop"

# Paths
$debug = ($args -contains "-debug")
$msys2Bin = "D:\Tools\msys64\ucrt64\bin"
$buildDir = ".\build"
$outputName = "volumouse-gcc"
$outputPath = "$buildDir\$outputName.exe"

# Validate GCC/MSYS2 installation
if (-not (Test-Path $msys2Bin)) {
    Write-Host "ERROR: MSYS2 installation not found at `"$msys2Bin`"" -ForegroundColor Red
    Write-Host "Please ensure MSYS2 path is correct." -ForegroundColor Red
    exit 1
}

# Add MSYS2 to PATH temporarily
Write-Host "Setting up MSYS2 environment..." -ForegroundColor Yellow
$env:PATH = "$msys2Bin;$env:PATH"
$gccPath = Get-Command gcc.exe -ErrorAction SilentlyContinue
$windresPath = Get-Command windres.exe -ErrorAction SilentlyContinue
if (-not $gccPath || -not $windresPath) {
    Write-Host "ERROR: Failed to find gcc.exe or windres.exe" -ForegroundColor Red
    Write-Host "Please ensure the following command was executed inside MSYS2 shell:" -ForegroundColor Red
    Write-Host "`"pacman -S --needed base-devel mingw-w64-ucrt-x86_64-toolchain`"" -ForegroundColor Red
    exit 1
}

# Create build directory if it doesn't exist
if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
    Write-Host "Created build directory: $buildDir" -ForegroundColor Cyan
}

# Step 1: Stop existing process if running
Write-Host "Stopping existing process..." -ForegroundColor Yellow
Stop-Process -Name "$outputName" -ErrorAction SilentlyContinue

# Step 2: Compile version resource
Write-Host "Compiling version resource..." -ForegroundColor Yellow
& windres.exe version.rc -O coff -o version.res
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Failed to compile version resource" -ForegroundColor Red
    exit 1
}

# Step 3: Compile executable
if ($debug) {
    Write-Host "Building in debug mode..." -ForegroundColor Yellow
    & gcc.exe -Wall -g -O0 -DEVENT_DEBUG hotcorner.c version.res -ladvapi32 -o $outputPath "-Wl,-subsystem,windows"
} else {
    Write-Host "Building in release mode..." -ForegroundColor Yellow
    & gcc.exe -Wall -O2 hotcorner.c version.res -o $outputPath "-Wl,-subsystem,windows"
}
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Compilation failed" -ForegroundColor Red
    exit 1
}
Remove-Item *.o -ErrorAction SilentlyContinue
Remove-Item *.res -ErrorAction SilentlyContinue


# Step 4: Check if executable was generated and run it
if (Test-Path $outputPath) {
    $fileInfo = Get-Item $outputPath
    Write-Host "`nBuild successful!" -ForegroundColor Green
    Write-Host "  Output: $($fileInfo.FullName)" -ForegroundColor Cyan
    Write-Host "  Size: $([math]::Round($fileInfo.Length / 1KB, 2)) KB" -ForegroundColor Cyan
    Write-Host "  Modified: $($fileInfo.LastWriteTime)" -ForegroundColor Cyan
    
    Write-Host "`nStarting application..." -ForegroundColor Yellow
    Start-Process -FilePath $outputPath
    Write-Host "Application started!" -ForegroundColor Green
} else {
    Write-Host "ERROR: Output executable not found at `"$outputPath`"" -ForegroundColor Red
    exit 1
}
