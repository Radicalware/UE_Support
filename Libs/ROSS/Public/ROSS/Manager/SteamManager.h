#pragma once
#include "ROSS/Manager/SessionManager.h"
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

#ifdef BxSteam
#include "OnlineSubsystemSteamModule.h"
#include "OnlineSubsystemSteam.h"
#include "steam/steam_gameserver.h"
#endif

#include "SteamManager.generated.h"


/**
 *      Only Ever Called on the Server Game Mode
 *      Everything must be static b/c you get a new instance every time you load a new level
 *      You want to avoid double logins, losing values and the like by using statics
 */
UCLASS()
class THEGAME_API ASteamManager : public ASessionManager
{
    GENERATED_BODY()
#ifdef BxSteam
    static bool bSteamAuthenticated;
    STEAM_GAMESERVER_CALLBACK(ASteamManager, OnSteamServersConnected, SteamServersConnected_t);
    STEAM_GAMESERVER_CALLBACK(ASteamManager, OnSteamServerConnectFailure, SteamServerConnectFailure_t);
    STEAM_GAMESERVER_CALLBACK(ASteamManager, OnSteamServersDisconnected, SteamServersDisconnected_t);
    STEAM_GAMESERVER_CALLBACK(ASteamManager, OnGSPolicyResponse, GSPolicyResponse_t);
public:
    ASteamManager();
    virtual ~ASteamManager() override;
    virtual void Tick(float DeltaSeconds) override;

    static auto& GetSteamConfig() { return SoSettings.MoSteamConfig; }
    virtual void SetPort(uint32 FnPort) override;
    virtual void RegisterServer() override;
#endif
    UFUNCTION() void RegisterSteamServer();
    UFUNCTION() void OnSteamAuthenticationComplete();
#ifdef BxSteam
    virtual void CreateSession() override;
    virtual void ServerCreateSession() override;
    virtual void P2PCreateSession() override;
    virtual void OnCreateSessionComplete(FName FsSessionName, bool bWasSuccessful) override;
    virtual void StartSession() override;
    virtual void ServerStartSession() override;
    virtual void P2PStartSession() override;
    virtual void OnStartSessionComplete(FName FsSessionName, bool bWasSuccessful) override;
    virtual void ServerTravelListen(const FString& FsMapPath = "", const FString& FsModePath = "") override;
    virtual void ServerTravelJoin(const FString& FsMapPath = "", const FString& FsModePath = "") override;

    virtual void EndSession() override;
    virtual void ServerEndSession() override;
    virtual void P2PEndSession() override;

    inline static const auto& GetSessionName() { return SsSessionName; }
    inline static auto& GetSettings() { return SoSettings; }

    // FOnLoginComplete, int32 /*LocalUserNum*/, bool /*bWasSuccessful*/, const FUniqueNetId& /*UserId*/, const FString& /*Error*/);
    virtual void OnAutoLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& Error) override;
            void OnAutoLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);
    virtual void RegisterPlayer(APlayerController* NewPlayerPtr, const FUniqueNetIdRepl& UniqueId, bool bWasFromInvite);
    virtual void UnregisterPlayer(FName FsSessionName, const FUniqueNetIdRepl& UniqueId) override;
    virtual void UnregisterPlayer(const APlayerController* ExitingPlayerPtr) override;
    virtual void UnregisterPlayers(FName FsSessionName, const TArray<FUniqueNetIdRepl>& Players) override;
    virtual void AddAdmin(APlayerController* AdminPlayerPtr) override;
    virtual void RemoveAdmin(APlayerController* AdminPlayerPtr) override;
    virtual bool BanPlayer(APlayerController* BannedPlayerPtr, const FText& BanReason) override; /// TODO
    virtual void Restart() override;
    int32 GetSteamAppID() const;
    virtual void RegisterServerFailed() override;
    virtual bool GetSessionJoinability(FName FsSessionName, FJoinabilitySettings& OutSettings) override;
    virtual void UpdateSessionJoinability(
        FName FsSessionName,
        bool  bPublicSearchable,
        bool  bAllowInvites,
        bool  bJoinViaPresence,
        bool  bJoinViaPresenceFriendsOnly) override;
    virtual void DumpSessionState() override;
    virtual void OnEndSessionComplete(FName FsSessionName, bool bWasSuccessful) override;

    virtual void SearchSessions() override;
#endif
};

