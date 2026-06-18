#include "Access/XF.h"
#include "GameFramework/GameModeBase.h"
#include "Misc/DefaultValueHelper.h"

FRotator XF::MakeRotator(float Roll, float Pitch, float Yaw)
{
    FRotator Rotator = FRotator::ZeroRotator;
    Rotator.Roll = Roll;
    Rotator.Pitch = Pitch;
    Rotator.Yaw = Yaw;
    return Rotator;
}

bool XF::LocationsAreClose(const FVector& Loc1, const FVector& Loc2)
{
    return FMath::IsNearlyEqual(Loc1.X, Loc2.X)
        && FMath::IsNearlyEqual(Loc1.Y, Loc2.Y)
        && FMath::IsNearlyEqual(Loc1.Z, Loc2.Z);
}

bool XF::RotationsAreClose(const FRotator& Rot1, const FRotator& Rot2)
{
    return FMath::IsNearlyEqual(Rot1.Roll, Rot2.Roll)
        && FMath::IsNearlyEqual(Rot1.Pitch, Rot2.Pitch)
        && FMath::IsNearlyEqual(Rot1.Yaw, Rot2.Yaw);
}

const char* XF::FStringToRValChars(const FString& FsStr)
{
    SsTempStr = TCHAR_TO_UTF8(*FsStr);
    return SsTempStr.c_str();

}

const char* XF::FStringToRValChars(const FName& FsStr)
{
    return FStringToRValChars(FsStr.ToString());
}

bool XF::BxMapExists(const FString& FsPathToWorld)
{
    UWorld* LoWorldClass = Cast<UWorld>(StaticLoadObject(UWorld::StaticClass(), nullptr, *FsPathToWorld));
    if (!LoWorldClass)
    {
        PrintE("Map asset NOT found: ", FsPathToWorld);
        return false;
    }
    return true;
}

bool XF::BxGameModeExists(const FString& FsPathToGameMode)
{
    UClass* LoGameModeClass = StaticLoadClass(AGameModeBase::StaticClass(), nullptr, *FsPathToGameMode);
    if (!LoGameModeClass) {
        PrintE("GameMode class NOT found: ", FsPathToGameMode);
        return false;
    }
    return true;
}

int32 XF::StringToInt(const FString& FsStr)
{
    int32 LnNumber = 0;
    if (FDefaultValueHelper::ParseInt(FsStr, LnNumber))
        return LnNumber;
    else
        throw FString(FString("Invalid Number String: ") + FsStr);
}
