// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ROSS/RpConfig.h"
#include "Interfaces/OnlinePartyInterface.h"
#include "RpParties.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class THEGAME_API URpParties : public URpConfig
{
    GENERATED_BODY()
public:
    URpParties();
protected:
    virtual void BeginPlay() override;
public:
    TArray<TSharedRef<const FOnlinePartyId>> GetPartiesJoinedParties() const;
    sp<TNetResult<>> CreateParty();
    TArray<IOnlinePartyJoinInfoConstRef> GetPartiesCurrentInvites() const;
    sp<TNetResult<>> AcceptInvite(const IOnlinePartyJoinInfoConstRef&Invite);
    TArray<FUniqueNetIdRepl> GetPartiesCurrentMembers(const FString &PartyId) const;
    sp<TNetResult<>> LeaveParty(const FString &PartyId);
    sp<TNetResult<>> KickMember(const FString &PartyId, const FUniqueNetIdRepl &MemberId);
    sp<TNetResult<>> InviteFriend(const FString &PartyId, const FUniqueNetIdRepl &MemberId);
};
