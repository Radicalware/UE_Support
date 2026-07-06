#include "ROSS/OSS/RpSessions.h"
#include "OnlineSubsystemUtils.h"
#include "Templates/SharedPointer.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerState.h"
#include "Interfaces/OnlinePartyInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Online/OnlineSessionNames.h"

#include "Online/OnlineSessionNames.h"
#include "ROSS/Util/NetResult.h"
#include "ROSS/ROSS.h"

URpSessions::URpSessions()
{
    MmSessionSearchResultsPtr = MakeShared<TMap<FString, FOnlineSessionSearchResult>>();
    MoSessionResultsPtr = MakeShared<TNetResult<FOnlineSessionSearch>>();
}

IOnlineSessionPtr URpSessions::GetSessionPtr(const IOnlineSubsystem& OSS) const
{
    auto Session = OSS.GetSessionInterface();
    ensureMsgf(Session.IsValid(), TEXT("Online subsystem does not support sessions."));
    return Session;
}

void URpSessions::BeginPlay()
{
    Super::BeginPlay();
}

// ExecuteSessionsFindSessions
TSharedPtr<TNetResult<FOnlineSessionSearch>> URpSessions::GetSessions(FVVDelegate&& FoDelegate, int32 FnMaxResults)
{
    auto& OSS = UROSS::GetSubsystem();
    auto SessionPtr = GetSessionPtr(OSS);
    GET(Session);

    GET(MoSessionResults);
    MoSessionResults.SetExternalCallback(FoDelegate);

    sp<FOnlineSessionSearch> LoSearchSettingsPtr = MakeThreadPtr(FOnlineSessionSearch);
    MoSessionResults.SetResult(LoSearchSettingsPtr);
    GET(LoSearchSettings);

    if (OSS.IsDedicated())
    {
        LoSearchSettings.QuerySettings.Set(SEARCH_DEDICATED_ONLY, true, EOnlineComparisonOp::Equals);
        LoSearchSettings.bIsLanQuery = false;
    }
    else {
        LoSearchSettings.QuerySettings.Set(SEARCH_DEDICATED_ONLY, false, EOnlineComparisonOp::Equals);
        LoSearchSettings.bIsLanQuery = true;
    }
    LoSearchSettings.MaxSearchResults = FnMaxResults;

    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle =
        Session.AddOnFindSessionsCompleteDelegate_Handle(FOnFindSessionsCompleteDelegate::CreateWeakLambda(
            this,
            [this, SessionPtr, CallbackHandle, ResultWk = TWeakPtr<TNetResult<FOnlineSessionSearch>>(MoSessionResultsPtr)]
            (bool bCallbackWasSuccessful)
            {
                GET(Session);
                if (!ResultWk.IsValid())
                {
                    PrintW("Result pointer is no longer valid.");
                    Session.ClearOnFindSessionsCompleteDelegate_Handle(*CallbackHandle);
                    return;
                }
                GetWeakSafe(Result);
                Print("Found Session Count: ", Result.GetResult().SearchResults.Num(), " <> ", Session.GetNumSessions());

                if (!bCallbackWasSuccessful)
                {
                    Result.OnReturnResult(false, nullptr, TEXT("Find sessions operation failed."));
                    Session.ClearOnFindSessionsCompleteDelegate_Handle(*CallbackHandle);
                    return;
                }
                else
                    SessionPtr->GetNumSessions();

                auto& LoSearch = Result.GetResult();
                // Check if this callback is for us.
                if (LoSearch.SearchState != EOnlineAsyncTaskState::Failed &&
                    LoSearch.SearchState != EOnlineAsyncTaskState::Done)
                {
                    // This callback isn't for our call.
                    return;
                }
                // Return if the read failed.
                if (LoSearch.SearchState == EOnlineAsyncTaskState::Failed)
                {
                    Result.OnReturnResult(false, MoSessionResultsPtr, TEXT("Session search failed."));
                    Session.ClearOnFindSessionsCompleteDelegate_Handle(*CallbackHandle);
                    return;
                }

                GET(MmSessionSearchResults);
                MmSessionSearchResults.Empty();
                for(auto& SearchResult : LoSearch.SearchResults){
                    MmSessionSearchResults.Add(SearchResult.GetSessionIdStr(), SearchResult);
                }

                Result.OnReturnResult(true, MoSessionResultsPtr, TEXT(""));
                Session.ClearOnFindSessionsCompleteDelegate_Handle(*CallbackHandle);
            }));

    if (!Session.FindSessions(LocalUserNum, LoSearchSettingsPtr.ToSharedRef()))
    {
        MoSessionResultsPtr->OnReturnResult(false, MoSessionResultsPtr, TEXT("FindSessions call failed to start."));
        Session.ClearOnFindSessionsCompleteDelegate_Handle(*CallbackHandle);
    }

    return MoSessionResultsPtr;
}

void URpSessions::SearchSessions(int32 FnMaxResults)
{
    MoSessionResultsPtr = The.GetSessions(
        FVVDelegate::CreateUObject(this, &URpSessions::OnSearchSessionsComplete), 
        FnMaxResults);
}

void URpSessions::OnSearchSessionsComplete()
{
    PrintStart();
    if (!MoSessionResultsPtr.IsValid())
    {
        PrintW("Session search failed");
        SearchSessions();
        return;
    }

    const auto& LoSessions = MoSessionResultsPtr->GetResult();

    Print("\n",
        " >> Session Count:    ", LoSessions.SearchResults.Num(), "\n",
        " >> Session State:    ", EOnlineAsyncTaskState::ToString(LoSessions.SearchState), "\n",
        " >> MaxSearchResults: ", LoSessions.MaxSearchResults, "\n",
        " >> bIsLanQuery:      ", LoSessions.bIsLanQuery, "\n",
        " >> PingBucketSize:   ", LoSessions.PingBucketSize, "\n",
        " >> PlatformHash:     ", LoSessions.PlatformHash, "\n",
        " >> TimeoutInSeconds: ", LoSessions.TimeoutInSeconds, "\n"
    );

    for (auto& LoSearch : LoSessions.SearchResults)
    {
        if (!LoSearch.IsValid())
            continue;
        auto LoID = LoSearch.GetSessionIdStr();
        auto& LoSession = LoSearch.Session;
        Print("\n",
            " >> OwningUserId:     ", LoSession.OwningUserId->ToString(), "\n",
            " >> OwningUserName:   ", LoSession.OwningUserName, "\n",
            " >> Public Connects:  ", LoSession.NumOpenPublicConnections, "\n",
            " >> Private Connects: ", LoSession.NumOpenPrivateConnections, "\n"
        );
    }
}

FString URpSessions::GetSessionId(FName SessionName) const
{
    auto& OSS = GetIOnlineSubsytem();
    auto Session = GetSessionPtr(OSS);

    auto* NamedSession = Session->GetNamedSession(SessionName);
    if (NamedSession == nullptr)
    {
        return TEXT("");
    }
    return NamedSession->GetSessionIdStr();
}

TArray<FUniqueNetIdRepl> URpSessions::GetSessionRegisteredPlayerIds(FName SessionName) const
{
    auto& OSS = GetIOnlineSubsytem();
    auto Session = GetSessionPtr(OSS);

    // Get the session interface, if the online subsystem supports it.
    if (!Session.IsValid())
        return TArray<FUniqueNetIdRepl>();

    // Get the named session.
    auto* NamedSession = Session->GetNamedSession(SessionName);
    if (NamedSession == nullptr)
        return TArray<FUniqueNetIdRepl>();

    // Return the registered session players.
    TArray<FUniqueNetIdRepl> PlayerIds;
    for (const auto& PlayerId : NamedSession->RegisteredPlayers)
        PlayerIds.Add(PlayerId);
    return PlayerIds;
}

// From: ExecuteSessionsStartListenServer
sp<TNetResult<>> URpSessions::ExeServerStartListenServer(int32 AvailableSlots)
{
    // We need somewhere to store session settings between the main menu and multiplayer map, so that when CreateSession
    // is called on the multiplayer map, it can retrieve the settings that were used to start the listen server. In this
    // example we use the game instance.

    auto Result = MakeThreadPtr(TNetResult<>);
    PrintE("Not Yet Programmed");
    return Result;
}

sp<TNetResult<void, FOnCreateSessionCompleteDelegate>>
URpSessions::ExeServerCreateSession(FOnStartSessionCompleteDelegate&& FoDelegate, const FRossSessionSettings& FoSessionSettings)
{
    // Starts Lobby Mode Session for people to get ready before game actually starts
    // In dedicated open-world, both Create+Start are used at the same time

    const auto LsSessionName = FName(*FoSessionSettings.MsSessionName);

    auto& OSS = GetIOnlineSubsytem();
    auto Session = GetSessionPtr(OSS);
    auto& ISession = *Session;

    auto LoResultPtr = MakeShared<TNetResult<void, FOnCreateSessionCompleteDelegate>>();
    auto& LoResult = LoResultPtr.Get();
    LoResult.SetExternalCallback(FoDelegate);

    // Check if the user is logged in
    auto Identity = OSS.GetIdentityInterface();
    if (!Identity.IsValid())
    {
        LoResultPtr->OnResult(false, TEXT("Subsystem not available or user not logged in."), LsSessionName, false);
        return LoResultPtr;
    }

    // Register an event so we can receive the create outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle =
        Session->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateWeakLambda(
            this,
            [Session, CallbackHandle, LsSessionName, ResultWk = TWeakPtr<TNetResult<void, FOnCreateSessionCompleteDelegate>>(LoResultPtr)]
            (
                FName CallbackSessionName,
                bool bCallbackWasSuccessful) 
            {
                    // Check if this callback is for us.
                    if (!LsSessionName.IsEqual(CallbackSessionName))
                        return;

                    GetWeakSafe(Result);
                    Result.OnResult(
                        bCallbackWasSuccessful,
                        bCallbackWasSuccessful 
                            ? TEXT("") 
                            : TEXT("Create session operation failed."),
                        LsSessionName,           // delegate 1st arg
                        bCallbackWasSuccessful); // delegate 2nd arg

                    // Unregister this callback since we've handled the call we care about.
                    Session->ClearOnCreateSessionCompleteDelegate_Handle(*CallbackHandle);
            }));

    // Create the session.
    if (!Session->CreateSession(this->LocalUserNum, LsSessionName, FoSessionSettings.ToOnlineSessionSettings()))
    {
        PrintW("CreateSession() call failed to start. LsSessionName = ", LsSessionName);
        if (ISession.GetNamedSession(LsSessionName) != nullptr){
            PrintE("Named session object already present right after failure (name collision).");
        }
        Session->ClearOnCreateSessionCompleteDelegate_Handle(*CallbackHandle);
        LoResultPtr->OnResult(false, TEXT("CreateSession call failed to start."), LsSessionName, false);
    }
    return LoResultPtr;
}


sp<TNetResult<void, FOnStartSessionCompleteDelegate>>
URpSessions::ExeServerStartSession(FOnStartSessionCompleteDelegate&& FoDelegate, const FRossSessionSettings& FoSessionSettings)
{
    // called right before moving to the main map+mode (no more joining after this state)
    // In dedicated open-world, both Create+Start are used at the same time
    const auto LsSessionName = FName(*FoSessionSettings.MsSessionName);
    Print("Starting SessionName: ", LsSessionName);
    auto& OSS = GetIOnlineSubsytem();
    auto Session = GetSessionPtr(OSS);
    auto LoResultPtr = MakeShared<TNetResult<void, FOnStartSessionCompleteDelegate>>();
    auto& LoResult = LoResultPtr.Get();
    LoResult.SetExternalCallback(FoDelegate);

    // Register an event so we can receive the outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle =
        Session->AddOnStartSessionCompleteDelegate_Handle(FOnStartSessionCompleteDelegate::CreateWeakLambda(
            this,
            [Session, CallbackHandle, LsSessionName, ResultWk = TWeakPtr<TNetResult<void, FOnStartSessionCompleteDelegate>>(LoResultPtr)]
            (
                FName CallbackSessionName,
                bool bCallbackWasSuccessful) 
            {
                    // Check if this callback is for us.
                    if (!LsSessionName.IsEqual(CallbackSessionName))
                    {
                        // This callback isn't for our call.
                        return;
                    }

                    GetWeakSafe(Result);
                    Result.OnResult(
                        bCallbackWasSuccessful,
                        bCallbackWasSuccessful 
                            ? TEXT("") 
                            : TEXT("Start session operation failed."),
                        LsSessionName,           // delegate 1st arg
                        bCallbackWasSuccessful); // delegate 2nd arg

                    // Unregister this callback since we've handled the call we care about.
                    Session->ClearOnStartSessionCompleteDelegate_Handle(*CallbackHandle);
            }));

    // Start the session.
    if (!Session->StartSession(LsSessionName))
    {
        constexpr auto LbSuccessful = false;
        PrintW("StartSession() call failed to start. LsSessionName = ", LsSessionName);
        Session->ClearOnStartSessionCompleteDelegate_Handle(*CallbackHandle);
    }
    return LoResultPtr;
}

sp<TNetResult<>> URpSessions::ExeServerEndSession(FName SessionName)
{
    auto& OSS = GetIOnlineSubsytem();
    auto Session = GetSessionPtr(OSS);
    auto Result = MakeThreadPtr(TNetResult<>);

    // Register an event so we can receive the outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle = Session->AddOnEndSessionCompleteDelegate_Handle(FOnEndSessionCompleteDelegate::CreateWeakLambda(
        this,
        [Session, CallbackHandle, SessionName, ResultWk = TWeakPtr<TNetResult<>>(Result)](
            FName CallbackSessionName,
            bool bCallbackWasSuccessful) {
                // Check if this callback is for us.
                if (!SessionName.IsEqual(CallbackSessionName))
                {
                    // This callback isn't for our call.
                    return;
                }

                GetWeakSafe(Result);
                Result.OnResult(
                    bCallbackWasSuccessful,
                    bCallbackWasSuccessful ? TEXT("") : TEXT("End session operation failed."));

                // Unregister this callback since we've handled the call we care about.
                Session->ClearOnEndSessionCompleteDelegate_Handle(*CallbackHandle);
        }));

    // End the session.
    if (!Session->EndSession(SessionName))
    {
        Result->OnResult(false, TEXT("EndSession call failed to start."));
        Session->ClearOnEndSessionCompleteDelegate_Handle(*CallbackHandle);
    }
    return Result;
}

sp<TNetResult<>> URpSessions::ExeServerTravelToMapAndMode(const FOnlineSessionSettings& FoSettings)
{
    auto& OSS = GetIOnlineSubsytem();
    auto Session = GetSessionPtr(OSS);
    GET(LoWorld, GetOuterWorld());
    GET(LoOnlineSub, IOnlineSubsystem::Get());
    GET(LoSessionInterface, LoOnlineSub.GetSessionInterface());

    auto Result = MakeThreadPtr(TNetResult<>);

    FString LsMapPath, LsModePath;
    int32 LnPort = 0;
    ensure(FoSettings.Get(TEXT("MAP_PATH"),  LsMapPath));
    ensure(FoSettings.Get(TEXT("MODE_PATH"), LsModePath));
    ensure(FoSettings.Get(TEXT("PORT"), LnPort));
    ensure(LnPort > 0);

    auto LsPlayerName = GetOuterWorld()->GetFirstPlayerController()->GetPlayerState<APlayerState>()->GetPlayerName();

    Print("\nMap: ", LsMapPath, "\n GameMode: ", LsModePath, "\n Port: ", LnPort);
    FString TravelURL = FString::Printf(TEXT("%s?listen&Player=%s&Game=%s"), *LsMapPath, *LsPlayerName, *LsModePath);
    Print("TravelURL: ", TravelURL);
    bool LbSuccess = LoWorld.ServerTravel(TravelURL);
    if (!LbSuccess)
        PrintE("ServerTravel failed!");

    return Result;
}

sp<TNetResult<>> URpSessions::ExeServerDestroySession(FName SessionName)
{
    auto& OSS = GetIOnlineSubsytem();
    auto Session = GetSessionPtr(OSS);
    auto Result = MakeThreadPtr(TNetResult<>);

    // Register an event so we can receive the outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle =
        Session->AddOnDestroySessionCompleteDelegate_Handle(FOnDestroySessionCompleteDelegate::CreateWeakLambda(
            this,
            [Session, CallbackHandle, SessionName, ResultWk = TWeakPtr<TNetResult<>>(Result)]
            (
                FName CallbackSessionName,
                bool bCallbackWasSuccessful
            ){
                    // Check if this callback is for us.
                    if (!SessionName.IsEqual(CallbackSessionName))
                    {
                        // This callback isn't for our call.
                        return;
                    }

                    GetWeakSafe(Result);
                    Result.OnResult(
                        bCallbackWasSuccessful,
                        bCallbackWasSuccessful ? TEXT("") : TEXT("Destroy session operation failed."));

                    // Unregister this callback since we've handled the call we care about.
                    Session->ClearOnDestroySessionCompleteDelegate_Handle(*CallbackHandle);
            }));

    // Destroy the session.
    if (!Session->DestroySession(SessionName))
    {
        Result->OnResult(false, TEXT("DestroySession call failed to start."));
        Session->ClearOnDestroySessionCompleteDelegate_Handle(*CallbackHandle);
    }
    return Result;
}

sp<TNetResult<>> URpSessions::ExeServerRegisterPlayer(
    FName SessionName,
    const FUniqueNetIdRepl& PlayerId,
    bool bWasFromInvite)
{
    auto& OSS = GetIOnlineSubsytem();
    auto Session = GetSessionPtr(OSS);
    auto Result = MakeThreadPtr(TNetResult<>);

    // Register an event so we can receive the outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle =
        Session->AddOnRegisterPlayersCompleteDelegate_Handle(FOnRegisterPlayersCompleteDelegate::CreateWeakLambda(
            this,
            [Session,
            CallbackHandle,
            SessionName,
            ResultWk = TWeakPtr<TNetResult<>>(Result)]
            (FName CallbackSessionName, const TArray<FUniqueNetIdRef>&, bool bCallbackWasSuccessful) 
            {
                // Check if this callback is for us.
                if (!SessionName.IsEqual(CallbackSessionName))
                {
                    // This callback isn't for our call.
                    return;
                }

                GetWeakSafe(Result);
                Result.OnResult(
                    bCallbackWasSuccessful,
                    bCallbackWasSuccessful ? TEXT("") : TEXT("RegisterPlayer operation failed."));

                // Unregister this callback since we've handled the call we care about.
                Session->ClearOnRegisterPlayersCompleteDelegate_Handle(*CallbackHandle);
            }));

    // Register the player.
    if (!Session->RegisterPlayer(SessionName, *PlayerId, bWasFromInvite))
    {
        Result->OnResult(false, TEXT("RegisterPlayer call failed to start."));
        Session->ClearOnRegisterPlayersCompleteDelegate_Handle(*CallbackHandle);
    }
    return Result;
}

sp<TNetResult<>> URpSessions::ExeServerUnregisterPlayer(
    FName SessionName,
    const FUniqueNetIdRepl& PlayerId)
{
    auto& OSS = GetIOnlineSubsytem();
    auto Session = GetSessionPtr(OSS);
    auto Result = MakeThreadPtr(TNetResult<>);

    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle =
        Session->AddOnUnregisterPlayersCompleteDelegate_Handle(FOnUnregisterPlayersCompleteDelegate::CreateWeakLambda(
            this,
            [Session,
            CallbackHandle,
            SessionName,
            ResultWk = TWeakPtr<TNetResult<>>(Result)
            ](FName CallbackSessionName, const TArray<FUniqueNetIdRef>&, bool bCallbackWasSuccessful) {
                // Check if this callback is for us.
                if (!SessionName.IsEqual(CallbackSessionName))
                {
                    // This callback isn't for our call.
                    return;
                }

                GetWeakSafe(Result);
                Result.OnResult(
                    bCallbackWasSuccessful,
                    bCallbackWasSuccessful ? TEXT("") : TEXT("UnregisterPlayer operation failed."));

                // Unregister this callback since we've handled the call we care about.
                Session->ClearOnUnregisterPlayersCompleteDelegate_Handle(*CallbackHandle);
            }));

    // Register the player.
    if (!Session->UnregisterPlayer(SessionName, *PlayerId))
    {
        Result->OnResult(false, TEXT("UnregisterPlayer call failed to start."));
        Session->ClearOnUnregisterPlayersCompleteDelegate_Handle(*CallbackHandle);
    }
    return Result;
}

sp<TNetResult<>> URpSessions::ExeJoinSession(
    const FOnlineSessionSearchResult& SearchResult,
    FName SessionName)
{
    auto& OSS = GetIOnlineSubsytem();
    auto Session = GetSessionPtr(OSS);
    auto Result = MakeThreadPtr(TNetResult<>);

    // Try to get the search result.
    if (MmSessionSearchResultsPtr == nullptr)
    {
        auto LsErr = "No Sessions Found";
        PrintW(LsErr);
        Result->OnResult(false, LsErr);
        return Result;
    }
    GET(MmSessionSearchResults);
    GET(SessionSearchResult, MmSessionSearchResults.Find(SearchResult.GetSessionIdStr()));

    // Register an event so we can receive the outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle = Session->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateWeakLambda(
        this,
        [this, 
        Session, 
        CallbackHandle, 
        SessionName,
        ResultWk = TWeakPtr<TNetResult<>>(Result)
        ](
            FName CallbackSessionName,
            EOnJoinSessionCompleteResult::Type CallbackResult) {
                // Check if this callback is for us.
                if (!SessionName.IsEqual(CallbackSessionName))
                {
                    // This callback isn't for our call.
                    return;
                }

                GetWeakSafe(Result);
                // If we're not successful, return now.
                if (CallbackResult != EOnJoinSessionCompleteResult::Success)
                {
                    FString ErrorMessage;
                    switch (CallbackResult)
                    {
                    case EOnJoinSessionCompleteResult::SessionIsFull:
                        ErrorMessage = TEXT("SessionIsFull");
                        break;
                    case EOnJoinSessionCompleteResult::SessionDoesNotExist:
                        ErrorMessage = TEXT("SessionDoesNotExist");
                        break;
                    case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress:
                        ErrorMessage = TEXT("CouldNotRetrieveAddress");
                        break;
                    case EOnJoinSessionCompleteResult::AlreadyInSession:
                        ErrorMessage = TEXT("AlreadyInSession");
                        break;
                    case EOnJoinSessionCompleteResult::UnknownError:
                        ErrorMessage = TEXT("UnknownError");
                        break;
                    case EOnJoinSessionCompleteResult::Success:
                    default:
                        break;
                    }

                    Result.OnResult(false, ErrorMessage);
                    Session->ClearOnJoinSessionCompleteDelegate_Handle(*CallbackHandle);
                    return;
                }

                // @note: Not all subsystems require this, but after we join the session, now connect to the game server.
                FString ConnectInfo;
                Session->GetResolvedConnectString(SessionName, ConnectInfo);
                GEngine->SetClientTravel(this->GetOuterWorld(), *ConnectInfo, TRAVEL_Absolute);

                // Return the results.
                Result.OnResult(true, TEXT(""));

                // Unregister this callback since we've handled the call we care about.
                Session->ClearOnJoinSessionCompleteDelegate_Handle(*CallbackHandle);
        }));

    // Join the session.
    if (!Session->JoinSession(this->LocalUserNum, SessionName, SessionSearchResult))
    {
        Result->OnResult(false, TEXT("JoinSession call failed to start."));
        Session->ClearOnJoinSessionCompleteDelegate_Handle(*CallbackHandle);
    }
    return Result;
}

sp<TNetResult<>> URpSessions::ExeJoinSessionWithParty(
    const FOnlineSessionSearchResult& SearchResult,
    FName SessionName)
{
    auto& OSS = GetIOnlineSubsytem();
    auto Session = GetSessionPtr(OSS);
    auto Result = MakeThreadPtr(TNetResult<>);
    GET(MmSessionSearchResults);
    GET(SessionSearchResult, MmSessionSearchResults.Find(SearchResult.GetSessionIdStr()));

    auto Identity = OSS.GetIdentityInterface();
    auto UserId = Identity->GetUniquePlayerId(LocalUserNum);

    // Register an event so we can receive the outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle = Session->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateWeakLambda(
        this,
        [this,
        Session,
        Identity,
        UserId,
        CallbackHandle,
        SessionName,
        ResultWk = TWeakPtr<TNetResult<>>(Result)
        ](FName CallbackSessionName, EOnJoinSessionCompleteResult::Type CallbackResult) {
            // Check if this callback is for us.
            if (!SessionName.IsEqual(CallbackSessionName))
            {
                // This callback isn't for our call.
                return;
            }


            GetWeakSafe(Result);

            // If we're not successful, return now.
            if (CallbackResult != EOnJoinSessionCompleteResult::Success)
            {
                FString ErrorMessage;
                switch (CallbackResult)
                {
                case EOnJoinSessionCompleteResult::SessionIsFull:
                    ErrorMessage = TEXT("SessionIsFull");
                    break;
                case EOnJoinSessionCompleteResult::SessionDoesNotExist:
                    ErrorMessage = TEXT("SessionDoesNotExist");
                    break;
                case EOnJoinSessionCompleteResult::CouldNotRetrieveAddress:
                    ErrorMessage = TEXT("CouldNotRetrieveAddress");
                    break;
                case EOnJoinSessionCompleteResult::AlreadyInSession:
                    ErrorMessage = TEXT("AlreadyInSession");
                    break;
                case EOnJoinSessionCompleteResult::UnknownError:
                    ErrorMessage = TEXT("UnknownError");
                    break;
                case EOnJoinSessionCompleteResult::Success:
                default:
                    break;
                }

                Result.OnResult(false, ErrorMessage);
                Session->ClearOnJoinSessionCompleteDelegate_Handle(*CallbackHandle);
                return;
            }

            // Get the online subsystem.
            auto CallbackOSS = Online::GetSubsystem(this->GetOuterWorld());
            if (CallbackOSS == nullptr)
            {
                Result.OnResult(false, TEXT("Online subsystem is not available."));
                return;
            }

            // Get the party interface, if the online subsystem supports it.
            auto PartySystem = CallbackOSS->GetPartyInterface();
            if (!PartySystem.IsValid())
            {
                Result.OnResult(false, TEXT("Online subsystem does not support parties."));
                return;
            }

            // Get the session ID.
            FString SessionId;
            auto* CurrentSession = Session->GetNamedSession(CallbackSessionName);
            if (CurrentSession != nullptr)
            {
                SessionId = CurrentSession->GetSessionIdStr();
            }

            // Find the first party that the user is also a leader of.
            TArray<TSharedRef<const FOnlinePartyId>> PartyIds;
            PartySystem->GetJoinedParties(*UserId, PartyIds);
            TSharedPtr<const FOnlinePartyId> SelectedPartyId;
            for (const auto& PartyId : PartyIds)
            {
                if (PartySystem->IsMemberLeader(*UserId, *PartyId, *UserId))
                {
                    SelectedPartyId = PartyId;
                    break;
                }
            }
            if (!SelectedPartyId.IsValid())
            {
                // Just join the game server - there's no party.
                FString ConnectInfo;
                Session->GetResolvedConnectString(SessionName, ConnectInfo);
                GEngine->SetClientTravel(this->GetOuterWorld(), *ConnectInfo, TRAVEL_Absolute);
                Result.OnResult(true, TEXT(""));
                Session->ClearOnJoinSessionCompleteDelegate_Handle(*CallbackHandle);
                return;
            }

            // Update the party data.
            auto PartyData =
                MakeShared<FOnlinePartyData>(*PartySystem->GetPartyData(*UserId, *SelectedPartyId, NAME_None));
            PartyData->SetAttribute(TEXT("JoinSessionIdFromParty"), FVariantData(SessionId));
            PartySystem->UpdatePartyData(*UserId, *SelectedPartyId, NAME_None, *PartyData);

            // @note: Not all subsystems require this, but after we join the session, now connect to the game server.
            FString ConnectInfo;
            Session->GetResolvedConnectString(SessionName, ConnectInfo);
            GEngine->SetClientTravel(this->GetOuterWorld(), *ConnectInfo, TRAVEL_Absolute);

            // Return the results.
            Result.OnResult(true, TEXT(""));

            // Unregister this callback since we've handled the call we care about.
            Session->ClearOnJoinSessionCompleteDelegate_Handle(*CallbackHandle);
        }));

    // Join the session.
    if (!Session->JoinSession(this->LocalUserNum, SessionName, SessionSearchResult))
    {
        Result->OnResult(false, TEXT("JoinSession call failed to start."));
        Session->ClearOnJoinSessionCompleteDelegate_Handle(*CallbackHandle);
    }
    return Result;
}

void URpSessions::ExeOpenInviteUI(FName SessionName)
{
    GET(OSS, Online::GetSubsystem(this->GetOuterWorld()));
    // Get the external UI interface, if the online subsystem supports it.
    auto ExternalUI = OSS.GetExternalUIInterface();
    if (!ExternalUI.IsValid())
        return;

    // Open the invite UI.
    ExternalUI->ShowInviteUI(this->LocalUserNum, SessionName);
}

void URpSessions::ExeReturnToMainMenu()
{
    GEngine->SetClientTravel(this->GetOuterWorld(), TEXT("/Game/MainMap"), TRAVEL_Absolute);
}
