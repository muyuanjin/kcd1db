#requires -Version 7.0
$ErrorActionPreference = 'Stop'

Push-Location -LiteralPath (Split-Path -Parent $PSScriptRoot)
try {
    New-Item -ItemType Directory -Path build/package, build/release-assets -Force | Out-Null
    Copy-Item -LiteralPath build/Release/kcd1db.asi, LICENSE -Destination build/package
    Copy-Item -LiteralPath docs/install.md -Destination build/package/INSTALL.md
    Copy-Item -LiteralPath build/_deps/sqlitecpp-src/LICENSE.txt -Destination build/package/SQLiteCpp-LICENSE.txt
    Copy-Item -LiteralPath build/_deps/minhook-src/LICENSE.txt -Destination build/package/MinHook-LICENSE.txt
    $archiveFiles = @(
        'build/package/kcd1db.asi'
        'build/package/LICENSE'
        'build/package/INSTALL.md'
        'build/package/SQLiteCpp-LICENSE.txt'
        'build/package/MinHook-LICENSE.txt'
    )
    Compress-Archive -LiteralPath $archiveFiles -DestinationPath build/release-assets/kcd1db.zip -Force
    Copy-Item -LiteralPath dist/kcd1db_fake_db.lua -Destination build/release-assets
} finally {
    Pop-Location
}
