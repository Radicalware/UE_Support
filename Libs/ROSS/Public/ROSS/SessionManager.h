#pragma once

#include "CoreMinimal.h"
#include "Access/General.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/Actor.h"
#include "OnlineSessionSettings.h"
#include "steam/steam_gameserver.h"
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

    enum class EState
    {
        None,
        RegisterServer,
        CreateSession,
        StartSession,
        StartLevel,
    };
    INL static EState SeState = EState::None;

    static bool bSteamAuthenticated;
    UPROPERTY() ATracker* MoTrackSubsystemReadyPtr = nullptr;
#ifdef USING_STEAM
    STEAM_GAMESERVER_CALLBACK(ASessionManager, OnSteamServersConnected, SteamServersConnected_t);
    STEAM_GAMESERVER_CALLBACK(ASessionManager, OnSteamServerConnectFailure, SteamServerConnectFailure_t);
    STEAM_GAMESERVER_CALLBACK(ASessionManager, OnSteamServersDisconnected, SteamServersDisconnected_t);
    STEAM_GAMESERVER_CALLBACK(ASessionManager, OnGSPolicyResponse, GSPolicyResponse_t);
    STEAM_CALLBACK(ASessionManager, OnLobbyCreated, LobbyCreated_t);
#endif
    struct FResults
    {
        sp<TNetResult<void, FOnLoginCompleteDelegate>>         MoLoginPtr;
        sp<TNetResult<void, FOnCreateSessionCompleteDelegate>> MoCreateSessionPtr;
        sp<TNetResult<void, FOnStartSessionCompleteDelegate>>  MoStartSessionPtr;

        INL auto& GetLogin()         { return *MoLoginPtr; }
        INL auto& GetCreateSession() { return *MoCreateSessionPtr; }
        INL auto& GetStartSession()  { return *MoStartSessionPtr; }
    };
    INL static FResults SoResults;

    UPROPERTY() TWeakObjectPtr<AROSS> RossPtr = nullptr;

public:
    ASessionManager();
    virtual ~ASessionManager() override;
    virtual void Tick(float DeltaSeconds) override;

    void SetServerArguments();
    void SetupRoss();
    virtual void InitOptions(const FString& Options) override;
    void SetSessionSettings();
    INL void SetSessionName(const FString& FsSessionName) { SsSessionName = FName(*FsSessionName); SessionName = SsSessionName; }
    INL void SetPort(uint32 FnPort) { SoSteamConfig.MnGamePort = FnPort; SoGameConfig.MnGamePort = FnPort; }
    virtual void RegisterServer() override;
    UFUNCTION() void RegisterSteamServer();
    UFUNCTION() void OnSteamAuthenticationComplete();
    UFUNCTION() void CreateSession();
    void OnCreateSessionComplete(FName InSessionName, bool bWasSuccessful);
    UFUNCTION() void StartSession();
    virtual void OnStartSessionComplete(FName InSessionName, bool bWasSuccessful) override;
    UFUNCTION() void StartLevel();

    inline const auto& GetSessionName() const { return SsSessionName; }
    inline const auto& GetSessionSettings() const { return MoSessionSettings; }

    // FOnLoginComplete, int32 /*LocalUserNum*/, bool /*bWasSuccessful*/, const FUniqueNetId& /*UserId*/, const FString& /*Error*/);
    virtual void OnAutoLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& Error) override;
            void OnAutoLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);
    virtual void RegisterPlayer(APlayerController* NewPlayerPtr, const FUniqueNetIdRepl& UniqueId, bool bWasFromInvite);
    virtual bool AtCapacity(bool bSpectator) override;
    virtual void UnregisterPlayer(FName InSessionName, const FUniqueNetIdRepl& UniqueId) override;
    virtual void UnregisterPlayers(FName InSessionName, const TArray<FUniqueNetIdRepl>& Players) override;
    virtual void UnregisterPlayer(const APlayerController* ExitingPlayerPtr) override;
    virtual void AddAdmin(APlayerController* AdminPlayerPtr) override;
    virtual void RemoveAdmin(APlayerController* AdminPlayerPtr) override;
    virtual bool BanPlayer(APlayerController* BannedPlayerPtr, const FText& BanReason) override; /// TODO
    virtual void Restart() override;
    int32 GetSteamAppID() const;
    FString GetMapName() const;
    virtual void RegisterServerFailed() override;
    virtual bool GetSessionJoinability(FName InSessionName, FJoinabilitySettings& OutSettings) override;
    virtual void UpdateSessionJoinability(
        FName InSessionName,
        bool  bPublicSearchable,
        bool  bAllowInvites,
        bool  bJoinViaPresence,
        bool  bJoinViaPresenceFriendsOnly) override;
    virtual void DumpSessionState() override;
    virtual void OnEndSessionComplete(FName InSessionName, bool bWasSuccessful) override;

protected:
    int32 GetNumPlayers() const;
    int32 GetNumSpectators() const;

    class BaseConfig
    {
    public:
        uint32 MnGamePort = 7777;
    };
    struct SteamConfig : public BaseConfig
    {
        uint32 unIP = 0;             // 0 = bind to all interfaces (most common)
        uint16 MnEphemeralPort = 0;  // 0 = Steam picks an ephemeral port (recommended)
        uint16 MnQueryPort = 27018;  // Source Query / A2S port for server browser
        //uint16 MnGamePort = 27017; // your game's main UDP port (clients connect here)
    };
    class GameConfig : public BaseConfig
    {
    public:
        INL GameConfig() { }

        void SetFromCLI();

        void SetMapPath(const FString& FsMapPath);
        void SetModePath(const FString& FsModePath);

        auto GetGamePort() const { return MnGamePort; }
        const auto& GetMap()  const { return MsMap; }
        const auto& GetMode() const { return MsMode; }
        const auto& GetMapPath()  const { return MsMapPath; }
        const auto& GetModePath() const { return MsModePath; }

    private:
        FString MsMapPath;
        FString MsModePath;

        FString MsMap;
        FString MsMode;
    };

    static SteamConfig SoSteamConfig;
    static GameConfig  SoGameConfig;

    static FName SsSessionName; // Static over AGameSession::SessionName instance cases (Set in StartSession)
           FOnlineSessionSettings MoSessionSettings; // (ok to be instance based (per level based))
    static TSet<FUniqueNetIdRepl> SvSessionPlayers;
    static TMap<FUniqueNetIdRepl, APlayerController*> SmAdmins;
    static FString SsTravel;
};