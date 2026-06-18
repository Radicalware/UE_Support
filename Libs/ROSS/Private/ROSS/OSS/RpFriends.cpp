// Fill out your copyright notice in the Description page of Project Settings.


#include "ROSS/OSS/RpFriends.h"
#include "Interfaces/OnlineFriendsInterface.h"
#include "Interfaces/OnlinePartyInterface.h"
#include "Interfaces/OnlinePresenceInterface.h"
#include "Interfaces/OnlineUserInterface.h"
#include "OnlineSubsystemUtils.h"


URpFriends::URpFriends()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URpFriends::BeginPlay()
{
	Super::BeginPlay();
}



sp<TNetResult<FString>> URpFriends::QueryFriends()
{
    auto Result = MakeThreadPtr(TNetResult<FString>);
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    auto Friends = OSS.GetFriendsInterface();
    if (!Friends.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support friends."));
        return Result;
    }

    // Ask the online subsystem to cache the initial list of friends. We don't return friends inside
    // ExecuteFriendsQueryFriends; we're just getting the online subsystem to cache them so that a later
    // GetFriendsCurrentFriends can return them.
    if (!Friends->ReadFriendsList(
        this->LocalUserNum,
        TEXT(""),
        FOnReadFriendsListComplete::CreateWeakLambda(
            this,
            [ResultWk = TWeakPtr<TNetResult<FString>>(Result)]
            (int32, bool bWasSuccessful, const FString& FsListName, const FString& ErrorStr) {
                    
                    GetWeakSafe(Result); // Make sure the result callback is still valid.
                    auto LoResultPtr = MakeShared<FString>(FsListName);
                    Result.OnReturnResult(bWasSuccessful, LoResultPtr, ErrorStr);
            })))
    {
        Result->OnResult(false, TEXT("The ReadFriendsList call failed to start."));
    }
    return Result;
}

TArray<FUniqueNetIdRepl> URpFriends::GetCurrentFriends() const
{
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    auto  Friends = OSS.GetFriendsInterface();
    if (!Friends.IsValid())
        return TArray<FUniqueNetIdRepl>();

    // Call GetFriendsList and return the user IDs of all the user's friends.
    TArray<FUniqueNetIdRepl> CurrentFriendIds;
    TArray<TSharedRef<FOnlineFriend>> LoOutCurrentFriends;
    if (!Friends->GetFriendsList(LocalUserNum, TEXT(""), LoOutCurrentFriends))
        return TArray<FUniqueNetIdRepl>();
    for (const auto& CurrentFriend : LoOutCurrentFriends)
        CurrentFriendIds.Add(CurrentFriend->GetUserId());
    return CurrentFriendIds;
}

FFriendState URpFriends::GetFriendState(const FUniqueNetIdRepl& TargetUserId) const
{
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    auto  Friends = OSS.GetFriendsInterface();
    if (!Friends.IsValid())
        return FFriendState();

    // Attempt to get the target user's friend information.
    auto TargetFriend = Friends->GetFriend(this->LocalUserNum, *TargetUserId, TEXT(""));
    if (!TargetFriend.IsValid())
    {
        return FFriendState();
    }

    // Attempt to get the party ID.
    FString PartyId;
    auto Party = OSS.GetPartyInterface();
    if (Party.IsValid())
    {
        auto AdvertisedPartyId = Party->GetAdvertisedParty(*UserId, *TargetUserId, Party->GetPrimaryPartyTypeId());
        if (AdvertisedPartyId.IsValid())
        {
            PartyId = AdvertisedPartyId->GetPartyId()->ToString();
        }
    }

    // Convert the main state.
    FFriendState State;
    State.Id = TargetFriend->GetUserId();
    State.DisplayName = TargetFriend->GetDisplayName();
    State.RealName = TargetFriend->GetRealName();
    switch (TargetFriend->GetInviteStatus())
    {
    case EInviteStatus::Accepted:
        State.InvitationStatus = EFriendInvitationStatus::Accepted;
        break;
    case EInviteStatus::PendingInbound:
        State.InvitationStatus = EFriendInvitationStatus::PendingInbound;
        break;
    case EInviteStatus::PendingOutbound:
        State.InvitationStatus = EFriendInvitationStatus::PendingOutbound;
        break;
    case EInviteStatus::Blocked:
        State.InvitationStatus = EFriendInvitationStatus::Blocked;
        break;
    case EInviteStatus::Suggested:
        State.InvitationStatus = EFriendInvitationStatus::Suggested;
        break;
    case EInviteStatus::Unknown:
    default:
        State.InvitationStatus = EFriendInvitationStatus::Unknown;
        break;
    }
    const auto& Presence = TargetFriend->GetPresence();
    State.PresenceSessionId = Presence.SessionId;
    State.PresencePartyId = PartyId;
    State.bPresenceIsOnline = Presence.bIsOnline;
    State.bPresenceIsPlaying = Presence.bIsPlaying;
    State.bPresenceIsPlayingThisGame = Presence.bIsPlayingThisGame;
    State.bPresenceIsJoinable = Presence.bIsJoinable;
    State.bPresenceHasVoiceSupport = Presence.bHasVoiceSupport;
    State.PresenceLastOnline = Presence.LastOnline;
    State.PresenceStatusString = Presence.Status.StatusStr;
    switch (Presence.Status.State)
    {
    case EOnlinePresenceState::Online:
        State.PresenceStatusState = EFriendPresenceStatus::Online;
        break;
    case EOnlinePresenceState::Away:
        State.PresenceStatusState = EFriendPresenceStatus::Away;
        break;
    case EOnlinePresenceState::ExtendedAway:
        State.PresenceStatusState = EFriendPresenceStatus::ExtendedAway;
        break;
    case EOnlinePresenceState::DoNotDisturb:
        State.PresenceStatusState = EFriendPresenceStatus::DoNotDisturb;
        break;
    case EOnlinePresenceState::Chat:
        State.PresenceStatusState = EFriendPresenceStatus::Chat;
        break;
    case EOnlinePresenceState::Offline:
    default:
        State.PresenceStatusState = EFriendPresenceStatus::Offline;
        break;
    }
    for (const auto& KV : Presence.Status.Properties)
    {
        State.PresenceStatusProperties.Add(KV.Key, KV.Value.ToString());
    }

    // Get a list of attribute keys to fetch from the friend.
    TArray<FString> Keys{
        TEXT("id"),
        TEXT("ready"),
        TEXT("productUserId"),
        TEXT("displayName"),
        TEXT("prefDisplayName"),
        TEXT("deletable"),
        TEXT("eosSynthetic.primaryFriend.subsystemName"),
        TEXT("eosSynthetic.preferredFriend.subsystemName"),
    };
    FString SubsystemNames;
    TArray<FString> SubsystemNamesArray;
    TargetFriend->GetUserAttribute(TEXT("eosSynthetic.subsystemNames"), SubsystemNames);
    SubsystemNames.ParseIntoArray(SubsystemNamesArray, TEXT(","));
    for (const auto& SubsystemName : SubsystemNamesArray)
    {
        Keys.Add(FString::Printf(TEXT("eosSynthetic.friend.%s.id"), *SubsystemName));
        Keys.Add(FString::Printf(TEXT("eosSynthetic.friend.%s.realName"), *SubsystemName));
        Keys.Add(FString::Printf(TEXT("eosSynthetic.friend.%s.displayName"), *SubsystemName));
    }
    FString ExternalAccountTypes;
    TArray<FString> ExternalAccountTypesArray;
    TargetFriend->GetUserAttribute(TEXT("externalAccountTypes"), ExternalAccountTypes);
    ExternalAccountTypes.ParseIntoArray(ExternalAccountTypesArray, TEXT(","));
    for (const auto& ExternalAccount : ExternalAccountTypesArray)
    {
        Keys.Add(FString::Printf(TEXT("externalAccount.%s.id"), *ExternalAccount));
        Keys.Add(FString::Printf(TEXT("externalAccount.%s.displayName"), *ExternalAccount));
        Keys.Add(FString::Printf(TEXT("externalAccount.%s.lastLoginTime.unixTimestampUtc"), *ExternalAccount));
        if (ExternalAccount == TEXT("epic"))
        {
            Keys.Add(FString::Printf(TEXT("externalAccount.%s.country"), *ExternalAccount));
            Keys.Add(FString::Printf(TEXT("externalAccount.%s.nickname"), *ExternalAccount));
            Keys.Add(FString::Printf(TEXT("externalAccount.%s.preferredLanguage"), *ExternalAccount));
        }
    }
    for (const auto& Key : Keys)
    {
        FString Value;
        if (TargetFriend->GetUserAttribute(Key, Value))
        {
            State.Attributes.Add(Key, Value);
        }
    }

    return State;
}

FString URpFriends::GetCurrentFriendCode() const
{
    auto UserID = GetNetUserID();
    auto UserAccount = GetIdentity().GetUserAccount(*UserID);
    checkf(UserID->IsValid(), TEXT("Expected GetUserAccount to return a valid account."));

    // Return the value of the "friendCode" auth attribute.
    FString Value;
    return UserAccount->GetAuthAttribute(TEXT("friendCode"), Value) ? Value : TEXT("");
}


sp<TNetResult<>> URpFriends::SetFriendAlias(
    const FUniqueNetIdRepl& TargetUserId,
    const FString& Alias)
{
    auto Result = MakeThreadPtr(TNetResult<>);
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    auto Friends = OSS.GetFriendsInterface();
    if (!Friends.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support friends."));
        return Result;
    }

    // Set the friend's alias.
    Friends->SetFriendAlias(
        this->LocalUserNum,
        *TargetUserId, // Friend ID
        TEXT(""), // ListName (name of the friends list to operate on)
        Alias,
        FOnSetFriendAliasComplete::CreateWeakLambda(
            this,
            [ResultWk = TWeakPtr<TNetResult<>>(Result)]
            (int32, const FUniqueNetId&, const FString&, const FOnlineError& Error) 
            {
                GetWeakSafe(Result);
                Result.OnResult(Error.bSucceeded, Error.ToLogString());
            }));
    return Result;
}

sp<TNetResult<>> URpFriends::DeleteFriendAlias(const FUniqueNetIdRepl& TargetUserId)
{
    auto Result = MakeThreadPtr(TNetResult<>);
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    auto Friends = OSS.GetFriendsInterface();
    if (!Friends.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support friends."));
        return Result;
    }

    // Delete the friend's alias.
    Friends->DeleteFriendAlias(
        this->LocalUserNum,
        *TargetUserId,
        TEXT(""),
        FOnDeleteFriendAliasComplete::CreateWeakLambda(
            this,
            [ResultWk = TWeakPtr<TNetResult<>>(Result)]
            (int32, const FUniqueNetId&, const FString&, const FOnlineError& Error)
            {
                GetWeakSafe(Result);
                Result.OnResult(Error.bSucceeded, Error.ToLogString());
            }));
    return Result;
}

sp<TNetResult<>> URpFriends::SendInvite(const FUniqueNetIdRepl& TargetUserId, sp<TNetResult<>> FoResultPtr)
{
    auto Result = (FoResultPtr.IsValid())
        ? FoResultPtr
        : MakeThreadPtr(TNetResult<>);
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    auto Friends = OSS.GetFriendsInterface();
    if (!Friends.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support friends."));
        return Result;
    }

    // Send the friend invite.
    if (!Friends->SendInvite(
        this->LocalUserNum,
        *TargetUserId,
        TEXT(""),
        FOnSendInviteComplete::CreateWeakLambda(
            this,
            [ResultWk = TWeakPtr<TNetResult<>>(Result)]
            (
                int32,
                bool bWasSuccessful,
                const FUniqueNetId&,
                const FString&,
                const FString& ErrorStr) 
            {
                GetWeakSafe(Result);
                Result.OnResult(bWasSuccessful, ErrorStr);
            })))
    {
        // Call failed to start.
        Result->OnResult(false, TEXT("SendInvite call failed to start."));
    }
    return Result;
}

sp<TNetResult<>> URpFriends::AcceptInvite(const FUniqueNetIdRepl& TargetUserId)
{
    auto Result = MakeThreadPtr(TNetResult<>);
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    auto Friends = OSS.GetFriendsInterface();
    if (!Friends.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support friends."));
        return Result;
    }

    // Accept the friend invite.
    if (!Friends->AcceptInvite(
        this->LocalUserNum,
        *TargetUserId,
        TEXT(""),
        FOnAcceptInviteComplete::CreateWeakLambda(
            this,
            [ResultWk = TWeakPtr<TNetResult<>>(Result)]
            (
                int32,
                bool bWasSuccessful,
                const FUniqueNetId&,
                const FString&,
                const FString& ErrorStr) 
            {
                    GetWeakSafe(Result);
                    Result.OnResult(bWasSuccessful, ErrorStr);
            })))
    {
        // Call failed to start.
        Result->OnResult(false, TEXT("AcceptInvite call failed to start."));
    }
    return Result;
}

sp<TNetResult<>> URpFriends::RejectInvite(const FUniqueNetIdRepl& TargetUserId)
{
    auto Result = MakeThreadPtr(TNetResult<>);
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    auto Friends = OSS.GetFriendsInterface();
    if (!Friends.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support friends."));
        return Result;
    }

    // Register an event so we can receive the reject outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle = Friends->AddOnRejectInviteCompleteDelegate_Handle(
        this->LocalUserNum,
        FOnRejectInviteCompleteDelegate::CreateWeakLambda(
            this,
            [this, Friends, CallbackHandle, TargetUserId, ResultWk = TWeakPtr<TNetResult<>>(Result)]
            (
                int32 CallbackLocalUserNum,
                bool bCallbackWasSuccessful,
                const FUniqueNetId& CallbackFriendId,
                const FString&,
                const FString& CallbackErrorStr) 
            {
                    // Check if this callback is for us.
                    if (CallbackLocalUserNum != this->LocalUserNum || CallbackFriendId != *TargetUserId)
                    {
                        // This callback isn't for our call.
                        return;
                    }

                    GetWeakSafe(Result);
                    Result.OnResult(bCallbackWasSuccessful, CallbackErrorStr);

                    // Unregister this callback since we've handled the call we care about.
                    Friends->ClearOnRejectInviteCompleteDelegate_Handle(this->LocalUserNum, *CallbackHandle);
            }));

    // Reject the friend invite.
    if (!Friends->RejectInvite(this->LocalUserNum, *TargetUserId, TEXT("")))
    {
        // The call failed to start; unregister callback handler.
        Friends->ClearOnRejectInviteCompleteDelegate_Handle(this->LocalUserNum, *CallbackHandle);
        Result->OnResult(false, TEXT("RejectInvite call failed to start."));
    }
    return Result;
}

sp<TNetResult<>> URpFriends::DeleteFriend(const FUniqueNetIdRepl& TargetUserId)
{
    auto Result = MakeThreadPtr(TNetResult<>);
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    auto Friends = OSS.GetFriendsInterface();
    if (!Friends.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support friends."));
        return Result;
    }

    // Register an event so we can receive the delete outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle = Friends->AddOnDeleteFriendCompleteDelegate_Handle(
        this->LocalUserNum,
        FOnDeleteFriendCompleteDelegate::CreateWeakLambda(
            this,
            [this, Friends, CallbackHandle, TargetUserId, ResultWk = TWeakPtr<TNetResult<>>(Result)]
            (
                int32 CallbackLocalUserNum,
                bool bCallbackWasSuccessful,
                const FUniqueNetId& CallbackFriendId,
                const FString&,
                const FString& CallbackErrorStr) 
            {
                    // Check if this callback is for us.
                    if (CallbackLocalUserNum != this->LocalUserNum || CallbackFriendId != *TargetUserId)
                    {
                        // This callback isn't for our call.
                        return;
                    }

                    GetWeakSafe(Result);
                    Result.OnResult(bCallbackWasSuccessful, CallbackErrorStr);

                    // Unregister this callback since we've handled the call we care about.
                    Friends->ClearOnDeleteFriendCompleteDelegate_Handle(this->LocalUserNum, *CallbackHandle);
            }));

    // Delete the friend.
    if (!Friends->DeleteFriend(this->LocalUserNum, *TargetUserId, TEXT("")))
    {
        // The call failed to start; unregister callback handler.
        Friends->ClearOnDeleteFriendCompleteDelegate_Handle(this->LocalUserNum, *CallbackHandle);
        Result->OnResult(false, TEXT("DeleteFriend call failed to start."));
    }
    return Result;
}

sp<TNetResult<>> URpFriends::QueryRecentPlayers()
{
    auto Result = MakeThreadPtr(TNetResult<>);
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    auto Friends = OSS.GetFriendsInterface();
    if (!Friends.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support friends."));
        return Result;
    }

    // Register an event so we can receive the query outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle =
        Friends->AddOnQueryRecentPlayersCompleteDelegate_Handle(FOnQueryRecentPlayersCompleteDelegate::CreateWeakLambda(
            this,
            [Friends, CallbackHandle, UserId, ResultWk = TWeakPtr<TNetResult<>>(Result)]
            (
                const FUniqueNetId& CallbackUserId,
                const FString&,
                bool bCallbackWasSuccessful,
                const FString& CallbackError) 
            {
                    // Check if this callback is for us.
                    if (CallbackUserId != *UserId)
                    {
                        // This callback isn't for our call.
                        return;
                    }

                    GetWeakSafe(Result);
                    Result.OnResult(bCallbackWasSuccessful, CallbackError);

                    // Unregister this callback since we've handled the call we care about.
                    Friends->ClearOnQueryRecentPlayersCompleteDelegate_Handle(*CallbackHandle);
            }));

    // Ask the online subsystem to query the recent players. We don't return recent players inside
    // ExecuteFriendsQueryRecentPlayers; we're just getting the online subsystem to cache them so that a later
    // GetFriendsCurrentRecentPlayers can return them.
    if (!Friends->QueryRecentPlayers(*UserId, TEXT("")))
    {
        // The call failed to start; unregister callback handler.
        Friends->ClearOnQueryRecentPlayersCompleteDelegate_Handle(*CallbackHandle);
        Result->OnResult(false, TEXT("QueryRecentPlayers call failed to start."));
    }
    return Result;
}

TArray<TSharedRef<FOnlineRecentPlayer>> URpFriends::GetCurrentRecentPlayers() const
{
    auto Result = MakeThreadPtr(TNetResult<>);
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    auto Friends = OSS.GetFriendsInterface();

    TArray<TSharedRef<FOnlineRecentPlayer>> RecentPlayers;
    if (!Friends.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support friends."));
        return RecentPlayers;
    }

    // Get the recent players.
    if (!Friends->GetRecentPlayers(*UserId, TEXT(""), RecentPlayers))
        Print("No Recent Players");
    return RecentPlayers;;
}

sp<TNetResult<>> URpFriends::BlockPlayer(const FUniqueNetIdRepl& TargetUserId)
{
    auto Result = MakeThreadPtr(TNetResult<>);
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    auto Friends = OSS.GetFriendsInterface();
    if (!Friends.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support friends."));
        return Result;
    }

    // Register an event so we can receive the block outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle = Friends->AddOnBlockedPlayerCompleteDelegate_Handle(
        this->LocalUserNum,
        FOnBlockedPlayerCompleteDelegate::CreateWeakLambda(
            this,
            [this, Friends, CallbackHandle, TargetUserId, ResultWk = TWeakPtr<TNetResult<>>(Result)]
            (
                int32 CallbackLocalUserNum,
                bool bCallbackWasSuccessful,
                const FUniqueNetId& CallbackTargetUserId,
                const FString&,
                const FString& CallbackError)
            {
                    // Check if this callback is for us.
                    if (CallbackLocalUserNum != this->LocalUserNum || CallbackTargetUserId != *TargetUserId)
                    {
                        // This callback isn't for our call.
                        return;
                    }

                    GetWeakSafe(Result);
                    Result.OnResult(bCallbackWasSuccessful, CallbackError);

                    // Unregister this callback since we've handled the call we care about.
                    Friends->ClearOnBlockedPlayerCompleteDelegate_Handle(this->LocalUserNum, *CallbackHandle);
            }));

    // Block the target player.
    if (!Friends->BlockPlayer(this->LocalUserNum, *TargetUserId))
    {
        // The call failed to start; unregister callback handler.
        Friends->ClearOnBlockedPlayerCompleteDelegate_Handle(this->LocalUserNum, *CallbackHandle);
        Result->OnResult(false, TEXT("BlockPlayer call failed to start."));
    }
    return Result;
}

sp<TNetResult<>> URpFriends::UnblockPlayer(const FUniqueNetIdRepl& TargetUserId)
{
    auto Result = MakeThreadPtr(TNetResult<>);
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    auto Friends = OSS.GetFriendsInterface();
    if (!Friends.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support friends."));
        return Result;
    }

    // Register an event so we can receive the unblock outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle = Friends->AddOnUnblockedPlayerCompleteDelegate_Handle(
        this->LocalUserNum,
        FOnUnblockedPlayerCompleteDelegate::CreateWeakLambda(
            this,
            [this, Friends, CallbackHandle, TargetUserId, ResultWk = TWeakPtr<TNetResult<>>(Result)]
            (
                int32 CallbackLocalUserNum,
                bool bCallbackWasSuccessful,
                const FUniqueNetId& CallbackTargetUserId,
                const FString&,
                const FString& CallbackError)
            {
                    // Check if this callback is for us.
                    if (CallbackLocalUserNum != this->LocalUserNum || CallbackTargetUserId != *TargetUserId)
                    {
                        // This callback isn't for our call.
                        return;
                    }

                    GetWeakSafe(Result);
                    Result.OnResult(bCallbackWasSuccessful, CallbackError);

                    // Unregister this callback since we've handled the call we care about.
                    Friends->ClearOnUnblockedPlayerCompleteDelegate_Handle(this->LocalUserNum, *CallbackHandle);
            }));

    // Unblock the target player.
    if (!Friends->UnblockPlayer(this->LocalUserNum, *TargetUserId))
    {
        // The call failed to start; unregister callback handler.
        Friends->ClearOnUnblockedPlayerCompleteDelegate_Handle(this->LocalUserNum, *CallbackHandle);
        Result->OnResult(false, TEXT("UnblockPlayer call failed to start."));
    }
    return Result;
}

sp<TNetResult<>> URpFriends::QueryBlockedPlayers()
{
    auto Result = MakeThreadPtr(TNetResult<>);
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    auto Friends = OSS.GetFriendsInterface();
    if (!Friends.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support friends."));
        return Result;
    }

    // Register an event so we can receive the query outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle = Friends->AddOnQueryBlockedPlayersCompleteDelegate_Handle(
        FOnQueryBlockedPlayersCompleteDelegate::CreateWeakLambda(
            this,
            [Friends, CallbackHandle, UserId, ResultWk = TWeakPtr<TNetResult<>>(Result)]
            (
                const FUniqueNetId& CallbackUserId,
                bool bCallbackWasSuccessful,
                const FString& CallbackError) 
            {
                    // Check if this callback is for us.
                    if (CallbackUserId != *UserId)
                    {
                        // This callback isn't for our call.
                        return;
                    }

                    GetWeakSafe(Result);
                    Result.OnResult(bCallbackWasSuccessful, CallbackError);

                    // Unregister this callback since we've handled the call we care about.
                    Friends->ClearOnQueryBlockedPlayersCompleteDelegate_Handle(*CallbackHandle);
            }));

    // Ask the online subsystem to query the blocked players. We don't return blocked players inside
    // ExecuteFriendsQueryBlockedPlayers; we're just getting the online subsystem to cache them so that a later
    // GetFriendsCurrentBlockedPlayers can return them.
    if (!Friends->QueryBlockedPlayers(*UserId))
    {
        // The call failed to start; unregister callback handler.
        Friends->ClearOnQueryBlockedPlayersCompleteDelegate_Handle(*CallbackHandle);
        Result->OnResult(false, TEXT("QueryBlockedPlayers call failed to start."));
    }
    return Result;
}

TArray<TSharedRef<FOnlineBlockedPlayer>> URpFriends::GetCurrentBlockedPlayers() const
{
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    auto Friends = OSS.GetFriendsInterface();
    if (!Friends.IsValid())
        return TArray<TSharedRef<FOnlineBlockedPlayer>>();

    // Get the blocked players.
    TArray<TSharedRef<FOnlineBlockedPlayer>> BlockedPlayers;
    if (!Friends->GetBlockedPlayers(*UserId, BlockedPlayers))
        Print("Mo Blocked Players Found");
    return BlockedPlayers;
}

sp<TNetResult<>> URpFriends::SendInviteByFriendCode(const FString& FriendCode)
{
    auto Result = MakeThreadPtr(TNetResult<>);
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    auto Friends = OSS.GetFriendsInterface();
    if (!Friends.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support friends."));
        return Result;
    }

    // Get the user interface, if the online subsystem supports it.
    auto User = OSS.GetUserInterface();
    if (!User.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support user querying."));
        return Result;
    }

    // Query user ID mapping by friend code.
    if (!User->QueryUserIdMapping(
        *UserId,
        FString::Printf(TEXT("FriendCode:%s"), *FriendCode),
        IOnlineUser::FOnQueryUserMappingComplete::CreateWeakLambda(
            this,
            [this, Result]
            (
                bool bWasSuccessful,
                const FUniqueNetId&,
                const FString&,
                const FUniqueNetId& FoundUserId,
                const FString& Error)
            {
                if (!bWasSuccessful)
                {
                    Result->OnResult(bWasSuccessful, Error);
                    return;
                }

                // Forward onto our existing SendInvite handling function, which
                // will also call Result->OnResult.
                this->SendInvite(FoundUserId, Result);
            })))
    {
        // The call failed to start.
        Result->OnResult(false, TEXT("QueryUserIdMapping call failed to start."));
    }

    return Result;
}

