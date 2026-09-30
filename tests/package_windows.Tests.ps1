$ErrorActionPreference = 'Stop'

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$packageScript = Join-Path $repositoryRoot 'scripts\package-windows.ps1'

if (-not (Test-Path -LiteralPath $packageScript -PathType Leaf)) {
    throw "Windows packaging script is missing: $packageScript"
}

function Invoke-PackageValidation {
    param([string]$Version, [string]$StageDir)

    try {
        & powershell.exe `
            -NoProfile `
            -ExecutionPolicy Bypass `
            -File $packageScript `
            -Version $Version `
            -StageDir $StageDir `
            -ValidateOnly *> $null
    } catch {
        # The negative test cases intentionally make the child process fail.
    }
    return $LASTEXITCODE
}

$emptyStage = Join-Path ([System.IO.Path]::GetTempPath()) ("smstrader-package-empty-" + [guid]::NewGuid())
$completeStage = Join-Path ([System.IO.Path]::GetTempPath()) ("smstrader-package-complete-" + [guid]::NewGuid())
New-Item -ItemType Directory -Path $emptyStage, $completeStage -Force | Out-Null

try {
    Set-Content -LiteralPath (Join-Path $completeStage 'smstrader_gui.exe') -Value 'development executable'

    if ((Invoke-PackageValidation -Version 'not-a-version' -StageDir $completeStage) -eq 0) {
        throw 'Invalid semantic version must be rejected.'
    }
    if ((Invoke-PackageValidation -Version '0.1.0' -StageDir $emptyStage) -eq 0) {
        throw 'A stage without smstrader_gui.exe must be rejected.'
    }
    if ((Invoke-PackageValidation -Version '0.1.0' -StageDir $completeStage) -ne 0) {
        throw 'A valid version and complete stage must pass validation.'
    }
} finally {
    Remove-Item -LiteralPath $emptyStage, $completeStage -Recurse -Force
}

Write-Host 'package-windows tests passed'
