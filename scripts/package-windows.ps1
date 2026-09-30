param(
    [Parameter(Mandatory)]
    [string]$Version,
    [ValidateSet('Debug', 'Release')]
    [string]$BuildConfiguration = 'Release',
    [string]$StageDir,
    [string]$ReleaseDir,
    [switch]$ValidateOnly,
    [switch]$SkipBuild
)

$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path -Parent $PSScriptRoot

if ([string]::IsNullOrWhiteSpace($StageDir)) {
    $StageDir = Join-Path $repositoryRoot 'dist\app'
}
if ([string]::IsNullOrWhiteSpace($ReleaseDir)) {
    $ReleaseDir = Join-Path $repositoryRoot 'dist\releases'
}

$semanticVersionPattern = '^[0-9]+\.[0-9]+\.[0-9]+(?:-[0-9A-Za-z.-]+)?$'
if ($Version -notmatch $semanticVersionPattern) {
    throw "Version '$Version' is not a semantic version such as 0.1.0 or 0.1.0-beta.1."
}

$StageDir = [System.IO.Path]::GetFullPath($StageDir)
$ReleaseDir = [System.IO.Path]::GetFullPath($ReleaseDir)

if ($ValidateOnly) {
    $mainExecutable = Join-Path $StageDir 'smstrader_gui.exe'
    if (-not (Test-Path -LiteralPath $mainExecutable -PathType Leaf)) {
        throw "Staged application is missing $mainExecutable"
    }
    Write-Host "Package inputs are valid for SMSTrader $Version."
    exit 0
}

$distRoot = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot 'dist'))
if (-not $StageDir.StartsWith($distRoot + [System.IO.Path]::DirectorySeparatorChar)) {
    throw "StageDir must be inside $distRoot to allow safe cleanup."
}
if (-not $ReleaseDir.StartsWith($distRoot + [System.IO.Path]::DirectorySeparatorChar)) {
    throw "ReleaseDir must be inside $distRoot."
}

$cmake = 'C:\Program Files\Microsoft Visual Studio\18\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
if (-not (Test-Path -LiteralPath $cmake -PathType Leaf)) {
    $cmakeCommand = Get-Command 'cmake.exe' -ErrorAction SilentlyContinue
    if ($null -eq $cmakeCommand) {
        throw 'CMake was not found.'
    }
    $cmake = $cmakeCommand.Source
}

if (-not $SkipBuild) {
    & $cmake --build (Join-Path $repositoryRoot 'build') --config $BuildConfiguration --target smstrader_gui -- /m:1
    if ($LASTEXITCODE -ne 0) {
        throw 'Release GUI build failed.'
    }

    if (Test-Path -LiteralPath $StageDir) {
        Remove-Item -LiteralPath $StageDir -Recurse -Force
    }
    New-Item -ItemType Directory -Path $StageDir -Force | Out-Null

    & $cmake --install (Join-Path $repositoryRoot 'build') `
        --config $BuildConfiguration `
        --prefix $StageDir `
        --component Runtime
    if ($LASTEXITCODE -ne 0) {
        throw 'CMake install/deployment failed.'
    }
}

$mainExecutable = Join-Path $StageDir 'smstrader_gui.exe'
if (-not (Test-Path -LiteralPath $mainExecutable -PathType Leaf)) {
    throw "Staged application is missing $mainExecutable"
}

New-Item -ItemType Directory -Path $ReleaseDir -Force | Out-Null

Push-Location $repositoryRoot
try {
    dotnet tool restore
    if ($LASTEXITCODE -ne 0) {
        throw 'Unable to restore the pinned Velopack tool.'
    }

    dotnet tool run vpk pack `
        --packId SMSTrader `
        --packVersion $Version `
        --packDir $StageDir `
        --mainExe smstrader_gui.exe `
        --runtime win-x64 `
        --outputDir $ReleaseDir
    if ($LASTEXITCODE -ne 0) {
        throw 'Velopack packaging failed.'
    }
} finally {
    Pop-Location
}

Write-Host "SMSTrader $Version artifacts are in $ReleaseDir"
