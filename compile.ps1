# Volumouse Build Script
# PowerShell version

$ErrorActionPreference = "Stop"

# Paths
$msys2Bin = "C:\msys64\ucrt64\bin"
$buildDir = ".\build"
$outputExe = "$buildDir\volumouse.exe"

# Add MSYS2 to PATH temporarily
$env:PATH = "$msys2Bin;$env:PATH"

# Create build directory if it doesn't exist
if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
    Write-Host "Created build directory: $buildDir" -ForegroundColor Cyan
}

# Step 1: Compile version resource
Write-Host "`nStopping existing process..." -ForegroundColor Yellow
Stop-Process -Name "volumouse" -ErrorAction SilentlyContinue

# Step 2: Compile version resource
Write-Host "Compiling version resource..." -ForegroundColor Yellow
& "$msys2Bin\windres.exe" version.rc -O coff -o "$buildDir\version.res"
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Failed to compile version resource" -ForegroundColor Red
    exit 1
}
Write-Host "Version resource compiled successfully" -ForegroundColor Green

# Step 3: Compile executable
Write-Host "Compiling executable..." -ForegroundColor Yellow
& "$msys2Bin\gcc.exe" -O2 hotcorner.c "$buildDir\version.res" -o $outputExe "-Wl,-subsystem,windows"
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Compilation failed" -ForegroundColor Red
    exit 1
}

# Step 4: Check if executable was generated and run it
if (Test-Path $outputExe) {
    $fileInfo = Get-Item $outputExe
    Write-Host "Build successful!" -ForegroundColor Green
    Write-Host "  Output: $($fileInfo.FullName)" -ForegroundColor Cyan
    Write-Host "  Size: $([math]::Round($fileInfo.Length / 1KB, 2)) KB" -ForegroundColor Cyan
    Write-Host "  Modified: $($fileInfo.LastWriteTime)" -ForegroundColor Cyan
    
    Write-Host "`nStarting application..." -ForegroundColor Yellow
    Start-Process -FilePath $outputExe
    Write-Host "`nApplication started!" -ForegroundColor Green
} else {
    Write-Host "ERROR: Output executable not found at $outputExe" -ForegroundColor Red
    exit 1
}
