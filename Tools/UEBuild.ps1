# Copywrite by Joel Leagues with the Apache v2 Licence

param(
    [switch]$Game,
    [switch]$Server,
    [switch]$Editor,
    [switch]$CoreOnly,
    
    [switch]$DotNet,
    [switch]$UnrealTool,
    [switch]$MSBuild,
    [switch]$BuildBat,
    [switch]$RunUAT,

    [switch]$OnlyPrint
)

# Use UnrealBuildTool via dotnet. Replace Win64 if needed.
# •	Development Server: dotnet "D:\UE\Engines\UnrealEngine\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll" ProjMenu Win64 Development -TargetType=Server -Project="D:\UE\Dev\Apps\ProjMenu\ProjMenu.uproject" -NoHotReloadFromIDE
# •	Development Client: dotnet "D:\UE\Engines\UnrealEngine\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll" ProjMenu Win64 Development -TargetType=Client -Project="D:\UE\Dev\Apps\ProjMenu\ProjMenu.uproject" -NoHotReloadFromIDE
# • Development editor: dotnet "D:\UE\Engines\UnrealEngine\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll" ProjMenu Win64 Development -TargetType=Editor -Project="D:\UE\Dev\Apps\ProjMenu\ProjMenu.uproject" -NoHotReloadFromIDE

# dotnet "D:\UE\Engines\UnrealEngine\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll" UnrealEditor Win64 Development

if (-not (Get-ChildItem -Path $PWD.Path -Filter *.uproject) -and -not (Get-ChildItem -Path $PWD.Path -Filter *.sln)) {
    Write-Host "No .uproject or .sln file found in $($PWD.Path). Exiting."
    exit 1
}

set64

[Flags()]
enum BuildMethod 
{
    None = 0;
    DotNet = 1;
    UnrealTool = 2;
    MSBuild = 3;
    BuildBat = 4;
    RunUAT = 5
}

[Flags()]
enum TargetMethod 
{
    None = 0;
    Editor = 1;
    Game = 2;
    Server = 3;
}

class Option
{
    [bool] $DotNet
    [bool] $UnrealTool  
    [bool] $MSBuild
    [bool] $BuildBat
    [bool] $RunUAT
    [bool] $CoreOnly
    [BuildMethod]  $MeBuild  = [BuildMethod]::None
    [TargetMethod] $MeTarget = [TargetMethod]::None
}
$LoOption = [Option]::new()
$LoOption.DotNet = $DotNet
$LoOption.UnrealTool = $UnrealTool
$LoOption.MSBuild = $MSBuild
$LoOption.BuildBat = $BuildBat
$LoOption.RunUAT = $RunUAT
$LoOption.CoreOnly = $CoreOnly

if($DotNet){
    $LoOption.MeBuild = [BuildMethod]::DotNet;
}
if($UnrealTool){
    if($LoOption.MeBuild -ne [BuildMethod]::None){
        throw "Build Method Already Set"
    }
    $LoOption.MeBuild = [BuildMethod]::UnrealTool;
}
if($MSBuild){
    if($LoOption.MeBuild -ne [BuildMethod]::None){
        throw "Build Method Already Set"
    }
    $LoOption.MeBuild = [BuildMethod]::MSBuild;
}
if($BuildBat){
    if($LoOption.MeBuild -ne [BuildMethod]::None){
        throw "Build Method Already Set"
    }
    $LoOption.MeBuild = [BuildMethod]::BuildBat;
}
if($RunUAT){
    if($LoOption.MeBuild -ne [BuildMethod]::None){
        throw "Build Method Already Set"
    }
    $LoOption.MeBuild = [BuildMethod]::RunUAT;
}
# ----
if($Editor){
    $LoOption.MeTarget = [TargetMethod]::Editor;
}
if($Game){
    if($LoOption.MeTarget -ne [TargetMethod]::None){
        throw "Target Method Already Set"
    }
    $LoOption.MeTarget = [TargetMethod]::Game;
}
if($Server){
    if($LoOption.MeTarget -ne [TargetMethod]::None){
        throw "Target Method Already Set"
    }
    $LoOption.MeTarget = [TargetMethod]::Server;
}

if ($LoOption.MeBuild -eq [BuildMethod]::None) {
    Write-Host "Usage: UEBuild.ps1 [-Game] [-Server] [-Editor] [-DotNet] [-UnrealTool] [-MSBuild]"
    Write-Host "  -Game       : Build the game (DebugGame configuration)"
    Write-Host "  -Server     : Build the server (Debug configuration)"
    Write-Host "  -Editor     : Build the editor (Development configuration)"
    Write-Host "  -DotNet     : Build with DotNet"
    Write-Host "  -UnrealTool : Build with Unreal Tool"
    Write-Host "  -MSBuild    : Build with MSBuild"
    Write-Host "  -BuildBat   : Build using BuildBat"
    Write-Host "  -RunUAT     : Build using BuildBat"
    Write-Host "  -OnlyPrint  : Only Print, No Execute"
    exit 1
}

class Core
{
    static[string] $SsEnginePath   = "D:\UE\Engines\UnrealEngine"
    static[string] $SsBuildBat     = "D:\UE\Engines\UnrealEngine\Engine\Build\BatchFiles\Build.bat"
    static[string] $SsRunUATBat    = "D:\UE\Engines\UnrealEngine\Engine\Build\BatchFiles\RunUAT.bat"
    static[string] $SsBuildToolEXE = "D:\UE\Engines\UnrealEngine\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"
    static[string] $SsBuildToolDLL = "D:\UE\Engines\UnrealEngine\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll"
    static[string] $SsMSBuild   = "MSBuild.exe"
    static[string] $SsAssembly  = "Win64"
    static[string[]] $SsExtraUProjectArgs = @("-Progress",  "-NoHotReload")
    static[string[]] $SsExtraSourceArgs   = @("-WaitMutex", "-FromMsBuild")
    static[string[]] $SvExcludeTargetsMSBuild = @(
        "Android", "Apple", "BuildStorageTool", "ChaosVisualDebugger", "Datasmith", "RTFM", 
        "UBA", "Uba",  "Unsync", "SlateUGS", "SubmitTool", "UnrealLightmass")
    static[string[]] $SvExcludeTargetsBuildBat = @("RTFM","UBA", "Uba", "Datasmith"
    )

    [string] $MsSolutionName;
    [string] $MsUProject;
    [string] $MsProjectName;
    [string] $MsNestedProjectName;
    [bool]   $MbSourceBuild = $false

    [Option] $MoOption

    Core([Option] $FoOption)
    {
        $This.MoOption = $FoOption
        if (Test-Path "$($PWD.Path)\Source") {
            $This.MsUProject = "$($PWD.Path)\$($(Get-ChildItem | Select-Object Name | Where-Object Name -like *uproject).Name)";
            $This.MsSolutionName = [System.IO.Path]::GetFileNameWithoutExtension($This.MsUProject);
            # $This.MsNestedProjectName = [System.IO.Path]::GetFileNameWithoutExtension($This.MsUProject);
            $This.MbSourceBuild = $false
            $SourceDirs = Get-ChildItem -Path "$($PWD.Path)\Source" -Directory
            if ($SourceDirs.Count -eq 1) {
                $This.MsNestedProjectName = $SourceDirs[0].Name
            }
            $LvBasePath = $(Get-ChildItem -Path "$($PWD.Path)" -Filter *.sln -File)
            if($LvBasePath.Length -eq 0){
                Write-Host "No .sln file found. Generate Project Files"
                exit
            }
            $This.MsProjectName = $LvBasePath[0].BaseName
        }
        elseif (Test-Path "$($PWD.Path)\UE5.sln") {
            $This.MbSourceBuild = $true
            $This.MsNestedProjectName = "UE5"
            $This.MsProjectName = "UE5"
        }
        else {
            throw "You must be in a UProject dir or the Engine Code Dir"
        }
    }

    static [void] PrintFullLine() {
        $width = $(Get-Host).UI.RawUI.WindowSize.Width
        Write-Host ('=' * $width)
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

    [string] GetUTCommandBuildDevelopmentEditor()
    {
        $LsComand = ""
        $LsBuildType = "Development"
        if($This.MbSourceBuild)
        {
            $LsTarget = "UnrealEditor"
            $LsComand = [Core]::JoinWithSpaces(@(
                [Core]::SsBuildToolEXE,
                $LsTarget,
                [Core]::SsAssembly,
                $LsBuildType
            ) + [Core]::SsExtraSourceArgs)
        }
        else {
            $LsExeName = $This.MsNestedProjectName + "Editor"
            $LsProject = "-Project=" + $This.MsUProject
            $LsTarget = "-TargetType=Editor"

            $LsComand = [Core]::JoinWithSpaces(@(
                [Core]::SsBuildToolEXE,
                $LsExeName,
                [Core]::SsAssembly,
                $LsBuildType,
                $LsProject,
                $LsTarget
            ) + [Core]::SsExtraUProjectArgs)
        }
        
        Write-Host $LsComand "`n"
        return $LsComand
    }

    [string] GetDotNetCommandBuildDevelopmentEditor()
    {
        $LsComand = ""
        $LsBuildType = "Development"
        if($This.MbSourceBuild)
        {
            $LsTarget = "UnrealEditor"
            $LsComand = [Core]::JoinWithSpaces(@(
                "dotnet",
                [Core]::SsBuildToolDLL,
                $LsTarget,
                [Core]::SsAssembly,
                $LsBuildType
            ))
        }
        else {
            $LsExeName = $This.MsNestedProjectName
            $LsProject = "-Project=" + $This.MsUProject
            $LsTarget = "-TargetType=Editor"

            $LsComand = [Core]::JoinWithSpaces(@(
                "dotnet",
                [Core]::SsBuildToolDLL,
                $LsExeName,
                [Core]::SsAssembly,
                $LsBuildType,
                $LsTarget,
                $LsProject
            ) + @("-NoHotReloadFromIDE"))
        }
        
        Write-Host $LsComand "`n"
        return $LsComand
    }

    [string] GetUTCommandBuildDevelopmentGame()
    {
        $LsComand = ""
        if($This.MbSourceBuild)
        {
            $LsTarget = "UnrealGame"
            $LsComand = [Core]::JoinWithSpaces(@(
                [Core]::SsBuildToolEXE,
                $LsTarget,
                [Core]::SsAssembly,
                "Development" # Debug / Development
            ) + [Core]::SsExtraSourceArgs)
        }
        else {
            $LsExeName = $This.MsNestedProjectName
            $LsBuildType = "Development" # DebugGame / Development
            $LsProject = "-Project=" + $This.MsUProject
            $LsTarget = "-TargetType=Game"

            $LsComand = [Core]::JoinWithSpaces(@(
                [Core]::SsBuildToolEXE,
                $LsExeName,
                [Core]::SsAssembly,
                $LsBuildType,
                $LsProject,
                $LsTarget
            ) + [Core]::SsExtraUProjectArgs)
        }
        Write-Host $LsComand "`n"
        return $LsComand
    }

    [string] GetDotNetCommandBuildDevelopmentGame()
    {
        $LsComand = ""
        $LsBuildType = "Development"
        if($This.MbSourceBuild)
        {
            $LsTarget = "UnrealClient"
            $LsComand = [Core]::JoinWithSpaces(@(
                "dotnet",
                [Core]::SsBuildToolDLL,
                $LsTarget,
                [Core]::SsAssembly,
                $LsBuildType
            ))
        }
        else {
            $LsExeName = $This.MsNestedProjectName
            $LsProject = "-Project=" + $This.MsUProject
            $LsTarget = "-TargetType=Client"

            $LsComand = [Core]::JoinWithSpaces(@(
                "dotnet",
                [Core]::SsBuildToolDLL,
                $LsExeName,
                [Core]::SsAssembly,
                $LsBuildType,
                $LsTarget,
                $LsProject
            ) + @("-NoHotReloadFromIDE"))
        }
        
        Write-Host $LsComand "`n"
        return $LsComand
    }    

    [string] GetUTCommandBuildDevelopmentServer()
    {
        $LsComand = ""
        if($This.MbSourceBuild)
        {
            # UnrealBuildTool.exe UE5Server Win64 Debug -WaitMutex -FromMsBuild
            $LsTarget = "UnrealServer"
            $LsComand = [Core]::JoinWithSpaces(@(
                [Core]::SsBuildToolEXE,
                $LsTarget,
                [Core]::SsAssembly,
                "Development" # Debug / Development
            ) + [Core]::SsExtraSourceArgs)
        }
        else {
            $LsExeName = $This.MsNestedProjectName + "Server"
            $LsBuildType = "Development" # DebugGame / Development
            $LsProject = "-Project=" + $This.MsUProject
            $LsTarget = "-TargetType=Server"

            $LsComand = [Core]::JoinWithSpaces(@(
                [Core]::SsBuildToolEXE,
                $LsExeName,
                [Core]::SsAssembly,
                $LsBuildType,
                $LsProject,
                $LsTarget
            ) + [Core]::SsExtraUProjectArgs)
        }
        Write-Host $LsComand "`n"
        return $LsComand
    }

    [string[]] GetFilteredProjectNames([string[]] $FvAvoid)
    {
        $LvProjectNames = $(Get-ChildItem "$($PWD.Path)\Engine\Intermediate\ProjectFiles" -File | Where-Object Name -Like "*.vcxproj").FullName
        $LvFilteredProjectNames=@()
        foreach($LsProject in $LvProjectNames){
            $LbAdd=$true
            foreach($LsAvoid in $FvAvoid){
                if($LsProject.Contains($LsAvoid)){
                    $LbAdd=$false
                    break;
                }
            }
            if($LbAdd){
                $LvFilteredProjectNames += $LsProject
            }
        }
        return $LvFilteredProjectNames;
    }

    [string] GetDotNetCommandBuildDevelopmentServer()
    {
        $LsComand = ""
        $LsBuildType = "Development"
        if($This.MbSourceBuild)
        {
            # UnrealBuildTool.exe UnrealEditor Win64 Development -WaitMutex -FromMsBuild
            $LsTarget = "UnrealEditor"
            $LsComand = [Core]::JoinWithSpaces(@(
                "dotnet",
                [Core]::SsBuildToolDLL,
                $LsTarget,
                [Core]::SsAssembly,
                $LsBuildType
            ))
        }
        else {
            $LsExeName = $This.MsNestedProjectName
            $LsProject = "-Project=" + $This.MsUProject
            $LsTarget = "-TargetType=Server"

            $LsComand = [Core]::JoinWithSpaces(@(
                "dotnet",
                [Core]::SsBuildToolDLL,
                $LsExeName,
                [Core]::SsAssembly,
                $LsBuildType,
                $LsTarget,
                $LsProject
            ) + @("-NoHotReloadFromIDE"))
        }
        
        Write-Host $LsComand "`n"
        return $LsComand
    }  

    [string[]] GetRunMSBuild()
    {
        $LsSolutionPath = "$($PWD.Path)\$($This.MsProjectName).sln"

        $LvRetCommands=@()
        $LbUnorderedBuild = $true
        if($LbUnorderedBuild)
        {
            $LvFilteredProjectNames=$This.GetFilteredProjectNames([Core]::SvExcludeTargetsMSBuild)
            $LbOnlyMainBuild=$($This.MoOption.MeTarget -ne [TargetMethod]::None)

            foreach($LsProjectName in $LvFilteredProjectNames)
            {
                $LbSkip=$LbOnlyMainBuild # plan to skip if only main build
                $LvTargetConfigs = @()
                if($LsProjectName.Contains("UE5"))
                {
                    $LbSkip=$false #no skip
                    if($This.MoOption.MeTarget -eq [TargetMethod]::Editor){
                        $LvTargetConfigs += "Development_Editor"
                    }elseif($This.MoOption.MeTarget -eq [TargetMethod]::Game){
                        $LvTargetConfigs += "Development_Client"
                    }elseif ($This.MoOption.MeTarget -eq [TargetMethod]::Server) {
                        $LvTargetConfigs += "Development_Server"
                    }else {
                        $LvTargetConfigs += "Development_Editor"    # UnrealEditor
                        $LvTargetConfigs += "Development_Client"    # UnrealGame
                        $LvTargetConfigs += "Development_Server"    # UnrealServer
                    }
                }
                elseif ($LsProjectName.Contains("LiveLinkHub")) {
                    $LvTargetConfigs += "Development_Editor"
                }
                elseif($LsProjectName -eq "AutomationTools.Tests"){
                    $LvTargetConfigs += "Release"
                }else{
                    $LvTargetConfigs += "Development"
                }

                if($LbSkip){
                    continue
                }

                foreach($LsTarget in $LvTargetConfigs)
                {
                    $LsComand = [Core]::JoinWithSpaces(@(
                        [Core]::SsMSBuild,
                        $LsProjectName,
                        "/t:Build",
                        "/p:Configuration=`"$LsTarget`"",
                        "/m",
                        "/p:TreatWarningsAsErrors=false /p:NoWarn=NU1900 /p:NoWarn=NU1901 /p:NoWarn=NU1902 /p:NoWarn=NU1903 /p:NoWarn=NU1904"
                    ))
                    $LvRetCommands += $LsComand
                }
            }
        }else{
            $LsComand = [Core]::JoinWithSpaces(@(
                [Core]::SsMSBuild,
                $LsSolutionPath
                "/p:Configuration=Development",
                "/p:Platform=$([Core]::SsAssembly)",
                "/p:SolutionDir=$($PWD.Path)\",
                "/v:minimal",
                "/p:TreatWarningsAsErrors=false /p:NoWarn=NU1900 /p:NoWarn=NU1901 /p:NoWarn=NU1902 /p:NoWarn=NU1903 /p:NoWarn=NU1904",
                "/p:Exclude=UbaExceptionHandler /p:Exclude=UbaCoordinatorHorde /p:Exclude=AutoRTFMTests /p:Exclude=AutoRTFMTestsWithExceptions /p:Exclude=AutoRTFMTestsWithMergeModules /p:Exclude=AutoRTFMTestsWithSTATS "
                # "/m", # can't resolve dependencines so no multi-threading
            ))
            $LvRetCommands += $LsComand
        }
        return $LvRetCommands
    }

    [string[]] GetRunBuildBat()
    {
        $LsSolutionPath="$($PWD.Path)\$($This.MsProjectName).sln"
        $LvRetCommands=@()
        
        $LvTargetConfigs = @()
        $LbBuildAll = $This.MoOption.MeTarget -eq [TargetMethod]::None
        if($This.MbSourceBuild)
        {
            $LvFilteredProjectNames=$This.GetFilteredProjectNames([Core]::SvExcludeTargetsBuildBat).ForEach({ $_ -replace ".*\\", "" -replace '.vcxproj','' });
            #$LvFilteredProjectNames += "DatasmithMax2026" # requires 3DS Max SDK

            foreach($LsProjectName in $LvFilteredProjectNames)
            {
                $LbBaseEngineProj=$LsProjectName.Contains("UE5")
                if(-not $LbBaseEngineProj){
                    continue; # not the base engine and we are to skip
                }

                if($LbBaseEngineProj)
                {
                    if($LbBuildAll){ # None specified means all
                        $LvTargetConfigs += "-Target=`"Unreal Win64 Development`" -ProjectFiles"
                        $LvTargetConfigs += "-Target=`"UnrealEditor Win64 Development`" -ProjectFiles"
                        $LvTargetConfigs += "-Target=`"UnrealGame Win64 Development`" -ProjectFiles"
                        $LvTargetConfigs += "-Target=`"UnrealServer Win64 Development`" -ProjectFiles"
                        
                        $LvTargetConfigs += "-Target=`"Unreal Win64 Development`""
                    }

                    if(($This.MoOption.MeTarget -eq [TargetMethod]::Editor) -Or $LbBuildAll){
                        $LvTargetConfigs += "-Target=`"UnrealEditor Win64 Development`""
                    }
                    if(($This.MoOption.MeTarget -eq [TargetMethod]::Game) -Or $LbBuildAll){
                        $LvTargetConfigs += "-Target=`"UnrealGame Win64 Development`""
                    }
                    if(($This.MoOption.MeTarget -eq [TargetMethod]::Server) -Or $LbBuildAll){
                        $LvTargetConfigs += "-Target=`"UnrealServer Win64 Development`""
                    }
                }
                elseif ($LsProjectName.Contains("LiveLinkHub")) {
                    $LvTargetConfigs += "-Target=`"LiveLinkHub Win64 Development`""
                }else{
                    $LvTargetConfigs += "$LsProjectName Win64 Development"
                }
            }
        }else{

            $LsProject = $(Get-ChildItem *.uproject).FullName
            if($LbBuildAll -and -not $This.MoOption.CoreOnly){ # None specified means all
                $LvTargetConfigs += "TheGame Win64 Development $LsProject -ProjectFiles"                
                $LvTargetConfigs += "TheGameEditor Win64 Development $LsProject -ProjectFiles"
                $LvTargetConfigs += "TheGameServer Win64 Development $LsProject -ProjectFiles"
                $LvTargetConfigs += "TheGameClient Win64 Development $LsProject -ProjectFiles"
                
                $LvTargetConfigs += "TheGame Win64 Development $LsProject"
            }

            if(($This.MoOption.MeTarget -eq [TargetMethod]::Editor) -Or $LbBuildAll){
                $LvTargetConfigs += "TheGameEditor Win64 Development $LsProject"
            }
            if(($This.MoOption.MeTarget -eq [TargetMethod]::Game) -Or $LbBuildAll){
                $LvTargetConfigs += "TheGameClient Win64 Development $LsProject"
            }
            if(($This.MoOption.MeTarget -eq [TargetMethod]::Server) -Or $LbBuildAll){
                $LvTargetConfigs += "TheGameServer Win64 Development $LsProject"
            }
        }
         
        foreach($LsTarget in $LvTargetConfigs)
        {
            $LsComand = [Core]::JoinWithSpaces(@(
                [Core]::SsBuildBat,
                $LsTarget
                ))
            $LvRetCommands += $LsComand
        }

        return $LvRetCommands
    }

    
    [string[]] GetRunUAT()
    {
        $LsProjectPath="$($PWD.Path)\$($This.MsProjectName).uproject"
        # RunUAT.bat BuildCookRun -project="D:\UE\Dev\Apps\TheGame\TheGame.uproject" -noP4 -build -platform=Win64 -clientconfig=Development -target=TheGame

        $LsComand = [Core]::JoinWithSpaces(@(
            [Core]::SsRunUATBat,
            "BuildCookRun",
            "-project=`"$LsProjectPath`"",
            "-noP4",
            "-build",
            "-platform=$([Core]::SsAssembly)",
            "-clientconfig=Development",
            "-target=$($This.MsProjectName)"
        ));

        return $LsComand
    }
};

try 
{
    if($null -eq $(Get-Command MSBuild.exe -ErrorAction SilentlyContinue)) {
        throw "MSBuild.exe not found in PATH. Run vcvarsall.bat and map to Powershell `$env (DevHint: Set-64)."
    }
    
    $LoCore = [Core]::new($LoOption)
    if($LoOption.BuildBat)
    {
        $LvCommands = $LoCore.GetRunBuildBat()
        foreach($LsCommand in $LvCommands)
        {
            [Core]::PrintFullLine()
            Write-Host "$LsCommand`n";
            Invoke-Expression $LsCommand
        }
        [Core]::PrintFullLine()
    }
    elseif($LoOption.RunUAT)
    {
        $LvCommands = $LoCore.GetRunUAT()
        foreach($LsCommand in $LvCommands)
        {
            [Core]::PrintFullLine()
            Write-Host "$LsCommand`n";
            Invoke-Expression $LsCommand
        }
        [Core]::PrintFullLine()
    }
    elseif($LoOption.MSBuild)
    {
        $LvCommands = $LoCore.GetRunMSBuild()
        foreach($LsCommand in $LvCommands)
        {
            [Core]::PrintFullLine()
            Write-Host "$LsCommand`n";
            Invoke-Expression $LsCommand
        }
        [Core]::PrintFullLine()
    }
    elseif($LoOption.UnrealTool)
    {
        if ($Editor) {
            [Core]::PrintFullLine()
            $LsCommand = $LoCore.GetUTCommandBuildDevelopmentEditor()
        }
        if ($Game) {
            [Core]::PrintFullLine()
            $LsCommand = $LoCore.GetUTCommandBuildDevelopmentGame()
        }
        if ($Server) {
            [Core]::PrintFullLine()
            $LsCommand = $LoCore.GetUTCommandBuildDevelopmentServer()
        }
    }
    elseif($LoOption.DotNet)
    {
        if ($Editor) {
            [Core]::PrintFullLine()
            $LsCommand = $LoCore.GetDotNetCommandBuildDevelopmentEditor()
        }
        if ($Game) {
            [Core]::PrintFullLine()
            $LsCommand = $LoCore.GetDotNetCommandBuildDevelopmentGame()
        }
        if ($Server) {
            [Core]::PrintFullLine()
            $LsCommand = $LoCore.GetDotNetCommandBuildDevelopmentServer()
        }
    }
    else{
        Write-Host "No Build Utility [DotNet, UnrealTool, MSBuild]" 
        [Core]::PrintFullLine()
        return;
    }

    if(-not $OnlyPrint -and -not $MSBuild){
        Invoke-Expression $LsCommand
    }
    [Core]::PrintFullLine()
    
    CopySteamAppID.ps1
}
catch {
    Write-Host "An error occurred: $_"
}


