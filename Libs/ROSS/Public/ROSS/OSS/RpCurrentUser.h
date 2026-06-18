// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ROSS/RpConfig.h"
#include "RpCurrentUser.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class THEGAME_API URpCurrentUser : public URpConfig
{
    GENERATED_BODY()
public:
    URpCurrentUser();
protected:
    virtual void BeginPlay() override;
public:
    FString GetCurrentUserDisplayName() const;
    FUniqueNetIdRepl GetCurrentUserId() const;
    FString GetCurrentUserSecondaryId() const;
    bool ShouldRenderUserSecondaryIdField() const;
    FString GetCurrentUserAuthAttribute(const FString &Key) const;
};
