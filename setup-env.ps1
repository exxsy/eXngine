Write-Host "---------------------------------------------------" -ForegroundColor Cyan
Write-Host "     Visual Studio & SDK Environment Setup (PS)" -ForegroundColor Cyan
Write-Host "---------------------------------------------------"
Write-Host "[INFO] Searching for Visual Studio..."

$vswherePath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsPath = $null

if (Test-Path $vswherePath) {
    $vsPath = & $vswherePath -latest -products * -property installationPath
}

if (-not [string]::IsNullOrWhiteSpace($vsPath)) {
    Write-Host "[FOUND] Visual Studio: $vsPath" -ForegroundColor Green
    
    [System.Environment]::SetEnvironmentVariable("VISUAL_STUDIO_PATH", $vsPath, "User")
    Write-Host "[SUCCESS] Variable 'VISUAL_STUDIO_PATH' updated." -ForegroundColor Green

    $vcvarsPath = Join-Path $vsPath "VC\Auxiliary\Build\vcvarsall.bat"
    if (Test-Path $vcvarsPath) {
        Write-Host "[FOUND] vcvarsall.bat: $vcvarsPath" -ForegroundColor Green
        [System.Environment]::SetEnvironmentVariable("VCVARSALL_PATH", $vcvarsPath, "User")
        Write-Host "[SUCCESS] Variable 'VCVARSALL_PATH' updated." -ForegroundColor Green
    } else {
        Write-Host "[WARNING] vcvarsall.bat not found at expected location." -ForegroundColor Yellow
    }
} else {
    Write-Host "[WARNING] Could not locate Visual Studio via vswhere." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "[INFO] Searching for Windows SDK..."

$sdkPath = $null
$regPaths = @(
    "HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows Kits\Installed Roots",
    "HKLM:\SOFTWARE\Microsoft\Windows Kits\Installed Roots"
)

foreach ($path in $regPaths) {
    if (Test-Path $path) {
        try {
            $sdkPath = Get-ItemPropertyValue -Path $path -Name "KitsRoot10" -ErrorAction Stop
            if ($sdkPath) { break }
        }
        catch {
            
        }
        
        try {
            $sdkPath = Get-ItemPropertyValue -Path $path -Name "KitsRoot81" -ErrorAction Stop
            if ($sdkPath) { break }
        }
        catch {}
    }
}

if (-not [string]::IsNullOrWhiteSpace($sdkPath)) {
    Write-Host "[FOUND] Windows SDK: $sdkPath" -ForegroundColor Green
    [System.Environment]::SetEnvironmentVariable("MICROSOFT_SDK_PATH", $sdkPath, "User")
    Write-Host "[SUCCESS] Variable 'MICROSOFT_SDK_PATH' updated." -ForegroundColor Green
} else {
    Write-Host "[WARNING] Could not locate Windows SDK in Registry." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Done. Please restart your terminal/PowerShell to load the new variables." -ForegroundColor Gray