// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ROSS/RpConfig.h"
#include "ROSS/Util/FriendState.h"
#include "RpFriends.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class THEGAME_API URpFriends : public URpConfig
{
    GENERATED_BODY()
public:
    URpFriends();
protected:
    virtual void BeginPlay() override;
public:
    sp<TNetResult<FString>> QueryFriends();
    TArray<FUniqueNetIdRepl> GetCurrentFriends() const;
    FFriendState GetFriendState(const FUniqueNetIdRepl &UserId) const;
    sp<TNetResult<>> SetFriendAlias(const FUniqueNetIdRepl &UserId, const FString &Alias);
    sp<TNetResult<>> DeleteFriendAlias(const FUniqueNetIdRepl &UserId);
    sp<TNetResult<>> SendInvite(const FUniqueNetIdRepl &UserId, sp<TNetResult<>> FoResultPtr = nullptr);
    sp<TNetResult<>> AcceptInvite(const FUniqueNetIdRepl &UserId);
    sp<TNetResult<>> RejectInvite(const FUniqueNetIdRepl &UserId);
    sp<TNetResult<>> DeleteFriend(const FUniqueNetIdRepl &UserId);
    sp<TNetResult<>> QueryRecentPlayers();
    TArray<TSharedRef<FOnlineRecentPlayer>> GetCurrentRecentPlayers() const;
    sp<TNetResult<>> BlockPlayer(const FUniqueNetIdRepl &UserId);
    sp<TNetResult<>> UnblockPlayer(const FUniqueNetIdRepl &UserId);
    sp<TNetResult<>> QueryBlockedPlayers();
    TArray<TSharedRef<FOnlineBlockedPlayer>> GetCurrentBlockedPlayers() const;
    sp<TNetResult<>> SendInviteByFriendCode(const FString &FriendCode);
    FString GetCurrentFriendCode() const;
};
