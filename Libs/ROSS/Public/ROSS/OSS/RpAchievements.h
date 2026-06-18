// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ROSS/RpConfig.h"
#include "ROSS/Util/AchievementState.h"
#include "RpAchievements.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class THEGAME_API URpAchievements : public URpConfig
{
    GENERATED_BODY()
public:
    URpAchievements();
protected:
    virtual void BeginPlay() override;
public:
    sp<TNetResult<TArray<FAchievementState>>> QueryAchievements();
    sp<TNetResult<>> UnlockAchievement(const FString &Id);
};
