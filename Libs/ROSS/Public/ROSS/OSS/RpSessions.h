// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ROSS/RpConfig.h"
#include "ROSS/Util/NetResult.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionDelegates.h"
#include "ROSS/Util/RossSessionSettings.h"
#include "RpSessions.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class THEGAME_API URpSessions : public URpConfig
{
    GENERATED_BODY()
public:
    URpSessions();
    IOnlineSessionPtr GetSessionPtr(const IOnlineSubsystem& OSS) const;
    sp<TMap<FString, FOnlineSessionSearchResult>> MmSessionSearchResultsPtr;
    sp<TNetResult<FOnlineSessionSearch>> MoSessionResultsPtr;
protected:
    virtual void BeginPlay() override;
public:
    // -- Getters
    FString GetSessionId(FName SessionName) const;
    TArray<FUniqueNetIdRepl> GetSessionRegisteredPlayerIds(FName SessionName) const;
    TSharedPtr<TNetResult<FOnlineSessionSearch>> GetSessions(FVVDelegate&& FoDelegate, int32 FnMaxResults = 50);
    void SearchSessions(int32 FnMaxResults = 50);
    UFUNCTION() void OnSearchSessionsComplete();
    // -- Server Executions
    sp<TNetResult<>> ExeServerStartListenServer(int32 AvailableSlots);
    sp<TNetResult<void, FOnCreateSessionCompleteDelegate>> ExeServerCreateSession(FOnCreateSessionCompleteDelegate&& FoDelegate, const FRossSessionSettings& FoSessionSettings);
    sp<TNetResult<void, FOnStartSessionCompleteDelegate>>  ExeServerStartSession(FOnStartSessionCompleteDelegate&& FoDelegate, const FRossSessionSettings& FoSessionSettings);
    sp<TNetResult<>> ExeServerEndSession(FName SessionName);
    sp<TNetResult<>> ExeServerTravelToMapAndMode(const FOnlineSessionSettings& FoSettings);
    bool RefreshHostSessionPort(FName SessionName, UWorld* World = nullptr);
    sp<TNetResult<>> ExeServerDestroySession(FName SessionName);
    sp<TNetResult<>> ExeServerRegisterPlayer(FName SessionName, const FUniqueNetIdRepl& PlayerId, bool bWasFromInvite);
    sp<TNetResult<>> ExeServerUnregisterPlayer(FName SessionName, const FUniqueNetIdRepl& PlayerId);
    // -- Player Executions
    sp<TNetResult<>> ExeJoinSession(const FOnlineSessionSearchResult& SearchResult, FName SessionName);
    sp<TNetResult<>> ExeJoinSessionWithParty(const FOnlineSessionSearchResult& SearchResult, FName SessionName);

    void ExeOpenInviteUI(FName SessionName);
    void ExeReturnToMainMenu();
};
