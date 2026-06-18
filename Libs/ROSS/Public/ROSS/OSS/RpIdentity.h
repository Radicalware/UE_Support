// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ROSS/RpConfig.h"
#include "RpIdentity.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class THEGAME_API URpIdentity : public URpConfig
{
    GENERATED_BODY()
public:
    URpIdentity();
protected:
    virtual void BeginPlay() override;
public:
    FUniqueNetIdRepl GetIdentityUniqueNetId(const FString &UniqueNetId);
};
