#include "ROSS/Util/GameConfig.h"
#include "Access/General.h"
#include "Mode/DefaultGameMode.h"

void FGameConfig::SetMapModeFromCLI()
{
    PrintStart();
    uint32 LnPort = 7777;
    FParse::Value(FCommandLine::Get(), TEXT("port="), MnGamePort);
    FParse::Value(FCommandLine::Get(), TEXT("MapPath="), MsMapPath);
    FParse::Value(FCommandLine::Get(), TEXT("ModePath="), MsModePath);

    SetMapPath(MsMapPath);
    SetModePath(MsModePath);
}

void FGameConfig::SetMapPath(const FString& FsMapPath)
{
    PrintStart();
    MsMapPath = FsMapPath;
    Print("MsMapPath: ", MsMapPath);
    MsMapPath = XF::FindReplace(MsMapPath, TEXT(R"(^(\.?)/Content/)"), TEXT("/Game/"));
    MsMapPath = XF::FindReplace(MsMapPath, TEXT(R"(\.umap)"), TEXT(""));
    Print("MsMapPath: ", MsMapPath);
    MsMap = FPackageName::GetShortName(MsMapPath);
    ensure(!MsMapPath.IsEmpty());
    ensure(!MsMap.IsEmpty());
    ensure(FPackageName::DoesPackageExist(MsMapPath));
}

void FGameConfig::SetModePath(const FString& FsModePath)
{
    PrintStart();
    std::tie(MsMode, MsModePath) = XF::ToUObjectAndAssetPath<ADefaultGameMode>(FsModePath);
}

// note: FRossSessionSettings::SetSessionSettingsMapMode
//       is where the Map/Modes are set for the Session Settings

