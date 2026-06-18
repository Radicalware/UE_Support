# Ensure COM interop is available

param(
    [switch]$Game,
    [switch]$Server
)

if ($PSVersionTable.PSEdition -ne 'Desktop' -or $PSVersionTable.PSVersion -ge [Version]'5.2') {
    Write-Host "❌ This script requires Windows PowerShell 5.1 or lower. Exiting..."
    exit
}


$VS2026="18"
Add-Type -Path "C:\Program Files\Microsoft Visual Studio\$VS2026\Community\Common7\IDE\PublicAssemblies\Microsoft.VisualStudio.Interop.dll"


Add-Type -AssemblyName System.Runtime.InteropServices

$LbRunGame   = $Game   -or (-not $Game -and -not $Server)
$LbRunServer = $Server -or (-not $Game -and -not $Server)

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
    if($LbRunServer){
        Write-Debug "Starting Server..."
        $LsCommand = $(UELaunch.ps1 -server -NoExec)
        $LoCore.AttachToVisualStudio($LsCommand)
    }
    if($LbRunGame){
        Write-Debug "Starting Game..."
        $LsCommand = $(UELaunch.ps1 -game -NoExec)
        $LoCore.AttachToVisualStudio($LsCommand)
    }
} catch {
    Write-Host "❌ An error occurred: $($_.Exception.Message)"
}
