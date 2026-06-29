# Copywrite by Joel Leagues with the Apache v2 Licence

param (
    [string] $Map,
    [string] $IpAddress = "127.0.0.1",
    [string] $Port = "7777",
    [switch] $Log,

    [switch] $Second,
    [switch] $Third,

    [switch] $Client,
    [switch] $Server,
    [switch] $P2P,
    [switch] $Editor,
    [switch] $Lite,

    [switch] $NoSteam,
    [switch] $NoExec,

    [switch] $help
)

if($help -eq $true)
{
    write-host "

UServer.ps1
    
    -Project   = Project Name (not required)
    -Map       = Give a Map Name (as a Server)
    -IpAddress = Give an Server IP   (Default = 127.0.0.1)
    -Port      = Give an Server Port (Default = 7777)

    -Client    = start as a client
    -Server    = start as a server
    -P2P       = start as a P2P
    -Editor    = Open the UE5 Editor 
    -Lite      = Runs using the Editor Lite build & not a Binary 

    -First     = Open game on the First Screen (default)
    -Second    = Open game on the second Screen
    -Third     = Open game on the third Screen

    -NoSteam   = Don't use Steam API

    "
    return;
}

if (-not (Get-ChildItem -Path $PWD.Path -Filter *.uproject)) {
    Write-Host "No .uproject file found in $($PWD.Path). Exiting."
    exit 1
}

$LnDisplay = 0
if($Third){
    $LnDisplay = 3
}elseif($Second){
    $LnDisplay = 2
}else{
    $LnDisplay = 1
}

enum Mode {
    Client
    Server
    P2P
    Editor
    None
}

$LeMode = [Mode]::None
if($Client){
    $LeMode = [Mode]::Client
}elseif($Server){
    $LeMode = [Mode]::Server
}elseif($P2P){
    $LeMode = [Mode]::P2P
}elseif($Editor){
    $LeMode = [Mode]::Editor
}else{
    throw "No Valid Mode"
}

class Core
{
    static[string] $SsEditor = "D:\UE\Engines\UnrealEngine\Engine\Build\Archive\Win64\TheGame\Binaries\Win64\UnrealEditor.exe"
    static[string] $SsPort = "7777"
    
    static[string[]] $SvDebugArgs  = @("-log", "-stdout", "-FullStdOutLogOutput")
    static[string[]] $SvGameArgs   = @("-windowed", "-resx=1280", "-resy=720")
    static[string[]] $SvServerArgs = @("-unattended")
    
    [boolean] $MbLite;
    [Mode]    $MeMode;
    [string]  $MsMode;
    [string]  $MsUProject;
    [string]  $MsDisplay;
    [string]  $MsNoSteam;
    [string]  $MsGameName;
    [string]  $MsLog;
    [bool]    $MbHasDisplay;

    Core([bool] $FeLite, [Mode] $FeMode, [int] $FnDisplay, [bool] $FbNoSteam, [bool] $FbLog
    ){
        $This.MbLite = $FeLite
        $This.MeMode = $FeMode
        $This.MsMode = "-" + $FeMode.ToString()
        $This.MbHasDisplay = $This.MeMode -eq [Mode]::Game -or $This.MeMode -eq [Mode]::P2P -or $This.MeMode -eq [Mode]::Editor
        if($This.MbHasDisplay)
        {
            if($FnDisplay -eq 2){
                $This.MsDisplay = "-WinX=1920 -WinY=0 -SAVEWINPOS=1"
            }elseif($FnDisplay -eq 3){
                $This.MsDisplay = "-WinX=3840 -WinY=1 -SAVEWINPOS=2"
            }else{
                $This.MsDisplay = "-SAVEWINPOS=0"
            }
        }

        if ($This.FbNoSteam -and $This.MsNoSteam.Length -gt 0) {
            $This.MsNoSteam = " -nosteam"
        }

        # switch($This.MeMode.ToString()){
        #     "Editor" { $This.MsLog = "-log=Editor.log"; break; }
        #     "Game"   { $This.MsLog = "-log=Game.log";   break; }
        #     "Server" { $This.MsLog = "-log=Server.log"; break; }
        #     default { throw "No Valid Launch Option" }
        # }

        # Current PWSH does not support
        # switch -Exact ($This.MeMode){
        #     [Mode]::Editor { $This.MsLog = "-log=Editor.log"; break; }
        #     [Mode]::Game   { $This.MsLog = "-log=Game.log";   break; }
        #     [Mode]::Server { $This.MsLog = "-log=Server.log"; break; }
        #     default { throw "No Valid Launch Option" }
        # }

        $This.MsUProject = "$($PWD.Path)\$($(Get-ChildItem | Select-Object Name | Where-Object Name -like *uproject).Name)";
        $This.MsGameName = (Get-ChildItem -Path "$($PWD.Path)\Source" -Directory)[0].Name
    }

    [string] GetUMapPath([string] $FsMap)
    {
        # Used by Clients
        $LsBasePath=$("$($(Get-Location).Path)\Content\*" -replace '\\','\\')
        $LsFullMapPath=$(Get-ChildItem ".\Content" -Recurse -Filter "$FsMap.umap" |
            Where-Object { $_.FullName -match "$LsBasePath" } |
            Select-Object -ExpandProperty FullName)
        return $LsFullMapPath
    }

    [String] GetLevelPath([string] $FsMap)
    {
        return $This.GetUMapPath([string] $FsMap) `
            -replace ".*\\Content\\", "\Game\" `
            -replace "\\", "/" `
            -replace ".umap", "" 
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

    [string] GetServerEditorCMD([string] $FsMap)
    {
        throw "not updated to use correct map"
        $LsMap = $FsMap -replace "\\", "/" -replace "./Content", "/Game" -replace ".umap", "?Listen"
        $LsPort = "-port=" + [Core]::SsPort

        $LvExecution = @([Core]::SsEditor, $This.MsUProject)
        if ($LsMap -and $LsMap.Length -gt 0) {
            $LvExecution += $LsMap
        }
        $LvExecution += @(
            $This.MsMode,
            "-server", # not needed for server binary as it is implied
            $LsMap,
            $LsPort
        )
        if ($This.MsNoSteam -and $This.MsNoSteam.Length -gt 0) {
            $LvExecution += $This.MsNoSteam
        }
        $LvExecution += [Core]::SvDebugArgs
        return [Core]::JoinWithSpaces($LvExecution);
    }

    [string] GetClientEditorCMD([string] $FsMap, [string] $IpAddress)
    {
        throw "not updated to use correct map"
        $LsMap = $FsMap -replace "\\", "/" -replace "./Content", "/Game" -replace ".umap", ""
        if($IpAddress.Length -eq 0){
            $IpAddress = "127.0.0.1:" + [Core]::SsPort
        }
        else{
            $IpAddress += ':' + [Core]::SsPort
        }
        $LvExecution = @([Core]::SsEditor, $This.MsUProject)
        if ($LsMap -and $LsMap.Length -gt 0) {
            $LvExecution += "$($LsMap)?$($IpAddress)"
        }
        else{
            $LvExecution += $IpAddress
        }
        $LvExecution += @(
            $This.MsMode,
            $This.MsDisplay
        )
        if($This.MsNoSteam.Length){
            $LvExecution += $This.MsNoSteam
        }
        $LvExecution += [Core]::SvDebugArgs
        return [Core]::JoinWithSpaces($LvExecution);
    }

    [string] CommandEditorStr([string] $FsMap, [string] $IpAddress)
    {
        $LsCommandStr = ""
        if($This.MeMode -eq [Mode]::Client){
            Write-Host "Warning: this is not likely to work" -ForegroundColor Yellow
            $LsCommandStr = $This.GetClientEditorCMD($This.GetUMapPath($FsMap), $IpAddress)
        }
        elseif($This.MeMode -eq [Mode]::Server){
            Write-Host "Warning: this is not likely to work" -ForegroundColor Yellow
            $LsCommandStr = $This.GetServerEditorCMD($This.GetLevelPath($FsMap))
        }
        elseif($This.MeMode -eq [Mode]::P2P){
            throw "Not yet developed a P2P for PIE"
        }
        return $LsCommandStr;
    }

    [string] GetServerDebugCMD([string] $FsMap)
    {
        # $LsBinary = ".\Build\Archive\Win64\TheGame\Binaries\Win64\{0}Server-Win64-DebugGame.exe" -f $This.MsGameName
        $LsBinary = ".\Build\Archive\Win64\TheGame\Binaries\Win64\{0}Server.exe" -f $This.MsGameName
        $LsPort = "-port=" + [Core]::SsPort
        $LvExecution = @()
        if($FsMap.Length -gt 0)
        {
            #$ListenMap = $FsMap + '?' + "Listen" # no ?Listen on a server, it's implied
            $LvExecution = @(
                $LsBinary,
                $FsMap,
                $LsPort
            )
        }else{
            $LvExecution = @(
                $LsBinary,
                $LsPort
            )
        }

        $LvExecution += [Core]::SvServerArgs
        $LvExecution += [Core]::SvDebugArgs
        return [Core]::JoinWithSpaces($LvExecution);
    }

    [string] GetP2PDebugCMD([string] $FsMap)
    {
        $LsBinary = ".\Build\Archive\Win64\TheGame\Binaries\Win64\{0}.exe" -f $This.MsGameName
        $LsPort = "-port=" + [Core]::SsPort
        $LvExecution = @()
        if($FsMap.Length -gt 0)
        {
            #$ListenMap = $FsMap + '?' + "Listen" # no ?Listen on a server, it's implied
            $LvExecution = @(
                $LsBinary,
                $FsMap,
                $LsPort
            )
        }else{
            $LvExecution = @(
                $LsBinary,
                $LsPort
            )
        }

        $LvExecution += [Core]::SvServerArgs
        $LvExecution += [Core]::SvDebugArgs
        return [Core]::JoinWithSpaces($LvExecution);
    }

    [string] GetClientDebugCMD([string] $FsMap, [string] $IpAddress)
    {
        # $LsBinary = ".\Build\Archive\Win64\TheGame\Binaries\Win64\{0}-Win64-DebugGame.exe" -f $This.MsGameName
        $LsBinary = ".\Build\Archive\Win64\TheGame\Binaries\Win64\{0}Client.exe" -f $This.MsGameName
        $LsPort = "-port=" + [Core]::SsPort
        $LvExecution = @(
            $LsBinary,
            $FsMap,
            $LsPort
        )
        $LvExecution += [Core]::SvGameArgs
        $LvExecution += [Core]::SvDebugArgs
        return [Core]::JoinWithSpaces($LvExecution);
    }
    
    [string] CommandDebugStr([string] $FsMap, [string] $IpAddress)
    {
        $LsCommandStr = ""
        if($This.MeMode -eq [Mode]::Client){
            $LsCommandStr = $This.GetClientDebugCMD($This.GetUMapPath($FsMap), $IpAddress)
        }
        elseif($This.MeMode -eq [Mode]::Server){
            $LsCommandStr = $This.GetServerDebugCMD($This.GetLevelPath($FsMap))
        }
        elseif($This.MeMode -eq [Mode]::P2P){
            $LsCommandStr = $This.GetP2PDebugCMD($This.GetLevelPath($FsMap))
        }

        return $LsCommandStr;
    }

    [string] GetCommandStr([string] $Map, [string] $IpAddress)
    {
        if($This.MbLite){
            return $This.CommandEditorStr($Map, $IpAddress);
        }
        return $This.CommandDebugStr($Map, $IpAddress);
    }
}

CopySteamAppID.ps1

$core = [Core]::new($Lite, $LeMode, $LnDisplay, $NoSteam, $Log);

$LsCommand = $core.GetCommandStr($Map, $IpAddress);

if($NoExec){
    return $LsCommand
}
else{
    Write-Host $LsCommand "`n"
    Invoke-Expression $LsCommand
}