# Check WSL environment and install dependencies
# Run this if gen_pack.sh fails

$winPath = (Get-Location).Path
$driveLetter = $winPath.Substring(0, 1).ToLower()
$restOfPath = $winPath.Substring(2) -replace '\\', '/'
$wslPath = "/mnt/$driveLetter$restOfPath"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Checking WSL Environment" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Check if required tools are installed
Write-Host "Checking required tools..." -ForegroundColor Yellow

$tools = @("curl", "zip", "bash", "sed")
$missingTools = @()

foreach ($tool in $tools) {
    $check = wsl bash -c "command -v $tool" 2>$null
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  ✓ $tool found" -ForegroundColor Green
    } else {
        Write-Host "  ✗ $tool not found" -ForegroundColor Red
        $missingTools += $tool
    }
}

if ($missingTools.Count -gt 0) {
    Write-Host ""
    Write-Host "Missing tools detected. Installing..." -ForegroundColor Yellow
    Write-Host ""
    
    # Detect package manager
    Write-Host "Detecting Linux distribution..." -ForegroundColor Cyan
    $distro = wsl bash -c "cat /etc/os-release | grep '^ID=' | cut -d= -f2 | tr -d '\"'"
    Write-Host "Distribution: $distro" -ForegroundColor Cyan
    Write-Host ""
    
    $installCmd = ""
    if ($distro -match "ubuntu|debian") {
        Write-Host "Using apt-get package manager..." -ForegroundColor Yellow
        $installCmd = "sudo apt-get update && sudo apt-get install -y " + ($missingTools -join " ")
    }
    elseif ($distro -match "fedora|rhel|centos") {
        Write-Host "Using dnf/yum package manager..." -ForegroundColor Yellow
        $installCmd = "sudo dnf install -y " + ($missingTools -join " ")
    }
    elseif ($distro -match "arch") {
        Write-Host "Using pacman package manager..." -ForegroundColor Yellow
        $installCmd = "sudo pacman -Sy --noconfirm " + ($missingTools -join " ")
    }
    elseif ($distro -match "opensuse") {
        Write-Host "Using zypper package manager..." -ForegroundColor Yellow
        $installCmd = "sudo zypper install -y " + ($missingTools -join " ")
    }
    elseif ($distro -match "alpine") {
        Write-Host "Using apk package manager..." -ForegroundColor Yellow
        $installCmd = "sudo apk add " + ($missingTools -join " ")
    }
    else {
        Write-Host "Unknown distribution. Please install manually: $($missingTools -join ', ')" -ForegroundColor Red
        Write-Host ""
        Write-Host "Try running in WSL:" -ForegroundColor Yellow
        Write-Host "  For zip: install 'zip' package using your package manager" -ForegroundColor White
        Write-Host ""
        Read-Host "Press Enter to exit"
        exit 1
    }
    
    Write-Host "This requires sudo password in WSL." -ForegroundColor Yellow
    Write-Host ""
    
    wsl bash -c $installCmd
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host ""
        Write-Host "Tools installed successfully!" -ForegroundColor Green
    } else {
        Write-Host ""
        Write-Host "Failed to install tools." -ForegroundColor Red
        Read-Host "Press Enter to exit"
        exit 1
    }
}

Write-Host ""
Write-Host "Environment check complete!" -ForegroundColor Green
Write-Host ""
Write-Host "Now you can run: .\gen_pack.ps1" -ForegroundColor Cyan
Write-Host ""
Read-Host "Press Enter to exit"
