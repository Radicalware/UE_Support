// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ROSS/RpConfig.h"
#include "RpUsers.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class THEGAME_API URpUsers : public URpConfig
{
    GENERATED_BODY()
public:
    URpUsers();
protected:
    virtual void BeginPlay() override;
public:
    sp<TNetResult<FString>> QueryUserInfo(const FString &UserId);

    sp<TNetResult<FString>> QueryUserByDisplayName(const FString &DisplayName);
        
    sp<TNetResult<FString>> QueryExternalIds(const FString &PlatformName, const TArray<FString> &ExternalIds);
};
