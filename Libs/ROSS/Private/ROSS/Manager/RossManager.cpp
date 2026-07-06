#include "ROSS/Manager/RossManager.h"
#ifdef BxROSS
// -----------------------------------------------
// Libs
#include "ROSS/Ross.h"
#include "Menu/MainMenu.h"
#include "Access/General.h"
#include "ROSS/OSS/RpAuth.h"
#include "ROSS/OSS/RpSessions.h"
#include "ROSS/OSS/RpCurrentUser.h"
#include "ROSS/Util/NetResult.h"
// -----------------------------------------------
// Engine
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "EngineUtils.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerState.h"
#include "Delegates/DelegateSignatureImpl.inl"

// -----------------------------------------------
// Online
#include "Online/OnlineSessionNames.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystemTypes.h"
#include "Misc/ConfigCacheIni.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

// -----------------------------------------------
ARossManager::ARossManager() : ASessionManager()
{
    PrintStart();
    SessionName = SsSessionName;
}

ARossManager::~ARossManager()
{
    PrintStart();
    Print("Server shutting down not included (commented out)");
}

void ARossManager::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
}

void ARossManager::RegisterServer()
{
    PrintStart();
    Super::RegisterServer();
    if (GetWorld()->GetNetMode() != ENetMode::NM_DedicatedServer) {
        PrintW("Not a dedicated server - skipping server registration");
        return;
    }
    if (MeState != EState::None) {
        PrintW("Incorrect State: ", (int)MeState, " != ", (int)EState::None);
        return;
    }
    Print("Registering Server...");

    GET(LoWorld, GetWorld());
    if (LoWorld.GetNetMode() != NM_DedicatedServer) {
        PrintE("Not a Dedicated Server");
        return;
    }

    Print("Dedicated Server Starting!!");
    constexpr uint32 LnDelaySecs = 5; // give time for server to initialize
    ATracker::DelayAction(GetWorld(), this, "RegisterServer", LnDelaySecs);
}

void ARossManager::CreateSession()
{
    PrintStart();
    if (GetWorld()->GetNetMode() == ENetMode::NM_DedicatedServer)
    {
        SetServerArguments();
        ServerCreateSession();
    }
    else
        P2PCreateSession();
}

void ARossManager::ServerCreateSession()
{
    PrintStart();
    throw BBB("Not created yet");
}

void ARossManager::P2PCreateSession()
{
    PrintStart();
    ASessionManager::P2PCreateSession();
}

void ARossManager::OnCreateSessionComplete(FName FsSessionName, bool bWasSuccessful)
{
    PrintStart();
    Super::OnCreateSessionComplete(FsSessionName, bWasSuccessful);
}

void ARossManager::StartSession()
{
    PrintStart();
    if (GetWorld()->GetNetMode() == ENetMode::NM_DedicatedServer)
        ServerStartSession();
    else
        P2PStartSession();
}

void ARossManager::ServerStartSession()
{
    PrintStart();
    ASessionManager::ServerStartSession();
}

void ARossManager::P2PStartSession()
{
    PrintStart();
    ASessionManager::P2PStartSession();
}

void ARossManager::OnStartSessionComplete(FName FsSessionName, bool bWasSuccessful)
{
    PrintStart();
    Super::OnStartSessionComplete(FsSessionName, bWasSuccessful);
}

void ARossManager::ServerTravelListen(const FString& FsMapPath, const FString& FsModePath)
{
    PrintStart()
    ASessionManager::ServerTravelListen(FsMapPath, FsModePath);
}

void ARossManager::ServerTravelJoin(const FString& FsMapPath, const FString& FsModePath)
{
    PrintStart()
    ASessionManager::ServerTravelJoin(FsMapPath, FsModePath);
}

void ARossManager::EndSession()
{
    PrintStart();
    if (GetWorld()->GetNetMode() == ENetMode::NM_DedicatedServer)
        ServerEndSession();
    else
        P2PEndSession();
}

void ARossManager::ServerEndSession()
{
    PrintW("Not Programed Yet");
    throw BBB("Not Programed Yet");
}

void ARossManager::P2PEndSession()
{
    ASessionManager::P2PEndSession();
}

void ARossManager::OnAutoLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& Error)
{
    PrintStart();
    Super::OnAutoLoginComplete(LocalUserNum, bWasSuccessful, Error);
}

void ARossManager::OnAutoLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
    PrintStart();
    // Super::OnAutoLoginComplete(LocalUserNum, bWasSuccessful, UserId, Error);
}

void ARossManager::RegisterPlayer(APlayerController* NewPlayerPtr, const FUniqueNetIdRepl& UniqueId, bool bWasFromInvite)
{
    PrintStart()
    ASessionManager::RegisterPlayer(NewPlayerPtr, UniqueId, bWasFromInvite);
}

void ARossManager::UnregisterPlayer(FName FsSessionName, const FUniqueNetIdRepl &UniqueId)
{
    PrintStart()
    ASessionManager::UnregisterPlayer(FsSessionName, UniqueId);
}

void ARossManager::UnregisterPlayer(const APlayerController* ExitingPlayerPtr)
{
    PrintStart()
    ASessionManager::UnregisterPlayer(ExitingPlayerPtr);
}

void ARossManager::UnregisterPlayers(FName FsSessionName, const TArray<FUniqueNetIdRepl>& Players)
{
    PrintStart()
    for (const FUniqueNetIdRepl& UniqueId : Players)
        UnregisterPlayer(FsSessionName, UniqueId);
}

void ARossManager::AddAdmin(APlayerController *AdminPlayerPtr)
{
    PrintStart();
    GET(AdminPlayer);
    GET(LoState, AdminPlayer.PlayerState);
    ASessionManager::AddAdmin(AdminPlayerPtr);
    throw BBB("ARossManager::AddAdmin not yet Programmed");
}

void ARossManager::RemoveAdmin(APlayerController *AdminPlayerPtr)
{
    PrintStart();
    GET(AdminPlayer);
    GET(LoState, AdminPlayer.PlayerState);
    ASessionManager::RemoveAdmin(AdminPlayerPtr);
    throw BBB("ARossManager::RemoveAdmin not yet Programmed");
}

bool ARossManager::BanPlayer(APlayerController *BannedPlayer, const FText &BanReason)
{
    PrintStart();
    /// NOT IMPLIMENTED YET
    PrintE("No Ban System Implimented Yet!");
    return false;
}

void ARossManager::Restart()
{
    PrintStart()
    GET(World, GetWorld());
    PrintW("\n\nMap: ", GetGameConfig().GetMapPath(), "\n GameMode: ", GetGameConfig().GetModePath(), "\n Port: ", GetGameConfig().GetGamePort(), "\n");
    throw BBB("Not Programed Yet");
}

void ARossManager::RegisterServerFailed()
{
    PrintStart();
    PrintW("Register Server Failed, re-trying");
    The.RegisterServer();
    ATracker::DelayAction(GetWorld(), this, "RegisterServerFailed");
}

bool ARossManager::GetSessionJoinability(FName FsSessionName, FJoinabilitySettings &OutSettings)
{
    PrintStart();
    PrintE("You need a SessionConfig object to track all the configurations, then return if it is joinable after checking max players");
    return false;
}

void ARossManager::UpdateSessionJoinability(FName FsSessionName, bool bPublicSearchable, bool bAllowInvites, bool bJoinViaPresence, bool bJoinViaPresenceFriendsOnly)
{
    PrintStart();
    PrintE("You need a SessionConfig object to track all the configurations, then return if it is joinable after checking max players");
}

void ARossManager::DumpSessionState()
{
    PrintStart();
    PrintE("You must create an object to store session stats, then add a stdout and write it to a file or DB");
    //if (UOnlineEngineInterfaceRedpointEOS* OnlineEngineInterface = UOnlineEngineInterfaceRedpointEOS::Get())
    //{
    //    OnlineEngineInterface->DumpSessionState(GetWorld());
    //}
    //else
    //{
    //    PrintW("Failed to get OnlineEngineInterface instance.");
    //}
}


void ARossManager::OnEndSessionComplete(FName FsSessionName, bool bWasSuccessful)
{
    PrintStart();
    PrintW("Todo: Add to Database Logs and for MTT, give Payouts");
}

void ARossManager::SearchSessions()
{
    PrintStart();
    GetROSS().GetSessions().SearchSessions();
}

//void ARossManager::OnRossServersConnected(RossServersConnected_t* pCallback)
//{
//    PrintStart();
//    static bool bLoggedOnce = false;
//    if (!bLoggedOnce)
//    {
//        bLoggedOnce = true;
//        ensure(SsSessionName == SessionName);
//        Print("Steam Game Server: Connected to Steam!");
//        Print("SteamID: ", FString::Printf(TEXT("%llu"), SteamGameServer()->GetSteamID().ConvertToUint64()));
//    }
//}
//
//void ARossManager::OnRossServerConnectFailure(RossServerConnectFailure_t* pCallback)
//{
//    PrintStart();
//    ensure(SsSessionName == SessionName);
//    PrintE("Ross Game Server: Connection failed!");
//    PrintE("Result: ", (int32)pCallback->m_eResult);
//    PrintE("Still retrying: ", pCallback->m_bStillRetrying);
//
//    // Optionally retry
//    if (!pCallback->m_bStillRetrying)
//        RegisterServerFailed();
//}
//
//void ARossManager::OnRossServersDisconnected(RossServersDisconnected_t* pCallback)
//{
//    PrintW("Ross Game Server: Disconnected!");
//    PrintW("Result: ", (int32)pCallback->m_eResult);
//}
//
//void ARossManager::OnGSPolicyResponse(GSPolicyResponse_t* pCallback)
//{
//    PrintStart();
//}

#endif