# Copywrite by Joel Leagues with the Apache v2 Licence

class Files
{
    [System.Collections.ArrayList] $files;

    Files()
    {
        $this.files = New-Object System.Collections.ArrayList;
        
        $this.files.add(".idea");
        $this.files.add(".vs");
        $this.files.add("Binaries");
        $this.files.add("Build");
        $this.files.add("Intermediate");
        $this.files.add("Saved");
        $this.files.add("DerivedDataCache");
        $this.files.add("ArchivedBuilds");
        $this.files.add("WindowsServer");
        $this.files.add("Plugins/EpicOnlineSubsystem/OnlineSubsystemRedpointEOS/Intermediate")
        $this.files.add("Plugins/VisualStudioTools/Binaries")
        $this.files.add("Plugins/VisualStudioTools/Intermediate")
        $this.files.add("Plugins/OnlineAPIExplorer/Binaries")
        $this.files.add("Plugins/OnlineAPIExplorer/Intermediate")
        $this.files.add("Plugins/HeadMountedVR/Binaries")
        $this.files.add("Plugins/HeadMountedVR/Intermediate")
        $this.files.add("Plugins/OculusVR/Binaries")
        $sln_file = $(Get-ChildItem | Where-Object { $_.Name -like "*.sln" }).Name
        if($sln_file){
            $this.files.add($sln_file);
        }
    }

    [void] Wipe()
    {
        $this.files.foreach({
            if(Test-Path($_))
            {
                Write-Host "Deleting: " $_
                Remove-Item $_ -Force -Recurse
            }
        });
    }
};


$files = [Files]::new();
$files.Wipe();
