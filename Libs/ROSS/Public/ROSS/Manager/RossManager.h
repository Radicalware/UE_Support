#pragma once
#include "CoreMinimal.h"
#include "Access/General.h"
#include "ROSS/ROSS.h"
#include "ROSS/Manager/SessionManager.h"
#include "ROSS/OSS/RpAuth.h"
#include "ROSS/Util/RossSessionSettings.h"
#include "ROSS/Util/NetResult.h"
#include "ROSS/Util/GameConfig.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/Actor.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionDelegates.h"
#include "RossManager.generated.h"

/**
 *      Only Ever Called on the Server Game Mode
 *      Everything must be static b/c you get a new instance every time you load a new level
 *      You want to avoid double logins, losing values and the like by using statics
 */
UCLASS()
class THEGAME_API ARossManager : public ASessionManager
{
    GENERATED_BODY()

public:
    ARossManager();
    virtual ~ARossManager() override;
    virtual void Tick(float DeltaSeconds) override;

#ifdef BxROSS
    static bool bRossAuthenticated;
    //ROSS_GAMESERVER_CALLBACK(ARossManager, OnRossServersConnected, RossServersConnected_t);
    //ROSS_GAMESERVER_CALLBACK(ARossManager, OnRossServerConnectFailure, RossServerConnectFailure_t);
    //ROSS_GAMESERVER_CALLBACK(ARossManager, OnRossServersDisconnected, RossServersDisconnected_t);
    //ROSS_GAMESERVER_CALLBACK(ARossManager, OnGSPolicyResponse, GSPolicyResponse_t);

    static auto& GetGameConfig()  { return SoSettings.MoGameConfig; }
    virtual void RegisterServer() override;

    virtual void CreateSession();
    virtual void ServerCreateSession();
    virtual void P2PCreateSession();
    virtual void OnCreateSessionComplete(FName FsSessionName, bool bWasSuccessful);
    virtual void StartSession();
    virtual void ServerStartSession();
    virtual void P2PStartSession();
    virtual void OnStartSessionComplete(FName FsSessionName, bool bWasSuccessful);
    virtual void ServerTravelListen(const FString& FsMapPath = "", const FString& FsModePath = "");
    virtual void ServerTravelJoin(const FString& FsMapPath = "", const FString& FsModePath = "");

    virtual void EndSession();
    virtual void ServerEndSession();
    virtual void P2PEndSession();

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
