

$LsSourceDir = if ($args.Count -gt 0) `
    { "$($args[0])\Source\TheGame" } else `
    { "$PWD\Source\TheGame" }
Write-host "Source Dir: $LsSourceDir"

if (-not (Test-Path $LsSourceDir))
{
    Write-Host "You are not in a project dir"
    exit
}

function New-PluginLinks {
    param(
        [string]$FsPluginName
    )
    # $CleanName = $PluginName -replace "\.\\", ""
    # $CleanName = $CleanName -replace "\\", ""
    
    $GitRepo = "$PSScriptRoot\$FsPluginName"
    write-host "$GitRepo\Public\$FsPluginName"
    write-host "$LsSourceDir\Public\$FsPluginName"
    write-host "$LsSourceDir\private\$FsPluginName"
    Write-Host "Path: $LsSourceDir\Public\$FsPluginName"
    Write-Host "Targ: $GitRepo\Public\$FsPluginName"

    $LsPublicPlugin = "$LsSourceDir\Public\$FsPluginName"
    if(-not (Test-Path $LsPublicPlugin))
    {
        New-Item `
            -ItemType SymbolicLink `
            -Path    $LsPublicPlugin  `
            -Target "$GitRepo\Public\$FsPluginName"    
    }

    $LsPrivatePlugin = "$LsSourceDir\Private\$FsPluginName"
    if(-not (Test-Path $LsPrivatePlugin))
    {
        New-Item `
            -ItemType SymbolicLink `
            -Path   $LsPrivatePlugin `
            -Target "$GitRepo\Private\$FsPluginName"
    }
}

Write-Host "-----------------------------------------------------------------"
[string[]]$LvPlugins = Get-ChildItem -Path $PSScriptRoot -Directory | Select-Object -ExpandProperty Name
foreach ($LsPlugin in $LvPlugins) {
    New-PluginLinks -FsPluginName $LsPlugin
    Write-Host "-----------------------------------------------------------------"
}
