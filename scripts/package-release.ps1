#requires -Version 7.0
param([string]$PluginPath = 'build/Release/kcd1db.asi')

$ErrorActionPreference = 'Stop'

$loaderVersion = 'v10.0.4'
$loaderRepository = 'https://github.com/ThirteenAG/Ultimate-ASI-Loader'

Push-Location -LiteralPath (Split-Path -Parent $PSScriptRoot)
try {
    $loaderDirectory = "build/asi-loader-$loaderVersion"
    $loaderArchive = "$loaderDirectory/Ultimate-ASI-Loader-NoPDB_x64.zip"
    $loaderLicense = "$loaderDirectory/LICENSE.txt"
    New-Item -ItemType Directory -Path $loaderDirectory, build/release-assets -Force | Out-Null
    Invoke-WebRequest -Uri "$loaderRepository/releases/download/$loaderVersion/Ultimate-ASI-Loader-NoPDB_x64.zip" -OutFile $loaderArchive
    Invoke-WebRequest -Uri "https://raw.githubusercontent.com/ThirteenAG/Ultimate-ASI-Loader/$loaderVersion/license" -OutFile $loaderLicense
    Expand-Archive -LiteralPath $loaderArchive -DestinationPath $loaderDirectory -Force

    $archiveFiles = @(
        $PluginPath
        "$loaderDirectory/dinput8.dll"
    )
    Compress-Archive -LiteralPath $archiveFiles -DestinationPath build/release-assets/kcd1db.zip -Force
    Copy-Item -LiteralPath dist/kcd1db_fake_db.lua -Destination build/release-assets

    @(
        'kcd1db'
        (Get-Content -LiteralPath LICENSE -Raw)
        'SQLiteCpp 3.3.1'
        (Get-Content -LiteralPath build/_deps/sqlitecpp-src/LICENSE.txt -Raw)
        'MinHook 1.3.4'
        (Get-Content -LiteralPath build/_deps/minhook-src/LICENSE.txt -Raw)
        "Ultimate ASI Loader $loaderVersion"
        (Get-Content -LiteralPath $loaderLicense -Raw)
    ) | Set-Content -LiteralPath build/release-assets/kcd1db-licenses.txt -Encoding utf8
} finally {
    Pop-Location
}
