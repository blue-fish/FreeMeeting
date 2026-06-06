param(
    [string]$BuildDir = ""
)

$ErrorActionPreference = "Stop"

$ProjectDir = Split-Path -Parent $MyInvocation.MyCommand.Path
if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $BuildDir = Join-Path (Split-Path -Parent $ProjectDir) "build-Wechat-Desktop_Qt_5_12_11_MinGW_32_bit-Release"
}

$QtBin = "E:\colin\Qt\5.12.11\mingw73_32\bin"
$MingwBin = "E:\colin\Qt\Tools\mingw730_32\bin"
$ProFile = Join-Path $ProjectDir "Wechat.pro"

if (!(Test-Path $ProFile)) {
    throw "Wechat.pro not found: $ProFile"
}

New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

$env:PATH = "$QtBin;$MingwBin;$env:PATH"

Push-Location $BuildDir
try {
    & (Join-Path $QtBin "qmake.exe") $ProFile -spec win32-g++ "CONFIG+=release" "CONFIG-=debug"
    if ($LASTEXITCODE -ne 0) {
        throw "qmake failed with exit code $LASTEXITCODE"
    }

    & (Join-Path $MingwBin "mingw32-make.exe") -f Makefile.Release
    if ($LASTEXITCODE -ne 0) {
        throw "mingw32-make failed with exit code $LASTEXITCODE"
    }

    Write-Host "Release build complete:"
    Write-Host (Join-Path $BuildDir "release\Wechat.exe")
}
finally {
    Pop-Location
}
