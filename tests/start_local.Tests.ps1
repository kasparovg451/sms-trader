$ErrorActionPreference = 'Stop'

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$launcherPath = Join-Path $repositoryRoot 'scripts\start-local.ps1'

if (-not (Test-Path -LiteralPath $launcherPath -PathType Leaf)) {
    throw "Local launcher is missing: $launcherPath"
}

. $launcherPath -FunctionsOnly

function Assert-Equal {
    param($Actual, $Expected, [string]$Message)
    if ($Actual -ne $Expected) {
        throw "$Message. Expected '$Expected', got '$Actual'."
    }
}

function New-TestRuntimeRoot {
    $root = Join-Path ([System.IO.Path]::GetTempPath()) ("smstrader-launcher-" + [guid]::NewGuid())
    New-Item -ItemType Directory -Path (Join-Path $root 'certs') -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $root 'build\Debug') -Force | Out-Null
    return $root
}

$missingRoot = New-TestRuntimeRoot
try {
    $missing = Get-LocalRuntimePrerequisites -ProjectRoot $missingRoot -BuildConfiguration 'Debug'
    Assert-Equal $missing.Ready $false 'Missing runtime must not be ready'
    Assert-Equal ($missing.Missing -contains 'db_config.json') $true 'Missing config must be reported'
    Assert-Equal ($missing.Missing -contains 'certs/server.crt') $true 'Missing certificate must be reported'
    Assert-Equal ($missing.Missing -contains 'certs/server.key') $true 'Missing key must be reported'
    Assert-Equal ($missing.Missing -contains 'smstrader_http.exe') $true 'Missing server must be reported'
    Assert-Equal ($missing.Missing -contains 'smstrader_gui.exe') $true 'Missing GUI must be reported'
} finally {
    Remove-Item -LiteralPath $missingRoot -Recurse -Force
}

$completeRoot = New-TestRuntimeRoot
try {
    Set-Content -LiteralPath (Join-Path $completeRoot 'db_config.json') -Value '{}'
    Set-Content -LiteralPath (Join-Path $completeRoot 'certs\server.crt') -Value 'certificate'
    Set-Content -LiteralPath (Join-Path $completeRoot 'certs\server.key') -Value 'key'
    Set-Content -LiteralPath (Join-Path $completeRoot 'build\Debug\smstrader_http.exe') -Value 'server'
    Set-Content -LiteralPath (Join-Path $completeRoot 'build\Debug\smstrader_gui.exe') -Value 'gui'

    $complete = Get-LocalRuntimePrerequisites -ProjectRoot $completeRoot -BuildConfiguration 'Debug'
    Assert-Equal $complete.Ready $true 'Complete runtime must be ready'
    Assert-Equal $complete.Missing.Count 0 'Complete runtime must have no missing items'
} finally {
    Remove-Item -LiteralPath $completeRoot -Recurse -Force
}

$healthUrl = Get-LocalHealthUrl -ProjectRoot $repositoryRoot
Assert-Equal $healthUrl 'https://localhost:8080/health' 'Default health URL must target local API'

Write-Host 'start-local tests passed'
