# Volumouse Build Script
# PowerShell version

$ErrorActionPreference = "Stop"

# Paths
$debug = ($args -contains "-debug")
$visualStudioVars = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
$buildDir = ".\build"
$outputName = "volumouse-msvc"
$outputPath = "$buildDir\$outputName.exe"

# Validate Visual Studio installation
if (-not (Test-Path $visualStudioVars)) {
    Write-Host "ERROR: Visual Studio installation not found at `"$visualStudioVars`"" -ForegroundColor Red
    Write-Host "Please ensure Visual Studio 2022 Community Edition is installed." -ForegroundColor Red
    exit 1
}

# Add MSVC to PATH temporarily
Write-Host "Setting up MSVC environment..." -ForegroundColor Yellow
try {
    cmd /c "`"$visualStudioVars`" x64 & set" | ForEach-Object {
        if ($_ -match '^([^=]*)=(.*)$') {
            [Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process")
        }
    }
    
    $clExePath = Get-Command cl.exe -ErrorAction SilentlyContinue
    if (-not $clExePath) {
        Write-Host "ERROR: Failed to set up MSVC environment: cl.exe not found in PATH" -ForegroundColor Red
        exit 1
    }
}
catch {
    Write-Host "ERROR: Failed to set up MSVC environment: $_" -ForegroundColor Red
    exit 1
}

# Create build directory if it doesn't exist
if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
    Write-Host "Created build directory: $buildDir" -ForegroundColor Cyan
}

# Step 1: Stop existing process if running
Write-Host "`nStopping existing process..." -ForegroundColor Yellow
Stop-Process -Name "$outputName" -ErrorAction SilentlyContinue

# Step 2: Compile version resource
Write-Host "Compiling version resource..." -ForegroundColor Yellow
& rc.exe version.rc
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Failed to compile version resource" -ForegroundColor Red
    exit 1
}

# Step 3: Compile executable
if ($debug) {
    Write-Host "Building in debug mode..." -ForegroundColor Yellow
    & cl.exe /W4 /O2 /DEVENT_DEBUG hotcorner.c version.res /link advapi32.lib /out:"$outputPath"
} else {
    Write-Host "Building in release mode..." -ForegroundColor Yellow
    & cl.exe /W4 /O2 hotcorner.c version.res /out:"$outputPath"
}
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Compilation failed" -ForegroundColor Red
    exit 1
}
Remove-Item *.obj -ErrorAction SilentlyContinue
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
}
else {
    Write-Host "ERROR: Output executable not found at `"$outputPath`"" -ForegroundColor Red
    exit 1
}
