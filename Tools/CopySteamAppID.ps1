param(
    [string]$AppIdFile = "steam_appid.txt"
)

# Resolve the source file
$SourcePath = Join-Path -Path (Get-Location) -ChildPath $AppIdFile
if (-not (Test-Path $SourcePath)) {
    Write-Error "steam_appid.txt not found in PWD: $SourcePath"
    return
}

# Folders to search
$SearchRoots = @("Build", "Binaries", "Intermediate", "Saved", "ArchivedBuilds")

# Executable names to match
$ExeNames = @(
    "TheGame.exe",
    "TheGameServer.exe",
    "TheGameClient.exe"
)

Write-Host "Searching for steam id files..." -ForegroundColor Cyan

foreach ($Root in $SearchRoots) {
    if (-not (Test-Path $Root)) { continue }

    # Recursively search for matching EXEs
    $Matches = Get-ChildItem -Path $Root -Recurse -File -ErrorAction SilentlyContinue |
                Where-Object { $ExeNames -contains $_.Name } 
    $Matches = $Matches.foreach({ return $_.Directory.FullName }) | Get-Unique
    foreach ($TargetDir in $Matches) {
        $TargetPath = Join-Path $TargetDir $AppIdFile

        if (-not (Test-Path $TargetPath)) {
            Write-Host "Copying steam_appid.txt → $TargetPath" -ForegroundColor Green
            Copy-Item -Path $SourcePath -Destination $TargetPath -Force
        }
    }
}

Write-Host "Done." -ForegroundColor Yellow