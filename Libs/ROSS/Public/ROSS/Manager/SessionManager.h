#pragma once

#include "CoreMinimal.h"
#include "Access/General.h"
#include "ROSS/Util/RossSessionSettings.h"
#include "ROSS/Util/GameConfig.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/Actor.h"
#include "OnlineSessionSettings.h"
#include "ROSS/ROSS.h"
#include "ROSS/OSS/RpAuth.h"
#include "ROSS/Util/NetResult.h"
#include "Interfaces/OnlineSessionDelegates.h"
#include "SessionManager.generated.h"


/**
 *      Only Ever Called on the Server Game Mode
 *      Everything must be static b/c you get a new instance every time you load a new level
 *      You want to avoid double logins, losing values and the like by using statics
 */
UCLASS()
class THEGAME_API ASessionManager : public AGameSession
{
    GENERATED_BODY()
protected:
    enum class EState
    {
        None,
        RegisterServer,
        CreateSession,
        StartSession,
        ServerTravelListen,
    };
    INL static EState SeState = EState::None;

    UPROPERTY() ATracker* MoTrackSubsystemReadyPtr = nullptr;
    UPROPERTY() TWeakObjectPtr<AROSS> RossPtr = nullptr;

public:
    ASessionManager();
    virtual ~ASessionManager() override;
    virtual void Tick(float DeltaSeconds) override;

    void SetServerArguments();
    void SetupRoss();
    virtual void InitOptions(const FString& Options) override;
    void SetSessionSettings(UWorld* FoWorldPtr, const FGameConfig& FoGameConfig);
    static auto& GetGameConfig()  { return SoSettings.MoGameConfig; }
    void SetSessionName(const FString& FsSessionName);
    virtual void SetPort(uint32 FnPort);
    virtual void RegisterServer() override;
    UFUNCTION() virtual void CreateSession();
    UFUNCTION() virtual void ServerCreateSession();
    UFUNCTION() virtual void P2PCreateSession();
    UFUNCTION() virtual void OnCreateSessionComplete(FName InSessionName, bool bWasSuccessful);
    UFUNCTION() virtual void StartSession();
    UFUNCTION() virtual void ServerStartSession();
    UFUNCTION() virtual void P2PStartSession();
    UFUNCTION() virtual void OnStartSessionComplete(FName InSessionName, bool bWasSuccessful);
    UFUNCTION() virtual void ServerTravelListen(const FString& FsMapPath = "", const FString& FsModePath = "");
    UFUNCTION() virtual void ServerTravelJoin(const FString& FsMapPath = "", const FString& FsModePath = "");
    UFUNCTION() virtual void EndSession();
    UFUNCTION() virtual void ServerEndSession();
    UFUNCTION() virtual void P2PEndSession();

    inline static const auto& GetSessionName() { return SsSessionName; }
    inline static auto& GetSettings() { return SoSettings; }

    // FOnLoginComplete, int32 /*LocalUserNum*/, bool /*bWasSuccessful*/, const FUniqueNetId& /*UserId*/, const FString& /*Error*/);
    UFUNCTION() virtual void OnAutoLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& Error) override;
    // UFUNCTION() virtual void OnAutoLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);
    UFUNCTION() virtual void RegisterPlayer(APlayerController* NewPlayerPtr, const FUniqueNetIdRepl& UniqueId, bool bWasFromInvite) override;
    UFUNCTION() virtual bool AtCapacity(bool bSpectator) override;
    virtual void UnregisterPlayer(FName FsSessionName, const FUniqueNetIdRepl& UniqueId) override;
    virtual void UnregisterPlayer(const APlayerController* ExitingPlayerPtr) override;
    UFUNCTION() virtual void UnregisterPlayers(FName FsSessionName, const TArray<FUniqueNetIdRepl>& Players) override;
    UFUNCTION() virtual void AddAdmin(APlayerController* AdminPlayerPtr) override;
    UFUNCTION() virtual void RemoveAdmin(APlayerController* AdminPlayerPtr) override;
    UFUNCTION() virtual bool BanPlayer(APlayerController* BannedPlayerPtr, const FText& BanReason) override; /// TODO
    UFUNCTION() virtual void Restart() override;
    UFUNCTION() virtual void RegisterServerFailed() override;
    UFUNCTION() virtual bool GetSessionJoinability(FName FsSessionName, FJoinabilitySettings& OutSettings) override;
    UFUNCTION() virtual void UpdateSessionJoinability(
        FName FsSessionName,
        bool  bPublicSearchable,
        bool  bAllowInvites,
        bool  bJoinViaPresence,
        bool  bJoinViaPresenceFriendsOnly) override;
    UFUNCTION() virtual void DumpSessionState() override;
    UFUNCTION() virtual void OnEndSessionComplete(FName FsSessionName, bool bWasSuccessful) override;


    UFUNCTION() virtual void SearchSessions();
    static FRossSessionSettings SoSettings;
protected:
    int32 GetNumPlayers() const;
    int32 GetNumSpectators() const;

    static FName SsSessionName; // Static over AGameSession::SessionName instance cases (Set in StartSession)
    inline static const FName SsSessionNameUnset = FName(TEXT("Unset"));
    static TSet<FUniqueNetIdRepl> SvSessionPlayers;
    static TMap<FUniqueNetIdRepl, APlayerController*> SmAdmins;
    static FString SsTravel;
    inline static bool bUsingDedicatedServer = false;
};