// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ROSS/RpConfig.h"
#include "ROSS/Util/StatState.h"
#include "RpStats.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class THEGAME_API URpStats : public URpConfig
{
    GENERATED_BODY()
public:
    URpStats();
protected:
    virtual void BeginPlay() override;
public:
    sp<TNetResult<TArray<FStatState>>> QueryStats();
    sp<TNetResult<>> IngestStat(const FString &StatName, double StatValue);
};
