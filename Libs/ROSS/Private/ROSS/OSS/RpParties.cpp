// Fill out your copyright notice in the Description page of Project Settings.


#include "ROSS/OSS/RpParties.h"
#include "OnlineSubsystemUtils.h"

URpParties::URpParties()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URpParties::BeginPlay()
{
	Super::BeginPlay();
}

TArray<TSharedRef<const FOnlinePartyId>> URpParties::GetPartiesJoinedParties() const
{
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    // Get the party interface, if the online subsystem supports it.
    auto PartySystem = OSS.GetPartyInterface();
    if (!PartySystem.IsValid())
    {
        return TArray<TSharedRef<const FOnlinePartyId>>();
    }

    // Get the list of parties currently joined.
    TArray<TSharedRef<const FOnlinePartyId>> PartyIds;
    if(!PartySystem->GetJoinedParties(*UserId, PartyIds))
        Print("No Joined Parties Found");
    return PartyIds;
}

sp<TNetResult<>> URpParties::CreateParty()
{
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    auto Result = MakeThreadPtr(TNetResult<>);
    // Get the party interface, if the online subsystem supports it.
    auto PartySystem = OSS.GetPartyInterface();
    if (!PartySystem.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support parties."));
        return Result;
    }

    // Create the party.
    auto PartyConfiguration = FPartyConfiguration{};
    PartyConfiguration.JoinRequestAction = EJoinRequestAction::AutoApprove;
    PartyConfiguration.PresencePermissions = PartySystemPermissions::EPermissionType::Anyone;
    PartyConfiguration.InvitePermissions = PartySystemPermissions::EPermissionType::Anyone;
    PartyConfiguration.bChatEnabled = true;
    PartyConfiguration.bShouldRemoveOnDisconnection = false;
    PartyConfiguration.bIsAcceptingMembers = true;
    PartyConfiguration.NotAcceptingMembersReason = 0;
    PartyConfiguration.MaxMembers = 4;
    PartyConfiguration.Nickname = TEXT("");
    PartyConfiguration.Description = TEXT("");
    PartyConfiguration.Password = TEXT("");
    PartySystem->CreateParty(
        *UserId,
        PartySystem->GetPrimaryPartyTypeId(),
        PartyConfiguration,
        FOnCreatePartyComplete::CreateWeakLambda(
            this,
            [ResultWk = TWeakPtr<TNetResult<>>(Result)]
            (
                const FUniqueNetId&,
                const TSharedPtr<const FOnlinePartyId>& PartyId,
                const ECreatePartyCompletionResult PartyResult) 
            {
                    GetWeakSafe(Result);

                    // Return the success status.
                    FString ErrorMessage;
                    switch (PartyResult)
                    {
                    case ECreatePartyCompletionResult::UnknownClientFailure:
                        ErrorMessage = TEXT("UnknownClientFailure");
                        break;
                    case ECreatePartyCompletionResult::AlreadyInPartyOfSpecifiedType:
                        ErrorMessage = TEXT("AlreadyInPartyOfSpecifiedType");
                        break;
                    case ECreatePartyCompletionResult::AlreadyCreatingParty:
                        ErrorMessage = TEXT("AlreadyCreatingParty");
                        break;
                    case ECreatePartyCompletionResult::AlreadyInParty:
                        ErrorMessage = TEXT("AlreadyInParty");
                        break;
                    case ECreatePartyCompletionResult::FailedToCreateMucRoom:
                        ErrorMessage = TEXT("FailedToCreateMucRoom");
                        break;
                    case ECreatePartyCompletionResult::NoResponse:
                        ErrorMessage = TEXT("NoResponse");
                        break;
                    case ECreatePartyCompletionResult::LoggedOut:
                        ErrorMessage = TEXT("LoggedOut");
                        break;
                    case ECreatePartyCompletionResult::NotPrimaryUser:
                        ErrorMessage = TEXT("NotPrimaryUser");
                        break;
                    case ECreatePartyCompletionResult::UnknownInternalFailure:
                        ErrorMessage = TEXT("UnknownInternalFailure");
                        break;
                    default:
                    case ECreatePartyCompletionResult::Succeeded:
                        break;
                    }
                    Result.OnResult(PartyResult == ECreatePartyCompletionResult::Succeeded, ErrorMessage);
            }));
    return Result;
}

TArray<IOnlinePartyJoinInfoConstRef> URpParties::GetPartiesCurrentInvites() const
{
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    auto PartySystem = OSS.GetPartyInterface();
    if (!PartySystem.IsValid())
        return TArray<IOnlinePartyJoinInfoConstRef>();

    // Get the list of current party invites.
    TArray<IOnlinePartyJoinInfoConstRef> PartyInvites;
    if(!PartySystem->GetPendingInvites(*UserId, PartyInvites))
        Print("No Party Invites Found");

    return PartyInvites;
}

sp<TNetResult<>>URpParties::AcceptInvite(const IOnlinePartyJoinInfoConstRef& Invite)
{
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    auto Result = MakeThreadPtr(TNetResult<>);
    // Get the party interface, if the online subsystem supports it.
    auto PartySystem = OSS.GetPartyInterface();
    if (!PartySystem.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support parties."));
        return Result;
    }

    // Locate the invite so we can join it.
    IOnlinePartyJoinInfoConstPtr SelectedPartyInvite;
    TArray<IOnlinePartyJoinInfoConstRef> PartyInvites;
    PartySystem->GetPendingInvites(*UserId, PartyInvites);
    for (const auto& PartyInvite : PartyInvites)
    {
        if (PartyInvite->GetPartyId()->ToString() == Invite->GetPartyId()->ToString())
        {
            SelectedPartyInvite = PartyInvite;
            break;
        }
    }
    if (!SelectedPartyInvite.IsValid())
    {
        Result->OnResult(false, TEXT("Invite not found."));
        return Result;
    }

    // Join the party.
    PartySystem->JoinParty(
        *UserId,
        *SelectedPartyInvite,
        FOnJoinPartyComplete::CreateWeakLambda(
            this,
            [ResultWk = TWeakPtr<TNetResult<>>(Result)]
            (
                const FUniqueNetId&,
                const FOnlinePartyId&,
                const EJoinPartyCompletionResult PartyResult,
                const int32) 
            {
                    GetWeakSafe(Result);

                    // Return the success status.
                    FString ErrorMessage;
                    switch (PartyResult)
                    {
                    case EJoinPartyCompletionResult::UnknownClientFailure:
                        ErrorMessage = TEXT("UnknownClientFailure");
                        break;
                    case EJoinPartyCompletionResult::BadBuild:
                        ErrorMessage = TEXT("BadBuild");
                        break;
                    case EJoinPartyCompletionResult::InvalidAccessKey:
                        ErrorMessage = TEXT("InvalidAccessKey");
                        break;
                    case EJoinPartyCompletionResult::AlreadyInLeadersJoiningList:
                        ErrorMessage = TEXT("AlreadyInLeadersJoiningList");
                        break;
                    case EJoinPartyCompletionResult::AlreadyInLeadersPartyRoster:
                        ErrorMessage = TEXT("AlreadyInLeadersPartyRoster");
                        break;
                    case EJoinPartyCompletionResult::NoSpace:
                        ErrorMessage = TEXT("NoSpace");
                        break;
                    case EJoinPartyCompletionResult::NotApproved:
                        ErrorMessage = TEXT("NotApproved");
                        break;
                    case EJoinPartyCompletionResult::RequesteeNotMember:
                        ErrorMessage = TEXT("RequesteeNotMember");
                        break;
                    case EJoinPartyCompletionResult::RequesteeNotLeader:
                        ErrorMessage = TEXT("RequesteeNotLeader");
                        break;
                    case EJoinPartyCompletionResult::NoResponse:
                        ErrorMessage = TEXT("NoResponse");
                        break;
                    case EJoinPartyCompletionResult::LoggedOut:
                        ErrorMessage = TEXT("LoggedOut");
                        break;
                    case EJoinPartyCompletionResult::IncompatiblePlatform:
                        ErrorMessage = TEXT("IncompatiblePlatform");
                        break;
                    case EJoinPartyCompletionResult::AlreadyJoiningParty:
                        ErrorMessage = TEXT("AlreadyJoiningParty");
                        break;
                    case EJoinPartyCompletionResult::AlreadyInParty:
                        ErrorMessage = TEXT("AlreadyInParty");
                        break;
                    case EJoinPartyCompletionResult::JoinInfoInvalid:
                        ErrorMessage = TEXT("JoinInfoInvalid");
                        break;
                    case EJoinPartyCompletionResult::AlreadyInPartyOfSpecifiedType:
                        ErrorMessage = TEXT("AlreadyInPartyOfSpecifiedType");
                        break;
                    case EJoinPartyCompletionResult::MessagingFailure:
                        ErrorMessage = TEXT("MessagingFailure");
                        break;
                    case EJoinPartyCompletionResult::GameSpecificReason:
                        ErrorMessage = TEXT("GameSpecificReason");
                        break;
                    case EJoinPartyCompletionResult::MismatchedApp:
                        ErrorMessage = TEXT("MismatchedApp");
                        break;
                    case EJoinPartyCompletionResult::UnknownInternalFailure:
                        ErrorMessage = TEXT("UnknownInternalFailure");
                        break;
                    default:
                    case EJoinPartyCompletionResult::Succeeded:
                        break;
                    }
                    Result.OnResult(PartyResult == EJoinPartyCompletionResult::Succeeded, ErrorMessage);
            }));
    return Result;
}

TArray<FUniqueNetIdRepl> URpParties::GetPartiesCurrentMembers(const FString& PartyIdStr) const
{
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    auto PartySystem = OSS.GetPartyInterface();
    if (!PartySystem.IsValid()){
        return TArray<FUniqueNetIdRepl>();
    }

    // Get the requested party.
    TSharedPtr<const FOnlinePartyId> SelectedPartyId;
    TArray<TSharedRef<const FOnlinePartyId>> PartyIds;
    PartySystem->GetJoinedParties(*UserId, PartyIds);
    for (const auto& PartyId : PartyIds)
    {
        if (PartyId->ToString() == PartyIdStr)
        {
            SelectedPartyId = PartyId;
            break;
        }
    }
    if (!SelectedPartyId.IsValid())
    {
        return TArray<FUniqueNetIdRepl>();
    }

    // Get the list of members in that party.
    TArray<FUniqueNetIdRepl> Entries;
    TArray<FOnlinePartyMemberConstRef> PartyMembers;
    PartySystem->GetPartyMembers(*UserId, *SelectedPartyId, PartyMembers);
    for (const auto& PartyMember : PartyMembers)
    {
        Entries.Add(PartyMember->GetUserId());
    }
    return Entries;
}

sp<TNetResult<>> URpParties::LeaveParty(const FString& PartyIdStr)
{
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    auto Result = MakeThreadPtr(TNetResult<>);
    auto PartySystem = OSS.GetPartyInterface();
    if (!PartySystem.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support parties."));
        return Result;
    }

    // Locate the party so we can leave it.
    TSharedPtr<const FOnlinePartyId> SelectedPartyId;
    TArray<TSharedRef<const FOnlinePartyId>> PartyIds;
    PartySystem->GetJoinedParties(*UserId, PartyIds);
    for (const auto& PartyId : PartyIds)
    {
        if (PartyId->ToString() == PartyIdStr)
        {
            SelectedPartyId = PartyId;
            break;
        }
    }
    if (!SelectedPartyId.IsValid())
    {
        Result->OnResult(false, TEXT("Party not found."));
        return Result;
    }

    // Leave the party.
    PartySystem->LeaveParty(
        *UserId,
        *SelectedPartyId,
        true,
        FOnLeavePartyComplete::CreateWeakLambda(
            this,
            [ResultWk = TWeakPtr<TNetResult<>>(Result)]
            (const FUniqueNetId&, const FOnlinePartyId&, const ELeavePartyCompletionResult PartyResult) 
            {
                    GetWeakSafe(Result);
                    // Return the success status.
                    FString ErrorMessage;
                    switch (PartyResult)
                    {
                    case ELeavePartyCompletionResult::UnknownClientFailure:
                        ErrorMessage = TEXT("UnknownClientFailure");
                        break;
                    case ELeavePartyCompletionResult::NoResponse:
                        ErrorMessage = TEXT("NoResponse");
                        break;
                    case ELeavePartyCompletionResult::LoggedOut:
                        ErrorMessage = TEXT("LoggedOut");
                        break;
                    case ELeavePartyCompletionResult::UnknownParty:
                        ErrorMessage = TEXT("UnknownParty");
                        break;
                    case ELeavePartyCompletionResult::LeavePending:
                        ErrorMessage = TEXT("LeavePending");
                        break;
                    case ELeavePartyCompletionResult::UnknownLocalUser:
                        ErrorMessage = TEXT("UnknownLocalUser");
                        break;
                    case ELeavePartyCompletionResult::NotMember:
                        ErrorMessage = TEXT("NotMember");
                        break;
                    case ELeavePartyCompletionResult::MessagingFailure:
                        ErrorMessage = TEXT("MessagingFailure");
                        break;
                    case ELeavePartyCompletionResult::UnknownTransportFailure:
                        ErrorMessage = TEXT("UnknownTransportFailure");
                        break;
                    case ELeavePartyCompletionResult::UnknownInternalFailure:
                        ErrorMessage = TEXT("UnknownInternalFailure");
                        break;
                    default:
                    case ELeavePartyCompletionResult::Succeeded:
                        break;
                    }
                    Result.OnResult(PartyResult == ELeavePartyCompletionResult::Succeeded, ErrorMessage);
            }));
    return Result;
}

sp<TNetResult<>> URpParties::KickMember(
    const FString& PartyIdStr,
    const FUniqueNetIdRepl& MemberId
){
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    auto Result = MakeThreadPtr(TNetResult<>);
    auto PartySystem = OSS.GetPartyInterface();
    if (!PartySystem.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support parties."));
        return Result;
    }

    // Locate the party so we can leave it.
    TSharedPtr<const FOnlinePartyId> SelectedPartyId;
    TArray<TSharedRef<const FOnlinePartyId>> PartyIds;
    PartySystem->GetJoinedParties(*UserId, PartyIds);
    for (const auto& PartyId : PartyIds)
    {
        if (PartyId->ToString() == PartyIdStr)
        {
            SelectedPartyId = PartyId;
            break;
        }
    }
    if (!SelectedPartyId.IsValid())
    {
        Result->OnResult(false, TEXT("Party not found."));
        return Result;
    }

    // Kick the member from the party.
    PartySystem->KickMember(
        *UserId,
        *SelectedPartyId,
        *MemberId,
        FOnKickPartyMemberComplete::CreateWeakLambda(
            this,
            [ResultWk = TWeakPtr<TNetResult<>>(Result)]
            (
                const FUniqueNetId&,
                const FOnlinePartyId&,
                const FUniqueNetId&,
                const EKickMemberCompletionResult PartyResult
            ){
                    GetWeakSafe(Result);
                    // Return the success status.
                    FString ErrorMessage;
                    switch (PartyResult)
                    {
                    case EKickMemberCompletionResult::UnknownClientFailure:
                        ErrorMessage = TEXT("UnknownClientFailure");
                        break;
                    case EKickMemberCompletionResult::UnknownParty:
                        ErrorMessage = TEXT("UnknownParty");
                        break;
                    case EKickMemberCompletionResult::LocalMemberNotMember:
                        ErrorMessage = TEXT("LocalMemberNotMember");
                        break;
                    case EKickMemberCompletionResult::LocalMemberNotLeader:
                        ErrorMessage = TEXT("LocalMemberNotLeader");
                        break;
                    case EKickMemberCompletionResult::RemoteMemberNotMember:
                        ErrorMessage = TEXT("RemoteMemberNotMember");
                        break;
                    case EKickMemberCompletionResult::MessagingFailure:
                        ErrorMessage = TEXT("MessagingFailure");
                        break;
                    case EKickMemberCompletionResult::NoResponse:
                        ErrorMessage = TEXT("NoResponse");
                        break;
                    case EKickMemberCompletionResult::LoggedOut:
                        ErrorMessage = TEXT("LoggedOut");
                        break;
                    case EKickMemberCompletionResult::UnknownInternalFailure:
                        ErrorMessage = TEXT("UnknownInternalFailure");
                        break;
                    default:
                    case EKickMemberCompletionResult::Succeeded:
                        break;
                    }
                    Result.OnResult(PartyResult == EKickMemberCompletionResult::Succeeded, ErrorMessage);
            }));
    return Result;
}

sp<TNetResult<>> URpParties::InviteFriend(
    const FString& PartyIdStr,
    const FUniqueNetIdRepl& MemberId
    )
{
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    auto Result = MakeThreadPtr(TNetResult<>);
    auto PartySystem = OSS.GetPartyInterface();
    if (!PartySystem.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support parties."));
        return Result;
    }

    // Locate the party so we can send the invitation.
    TSharedPtr<const FOnlinePartyId> SelectedPartyId;
    TArray<TSharedRef<const FOnlinePartyId>> PartyIds;
    PartySystem->GetJoinedParties(*UserId, PartyIds);
    for (const auto& PartyId : PartyIds)
    {
        if (PartyId->ToString() == PartyIdStr)
        {
            SelectedPartyId = PartyId;
            break;
        }
    }
    if (!SelectedPartyId.IsValid())
    {
        Result->OnResult(false, TEXT("Party not found."));
        return Result;
    }

    // Send the invitation to the friend.
    PartySystem->SendInvitation(
        *UserId,
        *SelectedPartyId,
        FPartyInvitationRecipient(*MemberId),
        FOnSendPartyInvitationComplete::CreateWeakLambda(
            this,
            [ResultWk = TWeakPtr<TNetResult<>>(Result)]
            (
                const FUniqueNetId&,
                const FOnlinePartyId&,
                const FUniqueNetId&,
                const ESendPartyInvitationCompletionResult PartyResult) 
            {
                    GetWeakSafe(Result);
                    // Return the success status.
                    FString ErrorMessage;
                    switch (PartyResult)
                    {
                    case ESendPartyInvitationCompletionResult::NotLoggedIn:
                        ErrorMessage = TEXT("NotLoggedIn");
                        break;
                    case ESendPartyInvitationCompletionResult::InvitePending:
                        ErrorMessage = TEXT("InvitePending");
                        break;
                    case ESendPartyInvitationCompletionResult::AlreadyInParty:
                        ErrorMessage = TEXT("AlreadyInParty");
                        break;
                    case ESendPartyInvitationCompletionResult::PartyFull:
                        ErrorMessage = TEXT("PartyFull");
                        break;
                    case ESendPartyInvitationCompletionResult::NoPermission:
                        ErrorMessage = TEXT("NoPermission");
                        break;
                    case ESendPartyInvitationCompletionResult::RateLimited:
                        ErrorMessage = TEXT("RateLimited");
                        break;
                    case ESendPartyInvitationCompletionResult::UnknownInternalFailure:
                        ErrorMessage = TEXT("UnknownInternalFailure");
                        break;
                    default:
                    case ESendPartyInvitationCompletionResult::Succeeded:
                        break;
                    }
                    Result.OnResult(PartyResult == ESendPartyInvitationCompletionResult::Succeeded, ErrorMessage);
            }));
    return Result;
}


