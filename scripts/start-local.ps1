param(
    [ValidateSet('Debug', 'Release')]
    [string]$BuildConfiguration = 'Debug',
    [ValidateRange(1, 300)]
    [int]$HealthTimeoutSeconds = 30,
    [switch]$NoGui,
    [switch]$FunctionsOnly,
    [string]$ProjectRoot
)

$ErrorActionPreference = 'Stop'

function Get-LocalRuntimePrerequisites {
    param(
        [Parameter(Mandatory)] [string]$ProjectRoot,
        [Parameter(Mandatory)] [string]$BuildConfiguration
    )

    $required = [ordered]@{
        'db_config.json'       = Join-Path $ProjectRoot 'db_config.json'
        'certs/server.crt'     = Join-Path $ProjectRoot 'certs\server.crt'
        'certs/server.key'     = Join-Path $ProjectRoot 'certs\server.key'
        'smstrader_http.exe'   = Join-Path $ProjectRoot "build\$BuildConfiguration\smstrader_http.exe"
        'smstrader_gui.exe'    = Join-Path $ProjectRoot "build\$BuildConfiguration\smstrader_gui.exe"
    }

    $missing = @(
        foreach ($entry in $required.GetEnumerator()) {
            if (-not (Test-Path -LiteralPath $entry.Value -PathType Leaf)) {
                $entry.Key
            }
        }
    )

    [pscustomobject]@{
        Ready      = $missing.Count -eq 0
        Missing    = $missing
        ServerPath = $required['smstrader_http.exe']
        GuiPath    = $required['smstrader_gui.exe']
    }
}

function Get-LocalHealthUrl {
    param([Parameter(Mandatory)] [string]$ProjectRoot)

    $configPath = Join-Path $ProjectRoot 'app_config.json'
    $baseUrl = 'https://localhost:8080'
    if (Test-Path -LiteralPath $configPath -PathType Leaf) {
        try {
            $config = Get-Content -LiteralPath $configPath -Raw | ConvertFrom-Json
            if ($config.api_base_url) {
                $baseUrl = [string]$config.api_base_url
            }
        } catch {
            $baseUrl = 'https://localhost:8080'
        }
    }
    return $baseUrl.TrimEnd('/') + '/health'
}

function Test-LocalHealth {
    param([Parameter(Mandatory)] [string]$HealthUrl)

    $curl = Get-Command 'curl.exe' -ErrorAction SilentlyContinue
    if ($null -eq $curl) {
        throw 'curl.exe is required for the local server readiness check.'
    }

    & $curl.Source --silent --insecure --fail --max-time 2 $HealthUrl *> $null
    return $LASTEXITCODE -eq 0
}

if ($FunctionsOnly) {
    return
}

if ([string]::IsNullOrWhiteSpace($ProjectRoot)) {
    $ProjectRoot = Split-Path -Parent $PSScriptRoot
}
$ProjectRoot = [System.IO.Path]::GetFullPath($ProjectRoot)

$runtime = Get-LocalRuntimePrerequisites `
    -ProjectRoot $ProjectRoot `
    -BuildConfiguration $BuildConfiguration

if (-not $runtime.Ready) {
    $missingList = $runtime.Missing -join ', '
    throw "SMSTrader local runtime is incomplete. Missing: $missingList. Build the GUI and HTTP targets first."
}

$healthUrl = Get-LocalHealthUrl -ProjectRoot $ProjectRoot
$serverProcess = $null

if (-not (Test-LocalHealth -HealthUrl $healthUrl)) {
    $logsDirectory = Join-Path $ProjectRoot 'logs'
    New-Item -ItemType Directory -Path $logsDirectory -Force | Out-Null
    $stdoutLog = Join-Path $logsDirectory 'local-server.log'
    $stderrLog = Join-Path $logsDirectory 'local-server-error.log'

    $serverProcess = Start-Process `
        -FilePath $runtime.ServerPath `
        -WorkingDirectory $ProjectRoot `
        -RedirectStandardOutput $stdoutLog `
        -RedirectStandardError $stderrLog `
        -WindowStyle Hidden `
        -PassThru

    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
    while ($stopwatch.Elapsed.TotalSeconds -lt $HealthTimeoutSeconds) {
        if ($serverProcess.HasExited) {
            throw "SMSTrader server exited before becoming ready. See $stderrLog"
        }
        if (Test-LocalHealth -HealthUrl $healthUrl) {
            break
        }
        Start-Sleep -Milliseconds 500
        $serverProcess.Refresh()
    }

    if (-not (Test-LocalHealth -HealthUrl $healthUrl)) {
        throw "SMSTrader server did not become healthy within $HealthTimeoutSeconds seconds. See $stderrLog"
    }
}

Write-Host "SMSTrader server is ready at $healthUrl"

if (-not $NoGui) {
    Start-Process `
        -FilePath $runtime.GuiPath `
        -WorkingDirectory $ProjectRoot
    Write-Host 'SMSTrader client started.'
}
