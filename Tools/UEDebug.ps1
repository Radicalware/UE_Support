# Copywrite by Joel Leagues with the Apache v2 Licence

param(
    [switch]$Client,
    [switch]$Server,
    [switch]$P2P
)

if ($Help -or (-not $Client -and -not $Server -and -not $P2P)) { 
    Write-Host @"
    Usage: UEDebug.ps1 [options]

    Options:
    -Client    Start Unreal Editor with game debugging
    -Server    Start Unreal Editor with server debugging
    -P2P       Start Unreal Editor with -P2P debugging
    -Help      Display this help message

    Examples:
    UEDebug.ps1 -Client
    UEDebug.ps1 -Server
    UEDebug.ps1 -P2P
"@
    exit
}

if ($PSVersionTable.PSEdition -ne 'Desktop' -or $PSVersionTable.PSVersion -ge [Version]'5.2') {
    Write-Host "❌ This script requires Windows PowerShell 5.1 or lower. Exiting..."
    exit
}

$VS2026="18"
Add-Type -Path "C:\Program Files\Microsoft Visual Studio\$VS2026\Community\Common7\IDE\PublicAssemblies\Microsoft.VisualStudio.Interop.dll"

Add-Type -AssemblyName System.Runtime.InteropServices

class Core
{
    static[string] $SsProjId = "VisualStudio.DTE.$VS2026.0"
    [string] $MsUProject;

    Core()
    {
        $This.MsUProject = "$($PWD.Path)\$($(Get-ChildItem | Select-Object Name | Where-Object Name -like *uproject).Name)";
    }

    [void] AttachToVisualStudio([string]$FsCommand)
    {
        Write-Host $FsCommand
        try {
            $Exe, $Args = $FsCommand -split '\s+', 2
            $LoProcess = Start-Process -FilePath $Exe -ArgumentList $Args -PassThru
            $LnGamePID = $LoProcess.Id
            Write-Host "Unreal Editor started with PID: $LnGamePID"
            $LoDTE = [System.Runtime.InteropServices.Marshal]::GetActiveObject([Core]::SsProjId)
            $LoProc = $LoDTE.Debugger.LocalProcesses | Where-Object { $_.ProcessID -eq $LnGamePID }
            if ($LoProc) {
                $LoProc.Attach()
                Write-Host "🟢 Successfully attached Visual Studio debugger to process ID $LnGamePID"
            } else {
                Write-Host "⚠️ Process ID $LnGamePID not found in Visual Studio's debugger list"
            }
        }
        catch {
            Write-Host "❌ Failed to connect to Visual Studio. Is it running?"
            Write-Host "Details: $($_.Exception.Message)"
        }
    }
}

try
{
    $LoCore = [Core]::new()
    $LsCommand = ""
    if($Client){
        Write-Debug "Starting Game..."
        $LsCommand = $(UELaunch.ps1 -clent -NoExec -First)
        $LoCore.AttachToVisualStudio($LsCommand)
    }
    if($Server){
        Write-Debug "Starting Server..."
        $LsCommand = $(UELaunch.ps1 -server -NoExec -First)
        $LoCore.AttachToVisualStudio($LsCommand)
    }
    if($P2P){
        Write-Debug "Starting Game..."
        $LsCommand = $(UELaunch.ps1 -P2P -NoExec -First)
        $LoCore.AttachToVisualStudio($LsCommand)
    }
} catch {
    Write-Host "❌ An error occurred: $($_.Exception.Message)"
}
