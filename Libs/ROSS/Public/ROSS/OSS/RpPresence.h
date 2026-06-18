// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ROSS/RpConfig.h"
#include "Interfaces/OnlinePresenceInterface.h"
#include "RpPresence.generated.h"


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class THEGAME_API URpPresence : public URpConfig
{
    GENERATED_BODY()
public:
    URpPresence();
protected:
    virtual void BeginPlay() override;
public:
    TArray<FText> GetPresenceAvailableStatuses() const;
    
    sp<TNetResult<>> SetPresenceStatus(
        const FString& NewStatusTextNamespace,
        const FString& NewStatusTextKey,
        const EOnlinePresenceState::Type NewStatus = EOnlinePresenceState::Online) const;
};
