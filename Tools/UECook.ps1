# Copywrite by Joel Leagues with the Apache v2 Licence

param(
    [switch]$Development,
    [switch]$Shipping,

    [switch]$OnlyClient,
    [switch]$OnlyServer,
    [switch]$OnlyP2P,
    [switch]$Editor,
    [switch]$SkipShaderCompile,

    [switch]$OnlyUpdateVersions,
    [switch]$SkipVersionUpdate,
    [switch]$OnlyPrint,
    [switch]$NoCopyFiles
)

set64

$RunParams = [PSCustomObject]@{
    Development = $Development.IsPresent
    Shipping    = $Shipping.IsPresent
    Editor      = $Editor.IsPresent
    Client      = $OnlyClient.IsPresent
    Server      = $OnlyServer.IsPresent
    P2P         = $OnlyP2P.IsPresent
    SkipShaderCompile = $SkipShaderCompile.IsPresent
    BuildAll   = $false
}

if ($RunParams.Client -eq $false -and $RunParams.Server -eq $false -and $RunParams.P2P -eq $false) 
{
    $RunParams.Client = $true
    $RunParams.Server = $true
    $RunParams.P2P    = $true
    $RunParams.BuildAll = $true
}

class Core 
{
    static [string]$Development = "Development"
    static [string]$Shipping = "Shipping"
    static [string]$RunUatPath = "D:\UE\Engines\UnrealEngine\Engine\Build\BatchFiles\RunUAT.bat"
    static [string]$EditorCMD = "D:\UE\Engines\UnrealEngine\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
    static [int]$TerminalWidth = $(Get-Host).UI.RawUI.WindowSize.Width

    [PSCustomObject]$MoRunParams
    [string]$UProjectPath
    [string]$ArchiveDirectory
    [string]$MsMethod
    [string]$MsProjectRoot
    [string]$MsProjectName
    
    Core([PSCustomObject]$FoRunParams) 
    {
        $This.MoRunParams = $FoRunParams
        $ProjectRoot = $PWD.Path
        $This.MsProjectRoot = $PWD.Path
        $This.SetProjectName()

        Write-Host "Ensure you have 1st resaved all assets so they have versions"
        Write-Host "Then you can save to newer versions"
        Write-Host "Then you cook content"

        # $This.ArchiveDirectory = "$ProjectRoot\ArchivedBuilds"

        if ($This.MoRunParams.Development) {
            $This.MsMethod = [Core]::Development
        } elseif ($This.MoRunParams.Shipping) {
            $This.MsMethod = [Core]::Shipping
        } else {
            Write-Host "Bad Code"
            exit 1
        }

        if (-not $This.MsProjectName) {
            Write-Error "❌ No .uproject file found in $ProjectRoot"
            exit 1
        }
    }

    [void] SetProjectName()
    {
        if (Test-Path "$($PWD.Path)\Source") {
            $This.UProjectPath = "$($PWD.Path)\$($(Get-ChildItem | Select-Object Name | Where-Object Name -like *uproject).Name)";
            $This.MsProjectName = [System.IO.Path]::GetFileNameWithoutExtension($This.UProjectPath);
            # $This.MsNestedProjectName = [System.IO.Path]::GetFileNameWithoutExtension($This.MsProjectName);
        }
    }

    [string] GetAssetVersionUpdateCMD()
    {
        $LvAargs = @(
            "$([Core]::EditorCMD)"
            "$($This.MsProjectRoot)\$($This.MsProjectName).uproject",
            "-run=ResavePackages",
            "-IgnoreEngineVersion",
            "-AllowCommandletRendering",
            "-PackageFolder=/Game",
            "-unattended",
            "-AutoCheckOutPackages",
            "-AutoCheckIn"
        )

        $LsCommand =  [Core]::JoinWithSpaces($LvAargs)
        return $LsCommand
    }

    static[void] PrintLine(){
        Write-Host ("=" * [Core]::TerminalWidth)
    }

    static [string] JoinWithSpaces([string[]]$FvArgs) 
    {
        [string] $LsResult = ""
        for($i = 0; $i -lt $FvArgs.Length; $i++) {
            if ($i -eq 0) {
                $LsResult = $FvArgs[$i]
            } else {
                $LsResult += " ```n"
                $LsResult += "    " + $FvArgs[$i]
            }
        }
        return $LsResult
    }

    [string] GetCookCMD()
    {
        $LvAargs = @(
            "BuildCookRun",
            "-project=`"$($This.UProjectPath)`"",
            "-noP4",
            "-platform=Win64",
            "-clientconfig=$($This.MsMethod)",
            "-serverconfig=$($This.MsMethod)",
            "-cook", 
            "-build", 
            "-stage", 
            "-pak", 
            "-iostore",
            "-compressed",
            "-package", 
            "-archive",
            # "-archivedirectory=`"$($This.ArchiveDirectory)`"" # fine for both client & server
            "-MSBuildArgs=`"/p:TreatWarningsAsErrors=false /p:NoWarn=NU1900 /p:NoWarn=NU1901 /p:NoWarn=NU1902 /p:NoWarn=NU1903 /p:NoWarn=NU1904`""
        )

        if($This.MoRunParams.Editor -eq $false)
        {
            if($this.MoRunParams.BuildAll -eq $false)
            {
                if ($This.MoRunParams.Client) {
                    $LvAargs += "-client" # exclusivly only client
                }
                if ($This.MoRunParams.Server) {
                    $LvAargs += "-server" # exclusivly only client
                }
            }else{
                $LvAargs += "-client"
                $LvAargs += "-server"
            }
            if ($This.MoRunParams.SkipShaderCompile){
                $LvAargs += "-noshadercompile"
            }
        }

        $LsCommandArgs =  [Core]::JoinWithSpaces($LvAargs)
        $LsCommand = "$([Core]::RunUatPath) $LsCommandArgs"

        return $LsCommand
    }

    [string] GetCookP2PCMD()
    {
        $LvAargs = @(
            "BuildCookRun",
            "-project=`"$($This.UProjectPath)`"",
            "-noP4",
            "-platform=Win64",
            "-clientconfig=Development",
            "-target=TheGame",
            "-build",
            "-cook",
            "-stage",
            "-package",
            "-pak",
            "-iostore"
            # "-archive", "-archivedirectory=`"$($This.ArchiveDirectory)`""
        )

        $LsCommandArgs =  [Core]::JoinWithSpaces($LvAargs)
        $LsCommand = "$([Core]::RunUatPath) $LsCommandArgs"
        
        return $LsCommand
    }

    [void] CopyIfNewer([string]$Source, [string]$DestinationFolder)
    {
        if (-not (Test-Path $Source)) {
            Write-Warning "Source does not exist: $Source"
            return
        }

        $Destination = Join-Path $DestinationFolder (Split-Path $Source -Leaf)
        if (-not (Test-Path $Destination)) {
            Copy-Item -Force -Recurse $Source $Destination
            return
        }

        $SourceItem = Get-Item $Source
        $DestItem = Get-Item $Destination

        if ($SourceItem.LastWriteTime -gt $DestItem.LastWriteTime) {
            Copy-Item -Force -Recurse $Source $Destination
            # Write-Host "Copied  (newer): $Source -> $Destination"
        }else{
            # Write-Host "Passing (older): $Source"
        }
    }

    [void] CopyFiles()
    {
        if($This.MoRunParams.Editor -eq $false){
            Write-Host ">>> NO COPY NO EDITOR OPTION <<<"
            return;
        }
        $LsSourceFolder = "$($This.MsProjectRoot)\Saved\Cooked\Windows\$($This.MsProjectName)\Content"
        $LsDestinationFolder = "$($This.MsProjectRoot)\Content"
        foreach($Item in $(Get-ChildItem $LsSourceFolder -Recurse -File)){
            $Extension = $Item.Extension.ToLower()
            if ($Extension -eq ".ushaderbytecode" -or $Extension -eq ".stinfo" -or $Extension -eq ".json") {
                $This.CopyIfNewer($Item, $LsDestinationFolder)
            }
        }
        
        $LsSourceFolder = "$($This.MsProjectRoot)\Saved\Cooked\Windows\Engine"
        $LsDestinationFolder = "$($This.MsProjectRoot)\Engine"
        foreach($Item in $(Get-ChildItem $LsSourceFolder -Recurse -File)){
            $This.CopyIfNewer($Item, $LsDestinationFolder)
        }
    }
}

function Show-Help {
    Write-Host "Usage: UECook.ps1 [-Development | -Shipping] [-Client | -Server | -BuildBoth]"
    Write-Host ""
    Write-Host "Options:"
    Write-Host "  -Development       Build in Development mode."
    Write-Host "  -Shipping          Build in Shipping mode."
    Write-Host "  -Client            Build the Client target."
    Write-Host "  -Server            Build the Server target."
    Write-Host "  -BuildBoth         Build both Client and Server targets."
    Write-Host ""
    Write-Host "Example:"
    Write-Host "  .\UECook.ps1 -Development -BuildBoth"
    exit 0
}

function Main 
{
    if (-not $RunParams.Development -and -not $RunParams.Shipping) {
        $RunParams.Development = $true
        Write-Host "Setting Development by default (NOT Shipping)"
    }

    if (-not $RunParams.Client -and -not $RunParams.Server -and -not $RunParams.P2P) {
        Write-Error "❌ Must have one of the following: Client, Server, P2P"
        Show-Help
        exit 1
    }

    $LoCore = [Core]::new($RunParams)

    if(-not $SkipVersionUpdate)
    {
        $LsCommand = $LoCore.GetAssetVersionUpdateCMD()
        Write-Host $LsCommand
        if(-not $OnlyPrint){
            Invoke-Expression $LsCommand
        }
    }
    
    [Core]::PrintLine()
    if($OnlyUpdateVersions){
        exit
    }

    [Core]::PrintLine()
    Write-Host "⏳ Starting Cook..."

    if($RunParams.Client -eq $true -or $RunParams.Server -eq $true)
    {
        $LsCommand = $LoCore.GetCookCMD()
        Write-Host $LsCommand
        
        if(-not $OnlyPrint){
            Invoke-Expression $LsCommand
        }
        [Core]::PrintLine()
    }

    
    if($RunParams.P2P -eq $true)
    {
        $LsCommand = $LoCore.GetCookP2PCMD()
        Write-Host $LsCommand
        
        if(-not $OnlyPrint){
            Invoke-Expression $LsCommand
        }
        [Core]::PrintLine()
    }
    
    if(-not $NoCopyFiles){
        $LoCore.CopyFiles()
    }

    if($RunParams.BuildAll) {
        Write-Host "✅ Both Client and Server builds completed."
    }elseif($RunParams.Client) {
        Write-Host "✅ Client build completed."
    }
    elseif($RunParams.Server) {
        Write-Host "✅ Server build completed."
    }
}

Write-Host "Order to Success"
Write-Host " 1. Set Changelist Version 'code D:\UE\Engines\UnrealEngine\Engine\Build\Build.version'"
Write-Host " 2. Open editor, resave all with new version then rebuild all levels"
Write-Host " 3. Run a git commit, because this script can break uassets if not versioned right"
Write-Host " 4. Run this script"

Main;

CopySteamAppID.ps1

