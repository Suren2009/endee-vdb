# Build the Endee Android library (AAR) for arm64-v8a.
param(
    [ValidateSet("release", "debug")]
    [string]$Variant = "release"
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$androidRoot = Resolve-Path (Join-Path $scriptDir "..")

$versionProps = Join-Path $androidRoot "gradle.properties"
$endeeVersion = "0.1.0"
if (Test-Path $versionProps) {
    foreach ($line in Get-Content $versionProps) {
        if ($line -match '^\s*endeeVersion\s*=\s*(.+)\s*$') {
            $endeeVersion = $Matches[1].Trim()
            break
        }
    }
}

if (-not $env:ANDROID_HOME) {
    $defaultSdk = Join-Path $env:LOCALAPPDATA "Android\Sdk"
    if (Test-Path $defaultSdk) {
        $env:ANDROID_HOME = $defaultSdk
    }
}

if (-not $env:ANDROID_HOME) {
    Write-Error "ANDROID_HOME is not set and the default SDK path was not found."
}

$localProps = Join-Path $androidRoot "local.properties"
if (-not (Test-Path $localProps)) {
    $sdkDir = $env:ANDROID_HOME.Replace('\', '\\')
    "sdk.dir=$sdkDir" | Set-Content -Encoding ASCII $localProps
}

$ndkDir = Join-Path $env:ANDROID_HOME "ndk\27.2.12479018"
if (-not (Test-Path (Join-Path $ndkDir "build\cmake\android.toolchain.cmake"))) {
    $ndkCandidates = Get-ChildItem (Join-Path $env:ANDROID_HOME "ndk") -Directory -ErrorAction SilentlyContinue |
        Sort-Object Name -Descending
    if ($ndkCandidates) {
        $ndkDir = $ndkCandidates[0].FullName
    }
}

if (Test-Path (Join-Path $ndkDir "build\cmake\android.toolchain.cmake")) {
    $env:ANDROID_NDK_HOME = $ndkDir
}

$capitalized = $Variant.Substring(0, 1).ToUpper() + $Variant.Substring(1)
$aarName = "endee-vdb-${Variant}-${endeeVersion}.aar"

Push-Location $androidRoot
try {
    & .\gradlew.bat ":library:stage${capitalized}Aar" --no-daemon
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    $dest = Join-Path $androidRoot "release\$aarName"
    if (-not (Test-Path $dest)) {
        Write-Error "AAR was not produced at release\$aarName"
    }

    Write-Host "[INFO] AAR: $dest"
}
finally {
    Pop-Location
}
