# Copywrite by Joel Leagues with the Apache v2 Licence


# TODO: correct the special prefix types to the DefaultEngine.ini

param (
    [string] $OldProjectName,
    [string] $NewProjectName,

    [string] $OldPrefixName,
    [string] $NewPrefixName,

    [string[]] $SpecialPrefix
)

if(
    $NewProjectName.Length -eq 0 -or
    $OldProjectName.Length -eq 0
)
{
    Write-Host "Add the following variables"
    Write-Host "---------------------------------"
    Write-Host "-OldProjectName"
    Write-Host "-NewProjectName"
    Write-Host "-OldPrefixName"
    Write-Host "-NewPrefixName"
    Write-Host
    Write-Host "Example 1: RenameProject.ps1 -OldProjectName ShooterGame     -NewProjectName TheGame -OldPrefix Shoooter -NewPrefixName x"
    Write-Host "Example 2: RenameProject.ps1 -OldProjectName PuzzlePlatforms -NewProjectName TheGame                     -NewPrefixName x"
    Write-Host
    Write-Host "Note: You need to make sure your project is in the folder of your new project name"
    Write-Host "      You don't want your project to be in the folder of its former name when running this script!"
    exit
}
Write-Host "[+] Starting!"


# Holds the files that need to be renamed
class FilesObj
{
    [System.Collections.ArrayList] $Projects # Files that have the project names swapped: ex: ShooterGame
    [System.Collections.ArrayList] $Prefixes # files that have prefixes that get swapped: ex: Shooter
    [System.Collections.ArrayList] $Articals # When we give an article instead of swapping the project name: ex ShooterGamePlayerState >> ThePlayerState
    [bool] $HasOldPrefixName

    FilesObj()
    {
        $this.Projects = [System.Collections.ArrayList]::new()
        $this.Prefixes = [System.Collections.ArrayList]::new()
        $this.Articals = [System.Collections.ArrayList]::new()
        $this.HasOldPrefixName = $false
    }
}

# Holds our current and target object naming scheme
class Target
{
    static [string] $OldProjectName
    static [string] $NewProjectName

    static [string] $OldPrefixName
    static [string] $NewPrefixName

    static [bool]   $HadOldPrefix
    static [string] $OverwriteName # Is the Prefix name if there is an old prefix given, else the project Name

    static [string] $ProjectPathName # working directory

    [void] SetProjectPath()
    {
        [Target]::ProjectPathName = $PWD.Path + '\'
        if([Target]::ProjectPathName.Contains(' ') -eq $true)
        {
            Write-Host "Your PWD can't have a space in it!"
            exit
        }        
    }
}

# Holds the Old and New Object Names
class RedirectKVP 
{
    [String] $OldObjName
    [string] $NewObjName
    [string] $BasePath
    [string] $Extension
    [bool] $Used

    RedirectKVP(){ $this.Used = $false }
    RedirectKVP([string] $OldFilePath, [string] $Old, [string] $New)
    {
        $this.OldObjName = $Old
        $this.NewObjName = $New
        $this.BasePath = [Regex]::new("^.*\\").match($OldFilePath).Value
        $this.Extension = [Regex]::new("\.(h|cpp)$").match($OldFilePath).Value
        $this.Used    = $true
    }

    [string] OldFilePath()
    {
        return $_.BasePath + $_.OldObjName + $_.Extension
    }

    [string] NewFilePath()
    {
        return $_.BasePath + $_.NewObjName + $_.Extension
    }

    [string] MoveCommand()
    {
        return "Move-Item `"" + $this.OldFilePath() + "`" `"" + $this.NewFilePath() + "`""
    }
}

# Holds the GameEngine.ini Redirects
class RedirectsObj : Target
{
    [string] $GameNameRedirect
    [System.Collections.ArrayList] $ClassRedirects
    [System.Collections.ArrayList] $MoveOperations
    [Regex] $HeaderFileRegexMatch
    [Regex] $FileNameRegexExtract
    static [string] $RedirectString
    
    RedirectsObj()
    {
        $this.SetProjectPath()
        $this.ClassRedirects = [System.Collections.ArrayList]::new()
        $this.MoveOperations = [System.Collections.ArrayList]::new()
        $this.HeaderFileRegexMatch = [Regex]::new("\\[^\\]*\.(h|cpp)$")
        $this.FileNameRegexExtract = [Regex]::new("(?<=\\)([^\\]+)(?=\.(h|cpp))")

        $this.MoveOperations = [System.Collections.ArrayList]::new()
        $this.GameNameRedirect = `
            $([RedirectsObj]::GetGameNameRedirectString([Target]::OldProjectName, [Target]::NewProjectName)) + "`n" +
            $([RedirectsObj]::GetGameNameRedirectString([Target]::OldProjectName, [Target]::NewProjectName) -replace '"/Game/', '"/Script/')
        $this.MoveOperations.Add($("Move-Item " + $this.wd + "\Source\" + [Target]::OldProjectName + " " + $this.wd + "\Source\" + [Target]::NewProjectName))
    }

    static [string] GetGameNameRedirectString([string] $oldName, [string] $newName)
    {
        $oldName = $oldName -replace "^(.?)Content.", "" -replace '\\', '/'
        $newName = $newName -replace "^(.?)Content.", "" -replace '\\', '/'
        return $([RedirectsObj]::RedirectString -replace "__OldGameName__", $oldName -replace "__NewGameName__", $newName)
    }

    [string] ExtractHeaderFile([string] $FileName)
    {
        if($this.HeaderFileRegexMatch.Matches($FileName).Count -eq 1)
        {
            return $this.FileNameRegexExtract.Match($FileName).Groups[0].Value
        }
        return ""
    }

    [RedirectKVP] HandleNewRedirect([string] $FileName, [string] $OldSegment, [string] $NewSegment)
    {
        $Local_OldObjName = $this.ExtractHeaderFile($FileName)
        if($Local_OldObjName.Length -eq 0){
            return [RedirectKVP]::new()
        }

        $Local_NewObjectName = $Local_OldObjName.Replace($OldSegment, $NewSegment);
        if($FileName | Select-String -Pattern "\.h$"){
            if($Local_NewObjectName -ne [Target]::NewPrefixName){
                $this.ClassRedirects.Add($('+ActiveClassRedirects=(OldClassName="' + $Local_OldObjName + '",  NewClassName="' + $Local_NewObjectName + '")'))
            }
        }
        
        return [RedirectKVP]::new($FileName, $Local_OldObjName, $Local_NewObjectName)
    }
}

# This is the crux(focal point) of the logic
class Crux : Target
{
    [FilesObj] $FilesToBeRenamed
    [RedirectsObj] $IniRedirects # This is to gather the GameEngine.ini Redirects
    [System.Collections.ArrayList] $ObjRenamingMap # [RedirectKVP] This is used to replace Object Names in the header/cpp files
    [System.Collections.ArrayList] $SpecialPrefix # This is used for files like GameMode.h where you would use a prefix instead of a project-name

    Crux([string[]] $iSpecialPrefix)
    {
        $this.SetProjectPath()
        
        try{
            Move-Item $(".\Source\" + [Target]::OldProjectName) $(".\source\" + [Target]::NewProjectName) -ErrorAction Stop 
        }catch{
            Write-Host "Path Does Not Exist: " $(".\Source\" + [Target]::OldProjectName);
            Write-Host "From: " + $(".\Source\" + [Target]::OldProjectName)
            Write-Host "To  : " + $(".\source\" + [Target]::NewProjectName)
            Write-Host "Move-Item " $(".\Source\" + [Target]::OldProjectName) " " $(".\source\" + [Target]::NewProjectName)
            exit
        }

        function AddIfNotExist([System.Collections.ArrayList] $array, [string] $string){
            if($array.Contains($string) -ne $true){
                $array.Add($string) | Out-Null
            }
        }
        
        $this.SpecialPrefix = [System.Collections.ArrayList]::new()
        $this.SpecialPrefix.Add("GameInstance")
        $this.SpecialPrefix.Add("GameMode")
        $this.SpecialPrefix.Add("GameState")
        $this.SpecialPrefix.Add("PlayerState")

        foreach ($item in $iSpecialPrefix) {
            AddIfNotExist($this.SpecialPrefix, $item)
        }

        $this.IniRedirects = [RedirectsObj]::new()
        $this.ObjRenamingMap = [System.Collections.ArrayList]::new()

        $this.FilesToBeRenamed = [FilesObj]::new()
        $CodeFilePattern = "[^\\]+((cpp)|h)$"
        $ProjectNameFilePattern = "\..*$"

        # Add files by Prefix Name
        if([Target]::HadOldPrefix -eq $true -or $true -eq $true){
            $this.FilesToBeRenamed.Prefixes  = $($(Get-ChildItem ".\Source" -Recurse -File).FullName | 
                Select-String -Pattern $([Target]::OldPrefixName  + $CodeFilePattern))

            $this.FilesToBeRenamed.HasOldPrefixName = $true
            [Target]::OverwriteName = [Target]::NewPrefixName
        }else{
            [Target]::OverwriteName = [Target]::NewProjectName
        }

        # Add files by Project Name
        $this.FilesToBeRenamed.Projects  = $($(Get-ChildItem ".\Source" -Recurse -File).FullName | 
            Select-String -Pattern $([Target]::OldProjectName + $ProjectNameFilePattern))

        # Correct Exceptions for Project/Prefix Names
            # step 1/2 Get Named Exceptions
        $ItemsToMove = [System.Collections.ArrayList]::new()
        $this.FilesToBeRenamed.Prefixes.ForEach({
            foreach ($item in $this.SpecialPrefix) {
                if($($_.ToString() | Select-String -Pattern $item).length -eq 1){
                    $ItemsToMove.Add($_)
                }
            }
        })
            # step 2/2 remove exceptions
        $ItemsToMove.ForEach({
            $this.FilesToBeRenamed.Prefixes.Remove($_)
        })

        $ItemsToMove.ForEach({
            $this.FilesToBeRenamed.Articals.Add($_)
        })

        $this.FilesToBeRenamed.Projects.ForEach({
            # rename the official OldProjectName.cpp the NewProjectName.cpp (not the Prefix.cpp)
            if($_ | Select-String -Pattern $([Target]::OldProjectName + "\.(h|cpp)$")){
                $this.ObjRenamingMap.Add($this.IniRedirects.HandleNewRedirect($_, [Target]::OldProjectName, [Target]::NewProjectName))
            }else{
                # Swap out the OldProjectName for the NewPrefixName (unless we passed in those arguments as the same)
                $this.ObjRenamingMap.Add($this.IniRedirects.HandleNewRedirect($_, [Target]::OldProjectName, [Target]::OverwriteName))
            }
        })
        Write-Host "---------------------------------------------------------------"
        if([Target]::HadOldPrefix -eq $true -or $true -eq $true)
        {
            $this.FilesToBeRenamed.Prefixes.ForEach({
                $this.ObjRenamingMap.Add($this.IniRedirects.HandleNewRedirect($_, [Target]::OldPrefixName, [Target]::NewPrefixName))
            })
        }
        Write-Host "---------------------------------------------------------------"
        $this.FilesToBeRenamed.Articals.ForEach({
            $this.ObjRenamingMap.Add($this.IniRedirects.HandleNewRedirect($_, [Target]::OldProjectName, "The"))
        })

        # Remove maps with null returns 
        $this.ObjRenamingMap = $this.ObjRenamingMap | Where-Object { $_.Used -eq $true }
    }
    
    [void] AddRedirectsToDefaultEngine()
    {
        $(Get-Content ".\Config\DefaultEngine.ini") + 
            "[/Script/Engine.Engine]" + 
            $this.IniRedirects.GameNameRedirect + 
            $this.IniRedirects.ClassRedirects | 
            Set-Content ".\Config\DefaultEngine.ini" -Force

        $this.ObjRenamingMap.ForEach({
            Invoke-Expression $_.MoveCommand()
        })
    }

    [void] UpdateDirectoryNames()
    {
        $RexUObject = [Regex]::new("\.(umap|uasset)$")
        $PowershellPathPrep = "Microsoft.PowerShell.Core\FileSystem::"
        $PrePath = $PowershellPathPrep + $pwd.Path
        $(Get-ChildItem ".\Content" -Directory).ForEach(
        {
            if($_.Name -eq [Target]::OldProjectName)
            {
                if($($_.PSParentPath+'\'+[target]::NewProjectName) -eq $false){
                    mkdir $($_.PSParentPath+'\'+[target]::NewProjectName)
                }

                $(Get-ChildItem $_.FullName -File -Recurse).ForEach(
                {
                    $NewFolder = $_.PSParentPath.Substring($PrePath.Length + 1) -replace [Target]::OldProjectName, [Target]::NewProjectName
                    if($RexUObject.IsMatch($_) -eq $true){
                        $RedirectString = [RedirectsObj]::GetGameNameRedirectString(
                            $_.PSParentPath.substring($PrePath.Length+1)  + '\' + $_.Name.Substring(0, $_.Name.Length - $_.Extension.Length), 
                            $($NewFolder + '\' + $_.Name.Substring(0, $_.Name.Length - $_.Extension.Length)))
                        Add-Content -Path ".\Config\DefaultEngine.ini" -Value $RedirectString
                    }
                })
                Move-Item $_.FullName $($NewFolder -replace $("(?<="+[target]::NewProjectName+").*$"), "") | Out-Null
            }
        })

        $GameEngineIni = ''
        $RexGameEngineIni = [Regex]::new("^(GameDefaultMap|EditorStartupMap|GlobalDefaultGameMode|GameInstanceClass|TransitionMap).*$")
        $(Get-Content ".\Config\DefaultEngine.ini").foreach({
            if($RexGameEngineIni.IsMatch($_) -eq $true)
            {
                $GameEngineIni += $($_ -replace [Target]::OldProjectName, [Target]::NewProjectName)+ "`n"
            }else {
                $GameEngineIni += $_ + "`n"
            }
        })
        
        Set-Content -path ".\Config\DefaultEngine.ini" -Value $GameEngineIni
    }

    [void] UpdateUProjectFile()
    {
        $UProjectFileName = $([Target]::NewProjectName + ".uproject")
        Move-Item $([Target]::OldProjectName + ".uproject") $UProjectFileName
        $(Get-Content $UProjectFileName).Replace([Target]::OldProjectName, [Target]::NewProjectName) | Set-Content $UProjectFileName -Force
    }

    [void] UpdateCSFiles()
    {
        foreach ($FilePath in $($(Get-ChildItem ".\Source" -Recurse -File).FullName| Select-String -Pattern $("cs$"))){
            $(Get-Content $FilePath).Replace([Target]::OldProjectName, [Target]::NewProjectName) | Set-Content $FilePath -Force
        }
    }

    [void] MoveLoadingScreen()
    {
        $LoadingDir = $(Get-ChildItem ".\Source" | Where-Object { $_ -like "*loading*" }).FullName
        if($LoadingDir.Length -gt 0)
        {
            Move-Item $LoadingDir $($LoadingDir -replace [Target]::OldProjectName, [Target]::NewProjectName)
        }        
    }

    [void] UpdateMiscFiles()
    {
        # Move and update the rest of the code files
        $RemainingCodeFiles = $(Get-ChildItem ".\Source" -Recurse -File).FullName | Select-String -Pattern $([Target]::OldProjectName + "[^\\]*\.(h|cpp|cs|rc|plist)$")
        $RemainingCodeFiles.ForEach({
            $(Get-Content $_).
                Replace([Target]::OldProjectName, [Target]::NewProjectName).
                Replace([Target]::OldProjectName.ToUpper(), [Target]::NewProjectName.ToUpper()) | 
                Set-Content $($_ -replace [Target]::OldProjectName, [Target]::NewProjectName) -Force
            Remove-Item $_
        })

        # Move and update the rest of the misc files
        $($(Get-ChildItem '.' -Recurse -File).FullName | Select-String -Pattern $([Target]::OldProjectName + "[^\\]+$")).ForEach({
            Write-Host "path part: '$_'"
            Move-Item $_ $($_ -replace [Target]::OldProjectName, [Target]::NewProjectName) -Force
        })

        $iniFiles = @(
            ".\Config\DefaultGame.ini",
            ".\Config\Linux\LinuxEngine.ini",
            ".\Config\LinuxAArch64\LinuxAArch64Engine.ini",
            ".\Config\Mac\MacEngine.ini",
            ".\Config\Windows\WindowsEngine.ini"
        )

        $iniFiles.ForEach({
            if( $(Test-Path $_) -eq $true){
                $(Get-Content $_).Replace([Target]::OldProjectName, [Target]::NewProjectName) | Set-Content $_ -Force
            }
        })        
    }

    [void] CreatePublicAndPrivateFolders()
    {
        $PublicFolder = $(".\source\" + [Target]::NewProjectName + "\Public")
        $PrivateFolder = $(".\source\" + [Target]::NewProjectName + "\Private")
        if ($(Test-Path $PublicFolder) -eq $false){
            mkdir $PublicFolder
        }
        if ($(Test-Path $PrivateFolder) -eq $false){
            mkdir $PrivateFolder
        }
        $SourceDir = $("Source\" + [Target]::NewProjectName)
        $(Get-ChildItem $SourceDir -Recurse).foreach(
        {
            if( $($_.Name | Select-String -Pattern ".h$").Length -gt 0 -and 
                $($_.Name | Select-String -Pattern "Public|Private").Length -eq 0
            ){
                $NewLocation = $_.FullName.Replace($SourceDir, $($SourceDir + "\Public"))
                $NewFolder = $NewLocation -replace "[^\\/]+$", ""                
                if($(Test-Path $NewFolder) -eq $false){
                    mkdir $NewFolder
                }
                Move-Item $_.FullName $NewLocation
            }
            elseif( $($_.Name | Select-String -Pattern ".cpp$").Length -gt 0 -and 
                    $($_.Name | Select-String -Pattern "Public|Private").Length -eq 0
            ){
                $NewLocation = $_.FullName.Replace($SourceDir, $($SourceDir + "\Private"))
                $NewFolder = $NewLocation -replace "[^\\/]+$", ""                
                if($(Test-Path $NewFolder) -eq $false){
                    mkdir $NewFolder
                }
                Move-Item $_.FullName $NewLocation
            }
        })
    }
}

# Stores Processes and Output for the "find & replace" jobs of coding file
Class Thread
{
    [PowerShell] $Pow
    $Job

    Thread([Crux] $Crux, [string] $FilePath, [string] $Original, [String] $Target)
    {
        $this.Pow = [PowerShell]::Create()

        $this.Pow.AddScript([Thread]::ProcessFiles)
        $this.Pow.AddArgument($crux)
        $this.Pow.AddArgument($FilePath)
        $this.Pow.AddArgument($Original)
        $this.Pow.AddArgument($Target)
        
        $this.Job = $this.Pow.BeginInvoke()
    }

    static ProcessFiles([Crux] $iCrux, [string] $iFilePath, [String] $iOriginal, [String] $iTarget)
    {
        # Replace Object Names

        $FileText = $(Get-Content $iFilePath)
        $iCrux.ObjRenamingMap.ForEach({ # replace every new/old object 
            $FileText = $FileText.Replace($_.OldObjName, $_.NewObjName)
        })

        $FileText.
            Replace([Target]::OldProjectName,           [Target]::NewProjectName).
            Replace([Target]::OldProjectName.ToUpper(), [Target]::NewProjectName.ToUpper()) |
            Set-Content $iFilePath -Force

        if([Target]::HadOldPrefix)
        {
            $(Get-Content $iFilePath).
                Replace([Target]::OldPrefixName,            [Target]::NewPrefixName).
                Replace([Target]::OldPrefixName.ToUpper(),  [Target]::NewPrefixName.ToUpper()) |
                Set-Content $iFilePath -Force
        }
    }
}

function UpdateFilesWithNewObjectNames($crux)
{
    $AllHeadersAndCppFiles = $($(Get-ChildItem ".\Source" -Recurse -File).FullName| Select-String -Pattern $("[^\\]+((cpp)|h|(cs))$"))
    $crux.ObjRenamingMap = $crux.ObjRenamingMap | Where-Object { $_.OldObjName -ne [Target]::OldProjectName }

    # $false, $true
    if($false) # USE THIS FOR MULTI-THREADING (BROKEN) (Can't Custom Object In Seperate Thread)
    {
        $Threads = [System.Collections.ArrayList]::new()
        foreach($FilePath in $AllHeadersAndCppFiles)
        {
            Write-Host $FilePath
            $Threads.Add([Thread]::new($crux, $FilePath, [Target]::OldProjectName, [Target]::NewProjectName))
        }

        $Threads.ForEach({
            $_.Job.AsyncWaitHandle.WaitOne()
            $_.Pow.EndInvoke($_.Job)
        })
    }else { # USE THIS FOR SINGLE-THREADING
        foreach ($FilePath in $AllHeadersAndCppFiles)
        {
            Write-Host "Processing: " $($FilePath -replace "^.*\\", "")
            [Thread]::ProcessFiles($crux, $FilePath, [Target]::OldProjectName, [Target]::NewProjectName)
        } 
    }
}

# =======================================================================================================================================
# =======================================================================================================================================

[Target]::OldProjectName  = $OldProjectName
[Target]::NewProjectName  = $NewProjectName
[RedirectsObj]::RedirectString = "+ActiveGameNameRedirects=(OldGameName=`"/Game/__OldGameName__`", NewGameName=`"/Game/__NewGameName__`")"

Write-Host [RedirectObj]::RedirectString
if($OldPrefixName.Length -eq 0){
    [Target]::OldPrefixName = [Target]::OldProjectName
    [Target]::HadOldPrefix = $false
}elseif([Target]::OldPrefixName -eq [Target]::OldProjectName){
    [Target]::HadOldPrefix = $false
}else{
    [Target]::HadOldPrefix = $true
}
[Target]::NewPrefixName   = $NewPrefixName

$crux = [Crux]::new($SpecialPrefix)
$crux.DirList
$crux.IniRedirects.ClassRedirects.ForEach({ Write-Host $_ })
Write-Host
UpdateFilesWithNewObjectNames($crux)
$crux.AddRedirectsToDefaultEngine()
$crux.UpdateDirectoryNames()
$crux.UpdateUProjectFile()
$crux.UpdateCSFiles()
$crux.MoveLoadingScreen()
$crux.UpdateMiscFiles()
$crux.CreatePublicAndPrivateFolders()

try{
    # For VS Code
    code    .\Config\DefaultEngine.ini
}catch{
    notepad .\Config\DefaultEngine.ini
}



