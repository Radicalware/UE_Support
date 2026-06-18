#pragma once

#include "CoreMinimal.h"
#include "Macros.h"
#include "Debug.generated.h"

UCLASS()
class UDebug : public UActorComponent
{
public:
    GENERATED_BODY()

    void DrawDebugArrow(const FVector& StartLocation, const FVector& Direction) const;
    void DrawSphere(const FVector& Location) const;
    void DrawString(AActor* Actor, const FString& String, float Duration = 1.f) const;
    void DrawArrow(const FVector& StartLocation, const FVector& Direction) const;

    static FString WriteErrorToFile(const FString& ErrorOut);
    static bool    ErrorOut(const UWorld* World, const uint8 FnPlayerID, const FString& FsError);

    inline static FString GetErrors() { return MsErrorText; }
private:
    inline static FString MsErrorText;
};
