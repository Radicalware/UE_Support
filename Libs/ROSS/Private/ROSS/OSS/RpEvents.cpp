// Fill out your copyright notice in the Description page of Project Settings.


#include "ROSS/OSS/RpEvents.h"
#include "Interfaces/OnlineFriendsInterface.h"
#include "Interfaces/OnlinePartyInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSubsystemUtils.h"

URpEvents::URpEvents()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URpEvents::BeginPlay()
{
	Super::BeginPlay();
}

void URpEvents::RaiseFriendsOnFriendsChange()
{
    FriendsOnFriendsChange.Broadcast();
}

void URpEvents::RaisePartiesOnPartiesStateChanged()
{
    PartiesOnPartiesStateChanged.Broadcast();
}

void URpEvents::RegisterEvents()
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetOuterWorld());
    if (OSS == nullptr)
    {
        return;
    }

    // Get the friends interface. 
    // If it's valid, create an event handler to notify UI when the friends list changes.
    auto Friends = OSS->GetFriendsInterface();
    if (Friends.IsValid())
    {
        Friends->AddOnFriendsChangeDelegate_Handle(
            this->LocalUserNum,
            FOnFriendsChangeDelegate::CreateWeakLambda(this, [this]() {
                this->RaiseFriendsOnFriendsChange();
                }));
    }

    // Party event handlers need to check if the incoming event is for the local user that this UI is for. 
    // Define a delegate that we can re-use that does this logic safely,
    // without holding pointers longer than is safe.
    auto IsForLocalUser =
        TDelegate<bool(const FUniqueNetId&)>::CreateWeakLambda(this,
            [this]
            (const FUniqueNetId& LocalUserId)
            {
                auto DelegateOSS = Online::GetSubsystem(this->GetOuterWorld());
                if (DelegateOSS != nullptr)
                {
                    auto Identity = DelegateOSS->GetIdentityInterface();
                    if (Identity.IsValid())
                    {
                        auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
                        if (UserId.IsValid() && *UserId == LocalUserId)
                        {
                            return true;
                        }
                    }
                }
                return false;
            });

    // Get the party interface. If it's valid, wire up event handlers to notify the UI when parties change state.
    auto PartySystem = OSS->GetPartyInterface();
    if (PartySystem.IsValid())
    {
        PartySystem->AddOnPartyJoinedDelegate_Handle(FOnPartyJoinedDelegate::CreateWeakLambda(
            this, // Join Party
            [this, IsForLocalUser](const FUniqueNetId& LocalUserId, const FOnlinePartyId&) {
                if (IsForLocalUser.IsBound() && IsForLocalUser.Execute(LocalUserId))
                {
                    this->RaisePartiesOnPartiesStateChanged();
                }
            }));
        PartySystem->AddOnPartyExitedDelegate_Handle(FOnPartyExitedDelegate::CreateWeakLambda(
            this, // Exit Party
            [this, IsForLocalUser](const FUniqueNetId& LocalUserId, const FOnlinePartyId&) {
                if (IsForLocalUser.IsBound() && IsForLocalUser.Execute(LocalUserId))
                {
                    this->RaisePartiesOnPartiesStateChanged();
                }
            }));
        PartySystem->AddOnPartyMemberJoinedDelegate_Handle(FOnPartyMemberJoinedDelegate::CreateWeakLambda(
            this, // Member Joined Party
            [this, IsForLocalUser](const FUniqueNetId& LocalUserId, const FOnlinePartyId&, const FUniqueNetId&) {
                if (IsForLocalUser.IsBound() && IsForLocalUser.Execute(LocalUserId))
                {
                    this->RaisePartiesOnPartiesStateChanged();
                }
            }));
        PartySystem->AddOnPartyMemberExitedDelegate_Handle(FOnPartyMemberExitedDelegate::CreateWeakLambda(
            this, // Member Exited Party
            [this, IsForLocalUser](
                const FUniqueNetId& LocalUserId,
                const FOnlinePartyId&,
                const FUniqueNetId&,
                EMemberExitedReason) {
                    if (IsForLocalUser.IsBound() && IsForLocalUser.Execute(LocalUserId))
                    {
                        this->RaisePartiesOnPartiesStateChanged();
                    }
            }));
        PartySystem->AddOnPartyInvitesChangedDelegate_Handle(FOnPartyInvitesChangedDelegate::CreateWeakLambda(
            this, // Invites Changed
            [this, IsForLocalUser](const FUniqueNetId& LocalUserId) {
                if (IsForLocalUser.IsBound() && IsForLocalUser.Execute(LocalUserId))
                {
                    this->RaisePartiesOnPartiesStateChanged();
                }
            }));
    }

    // If the party interface is valid, wire up the event handler to join sessions along with the party.
    if (PartySystem.IsValid())
    {
        PartySystem->AddOnPartyDataReceivedDelegate_Handle(FOnPartyDataReceivedDelegate::CreateWeakLambda(
            this,
            [this, IsForLocalUser]
            (
                const FUniqueNetId& LocalUserId,
                const FOnlinePartyId& PartyId,
                const FName& Namespace,
                const FOnlinePartyData& PartyData)
            {
                if (!(IsForLocalUser.IsBound() && IsForLocalUser.Execute(LocalUserId)))
                    return;

                FVariantData JoinSessionIdFromPartyAttr;
                PartyData.GetAttribute(TEXT("JoinSessionIdFromParty"), JoinSessionIdFromPartyAttr);
                FString JoinSessionIdFromParty;
                JoinSessionIdFromPartyAttr.GetValue(JoinSessionIdFromParty);

                // If "JoinSessionIdFromParty" isn't empty and we haven't used it yet...
                if (!(!JoinSessionIdFromParty.IsEmpty() && JoinSessionIdFromParty != MsJoinSessionIdFromParty))
                    return;

                MsJoinSessionIdFromParty = JoinSessionIdFromParty;

                // Find the session, join it, and then join the game server.
                auto CallbackOSS = Online::GetSubsystem(this->GetOuterWorld());
                if (CallbackOSS == nullptr)
                {
                    return;
                }
                auto Session = CallbackOSS->GetSessionInterface();
                if (!Session.IsValid())
                {
                    return;
                }

                // FVariantData.GetValue >> Join Session ID >> Create Session ID >> Find Session by ID
                auto SessionIdPtr = Session->CreateSessionIdFromString(JoinSessionIdFromParty);
                if (!SessionIdPtr.IsValid())
                {
                    return;
                }

                // Find Session by ID >> Join Session >> Join Complete >> Client Travel
                Session->FindSessionById(
                    LocalUserId,
                    *SessionIdPtr,
                    LocalUserId,
                    FOnSingleSessionResultCompleteDelegate::CreateWeakLambda(
                        this,
                        [this, Session]
                        (int32, bool bWasSuccessful, const FOnlineSessionSearchResult& SearchResult)
                        {
                            if (!bWasSuccessful)
                                return;
                            auto CallbackHandle = MakeShared<FDelegateHandle>();
                            *CallbackHandle = Session->AddOnJoinSessionCompleteDelegate_Handle(
                                FOnJoinSessionCompleteDelegate::CreateWeakLambda(
                                    this,
                                    [this, Session, CallbackHandle]
                                    (FName CallbackSessionName, EOnJoinSessionCompleteResult::Type CallbackResult)
                                    {
                                        if (!CallbackSessionName.IsEqual(NAME_GameSession))
                                        {
                                            return;
                                        }
                                        if (CallbackResult != EOnJoinSessionCompleteResult::Success)
                                        {
                                            return;
                                        }
                                        FString ConnectInfo;
                                        Session->GetResolvedConnectString(NAME_GameSession, ConnectInfo);
                                        GEngine->SetClientTravel(
                                            this->GetOuterWorld(),
                                            *ConnectInfo,
                                            TRAVEL_Absolute);
                                        Session->ClearOnJoinSessionCompleteDelegate_Handle(*CallbackHandle);
                                    }));
                            if (!Session->JoinSession(this->LocalUserNum, NAME_GameSession, SearchResult))
                            {
                                Session->ClearOnJoinSessionCompleteDelegate_Handle(*CallbackHandle);
                                return;
                            }
                        }));
            }));
    }
}
