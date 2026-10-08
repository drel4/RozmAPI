$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path -Parent $PSScriptRoot
$releaseRoot = Join-Path $taskRoot 'build\release\RozmAPI'
New-Item -ItemType Directory -Force -Path $releaseRoot | Out-Null
foreach ($dir in @('src', 'include', 'examples', 'tests', 'docs', 'tools')) {
    $dest = Join-Path $releaseRoot $dir
    New-Item -ItemType Directory -Force -Path $dest | Out-Null
    Get-ChildItem -LiteralPath (Join-Path $taskRoot $dir) -File | Copy-Item -Destination $dest -Force
}
Copy-Item -LiteralPath (Join-Path $taskRoot 'README.md'), (Join-Path $taskRoot 'LICENSE'), (Join-Path $taskRoot 'build.cmd') -Destination $releaseRoot -Force
foreach ($arch in @('win32', 'win64')) {
    $dest = Join-Path $releaseRoot "build\$arch"
    New-Item -ItemType Directory -Force -Path $dest | Out-Null
    foreach ($file in @('RozmAPI.dll', 'RozmAPI.lib', 'RozmWorker32.exe')) {
        Copy-Item -LiteralPath (Join-Path $taskRoot "build\$arch\$file") -Destination $dest -Force
    }
}
$zip = Join-Path $taskRoot 'build\RozmAPI-1.0.zip'
Compress-Archive -LiteralPath $releaseRoot -DestinationPath $zip -Force
Get-FileHash -LiteralPath $zip -Algorithm SHA256 | Format-List
