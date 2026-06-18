// Fill out your copyright notice in the Description page of Project Settings.


#include "ROSS/OSS/RpPresence.h"

#include "OnlineSubsystemUtils.h"

URpPresence::URpPresence()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URpPresence::BeginPlay()
{
	Super::BeginPlay();
}

TArray<FText> URpPresence::GetPresenceAvailableStatuses() const
{
    TArray<FText> Text;
    Text.Add(NSLOCTEXT("PresenceLocalisationText", "PresenceExample1", "Example presence status 1"));
    Text.Add(NSLOCTEXT("PresenceLocalisationText", "PresenceExample2", "Example presence with numeric value {Number}"));
    return Text;
}

sp<TNetResult<>> URpPresence::SetPresenceStatus(
    const FString& NewStatusTextNamespace,
    const FString& NewStatusTextKey,
    const EOnlinePresenceState::Type NewStatus) const
{
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    auto Result = MakeThreadPtr(TNetResult<>);
    auto Presence = OSS.GetPresenceInterface();
    if (!Presence.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support presence."));
        return Result;
    }

    // Update the local user's presence.
    FOnlineUserPresenceStatus PresenceStatus;
    PresenceStatus.State = NewStatus;
    PresenceStatus.StatusStr = FString::Printf(TEXT("%s_%s"), *NewStatusTextNamespace, *NewStatusTextKey);
    PresenceStatus.Properties.Add(TEXT("Number"), 200);
    Presence->SetPresence(
        *UserId,
        PresenceStatus,
        IOnlinePresence::FOnPresenceTaskCompleteDelegate::CreateWeakLambda(
            this,
            [ResultWk = TWeakPtr<TNetResult<>>(Result)]
            (const class FUniqueNetId&, const bool bWasSuccessful) 
            {
                GetWeakSafe(Result);
                Result.OnResult(bWasSuccessful, bWasSuccessful ? TEXT("") : TEXT("Failed to set presence"));
            }));
    return Result;
}



