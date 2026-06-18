// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ROSS/RpConfig.h"
#include "ROSS/Util/NetResult.h"
#include "RpAvatar.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class THEGAME_API URpAvatar : public URpConfig
{
    GENERATED_BODY()
public:
    URpAvatar();
protected:
    virtual void BeginPlay() override;
public:
    TSharedPtr<TNetResult<UTexture>> GetAvatar(
        const FUniqueNetIdRepl &UserId);
};
