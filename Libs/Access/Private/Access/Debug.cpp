
#include "Access/Debug.h"
#include "Access/Macros.h"
#include "Access/XF.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

void UDebug::DrawDebugArrow(const FVector& StartLocation, const FVector& Direction) const
{
    DrawDebugDirectionalArrow(
        GetWorld(),
        StartLocation,
        StartLocation + Direction,
        32, FColor::Yellow,
        false,
        0.f,
        1.f
    );
}

void UDebug::DrawSphere(const FVector& Location) const
{
    DrawDebugSphere(
        GetWorld(),
        Location,
        20,
        12,
        FColor::Yellow,
        false,
        4.f,
        1.f
    );
}

void UDebug::DrawString(AActor* Actor, const FString& String, float Duration) const
{
    DrawDebugString(
        GetWorld(),
        FVector::ZeroVector,
        String,
        Actor,
        FColor::White,
        Duration,
        true
    );
}

void UDebug::DrawArrow(const FVector& StartLocation, const FVector& Direction) const
{
    DrawDebugDirectionalArrow(
        GetWorld(),
        StartLocation,
        StartLocation + Direction,
        32,
        FColor::Yellow,
        false,
        0.f,
        0,
        1.f
    );
}

FString UDebug::WriteErrorToFile(const FString& ErrorOut)
{
    MsErrorText = ErrorOut;
    if (GIsEditor)
    {
        const FString FilePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()) + TEXT("/ErrorLog.txt");
        PrintE(ErrorOut);
        FFileHelper::SaveStringToFile(ErrorOut, *FilePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), EFileWrite::FILEWRITE_Append);
    }
    return ErrorOut;
}

bool UDebug::ErrorOut(const UWorld* World, const uint8 FnPlayerID, const FString& FsError)
{
    MsErrorText = FsError;
    PrintE(FsError);
    UDebug::WriteErrorToFile(FsError + "\n");
    if (GIsEditor)
    {
        if (GIsEditor)
            UKismetSystemLibrary::QuitGame(World, nullptr, EQuitPreference::Quit, false);
        return true;
    }
    return false;
}
