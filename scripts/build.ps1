<#
.SYNOPSIS
    Configure and build a CMake preset inside a Visual Studio developer environment.

.DESCRIPTION
    Ninja + MSVC needs the vcvars environment for the target architecture.
    This script locates VS 2022 (Build Tools included) with vswhere, enters the
    dev shell for x64 or x86, then runs `cmake --preset` and `cmake --build --preset`
    in the windows/ directory. VCPKG_ROOT defaults to C:\dev\vcpkg when unset.

    Run it in a fresh process so x86/x64 environments do not pile up:
        powershell -NoProfile -File scripts\build.ps1 -Preset x64-debug
        powershell -NoProfile -File scripts\build.ps1 -Preset x86-release
#>
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('x64-debug', 'x64-release', 'x86-release')]
    [string]$Preset
)

$ErrorActionPreference = 'Stop'

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) {
    throw 'vswhere.exe not found. Install Visual Studio 2022 Build Tools (C++ workload).'
}
$vsPath = & $vswhere -latest -products * -version '[17.0,18.0)' `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsPath) {
    throw 'MSVC v143 toolset not found.'
}

if (-not $env:VCPKG_ROOT) {
    $env:VCPKG_ROOT = 'C:\dev\vcpkg'
}
if (-not (Test-Path (Join-Path $env:VCPKG_ROOT 'scripts\buildsystems\vcpkg.cmake'))) {
    throw "vcpkg not found at $env:VCPKG_ROOT"
}

$arch = if ($Preset.StartsWith('x86')) { 'x86' } else { 'amd64' }
Import-Module (Join-Path $vsPath 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll')
Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation -DevCmdArguments "-arch=$arch -host_arch=amd64" | Out-Null

Push-Location (Join-Path $PSScriptRoot '..\windows')
try {
    cmake --preset $Preset
    if ($LASTEXITCODE -ne 0) { throw "configure failed (exit $LASTEXITCODE)" }
    cmake --build --preset $Preset
    if ($LASTEXITCODE -ne 0) { throw "build failed (exit $LASTEXITCODE)" }
}
finally {
    Pop-Location
}
