#pragma once

#include "CoreMinimal.h"
#include "AchievementState.generated.h"


USTRUCT(BlueprintType)
struct THEGAME_API FAchievementState
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
    FString Id;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
    FText DisplayName;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
    float Progress = 0.0f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
    bool bUnlocked = false;
};
