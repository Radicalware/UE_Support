#pragma once

#include "CoreMinimal.h"
#include "StatState.generated.h"

USTRUCT(BlueprintType)
struct THEGAME_API FStatState
{
    GENERATED_BODY()
    inline static TArray<FString> SvStatNames;
public:
    inline FStatState();

    inline static auto& GetStatNames() { return SvStatNames; }

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
    FString Name;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Data")
    double CurrentValue = 0.0;
};

inline static FStatState SoDefaultStatState; // init constructor
