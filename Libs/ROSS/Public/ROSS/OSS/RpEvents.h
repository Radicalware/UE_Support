// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ROSS/RpConfig.h"
#include "RpEvents.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFriendsOnFriendsChangeEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPartiesOnPartiesStateChangedEvent);

// Has to do with registering UI Event Handlers for Parties/Friends
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class THEGAME_API URpEvents : public URpConfig
{
    GENERATED_BODY()
public:
    URpEvents();
protected:
    virtual void BeginPlay() override;
    
    TMulticastDelegate<void()> FriendsOnFriendsChange;
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "RaiseFriendsOnFriendsChange", Category = "Friends"))
    void RaiseFriendsOnFriendsChange();

    TMulticastDelegate<void()> PartiesOnPartiesStateChanged;
    UFUNCTION(BlueprintCallable, meta = (DisplayName = "RaisePartiesOnPartiesStateChanged", Category = "Friends"))
    void RaisePartiesOnPartiesStateChanged();

    FString MsJoinSessionIdFromParty;

public:
    void RegisterEvents();
    const FString& GetJoinSessionIdFromParty() const { return MsJoinSessionIdFromParty; }
};
