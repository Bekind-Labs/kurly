# Builds Kurly and packs it, with Qt and the MSVC runtime, into a portable zip.
#
#   pwsh packaging/windows/make-zip.ps1 [-BuildDir build-release] [-OutDir dist]
#
# Run from a Visual Studio developer prompt (so MSVC and VCToolsRedistDir are
# set) with Qt's bin directory on PATH (for windeployqt).
param(
    [string]$BuildDir = "build-release",
    [string]$OutDir = "dist"
)
$ErrorActionPreference = "Stop"

function Invoke-Checked {
    param([string]$Exe, [string[]]$Arguments)
    & $Exe @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Exe failed with exit code $LASTEXITCODE" }
}

$Root = (Resolve-Path "$PSScriptRoot\..\..").Path
$Version = (Select-String -Path "$Root\CMakeLists.txt" -Pattern 'project\(kurly VERSION ([0-9.]+)').Matches[0].Groups[1].Value
$BuildDir = [IO.Path]::GetFullPath($BuildDir)
$OutDir = [IO.Path]::GetFullPath($OutDir)

Write-Host "==> Building Kurly $Version"
Invoke-Checked cmake @("-S", $Root, "-B", $BuildDir, "-DCMAKE_BUILD_TYPE=Release",
                       "-DKURLY_BUILD_TESTS=OFF", "-DKURLY_REQUIRE_QT_TRANSLATIONS=ON")
Invoke-Checked cmake @("--build", $BuildDir, "--config", "Release", "--parallel")

# Multi-config generators (Visual Studio) put the exe in Release\.
$Exe = @("$BuildDir\Release\kurly.exe", "$BuildDir\kurly.exe") | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $Exe) { throw "kurly.exe not found in $BuildDir" }

Write-Host "==> Staging"
$Name = "Kurly-$Version-windows-x64"
$Stage = Join-Path $BuildDir $Name
if (Test-Path $Stage) { Remove-Item -Recurse -Force $Stage }
New-Item -ItemType Directory -Force -Path $Stage, $OutDir | Out-Null
Copy-Item $Exe "$Stage\Kurly.exe"
Copy-Item "$Root\README.md", "$Root\LICENSE", "$Root\THIRD_PARTY_NOTICES.md" $Stage
Copy-Item -Recurse "$Root\licenses" "$Stage\licenses"

# Kurly's and Qt's Japanese translations are embedded in the exe already.
$WinDeployQt = (Get-Command windeployqt -ErrorAction Stop).Source
Invoke-Checked $WinDeployQt @("--release", "--no-translations", "--no-system-d3d-compiler",
                              "--no-opengl-sw", "$Stage\Kurly.exe")

# The MSVC runtime, so users don't need to install the VC++ redistributable.
if ($env:VCToolsRedistDir) {
    $Crt = Get-ChildItem "$env:VCToolsRedistDir\x64\Microsoft.VC*.CRT" -Directory | Select-Object -First 1
    if ($Crt) { Copy-Item "$($Crt.FullName)\*.dll" $Stage }
} else {
    Write-Warning "VCToolsRedistDir not set; the MSVC runtime DLLs were not bundled."
}

Write-Host "==> Zipping"
$Zip = Join-Path $OutDir "$Name.zip"
if (Test-Path $Zip) { Remove-Item $Zip }
Compress-Archive -Path $Stage -DestinationPath $Zip
Write-Host "==> Built $Zip"
