#include "ROSS/Util/GameConfig.h"

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
    Print("FsMapPath: ", FsMapPath);
    The.MsMapPath = XF::FindReplace(FsMapPath, TEXT("^/Content/"), TEXT("/Game/"));
    Print("MsMapPath: ", MsMapPath);
    The.MsMap = FPackageName::GetShortName(MsMapPath);
    ensure(!MsMapPath.IsEmpty());
    ensure(!MsMap.IsEmpty());
    ensure(FPackageName::DoesPackageExist(MsMapPath));
}

void FGameConfig::SetModePath(const FString& FsModePath)
{
    PrintStart();
    Print("FsModePath: ", FsModePath);
    The.MsModePath = XF::FindReplace(FsModePath, TEXT("^/Content/"), TEXT("/Game/"));
    if (!MsModePath.EndsWith(TEXT("_C"))) 
    {
        auto LsGameModeName = XF::FindFirstMatch(FsModePath, TEXT("/([^/]+)$"));
        MsModePath += TEXT(".") + LsGameModeName + TEXT("_C");
    }
    Print("MsModePath: ", MsModePath);
    The.MsMode = FPackageName::GetShortName(MsModePath);
    ensure(!MsModePath.IsEmpty());
    ensure(!MsMode.IsEmpty());
    ensure(XF::BxGameModeExists(MsModePath));
}
