#pragma once

#include "CoreMinimal.h"

#include "LeaderboardEntry.generated.h"

USTRUCT(BlueprintType)
struct THEGAME_API FLeaderboardEntry
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
    int32 Rank = 0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
    FString PlayerName;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
    double CurrentValue = 0.0;
};
