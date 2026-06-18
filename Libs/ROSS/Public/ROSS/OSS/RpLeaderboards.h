// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ROSS/RpConfig.h"
#include "ROSS/Util/LeaderboardEntry.h"
#include "RpLeaderboards.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class THEGAME_API URpLeaderboards : public URpConfig
{
    GENERATED_BODY()
public:
    URpLeaderboards();
protected:
    virtual void BeginPlay() override;
public:
    sp<TNetResult<TArray<FLeaderboardEntry>>> QueryGlobalLeaderboards();
    sp<TNetResult<TArray<FLeaderboardEntry>>> QueryFriendsLeaderboards();
};
