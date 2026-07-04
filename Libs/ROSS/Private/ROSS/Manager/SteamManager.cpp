#include "ROSS/Manager/SteamManager.h"
#ifdef BxSteam
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

#ifdef BxSteam
    #include "OnlineSubsystemSteam.h"
    #include "steam/steam_gameserver.h"
#endif
// -----------------------------------------------
ASteamManager::ASteamManager()
{
    PrintStart();
    SessionName = SsSessionName;
}

ASteamManager::~ASteamManager()
{
    PrintStart();

    Print("Server shutting down not included (commented out)");
    RossPtr = nullptr;
    if (SteamGameServer())
    {
        SteamGameServer()->LogOff();
        SteamGameServer_Shutdown();
    }
}

void ASteamManager::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bSteamAuthenticated)
        SteamGameServer_RunCallbacks();

}

void ASteamManager::SetPort(uint32 FnPort)
{
    ASessionManager::SetPort(FnPort);
    GetSteamConfig().SetGamePort(FnPort);
}

void ASteamManager::RegisterServer()
{
    PrintStart();
    Super::RegisterServer();
    if (GetWorld()->GetNetMode() != ENetMode::NM_DedicatedServer) {
        PrintW("Not a dedicated server - skipping server registration");
        return;
    }
    if (SeState != EState::None) {
        PrintW("Incorrect State: ", (int)SeState, " != ", (int)EState::None);
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
#endif
void ASteamManager::RegisterSteamServer()
{
#ifdef BxSteam
    PrintStart();

    if (GetWorld()->GetNetMode() != ENetMode::NM_DedicatedServer) {
        PrintW("Not a dedicated server - skipping server registration");
        return;
    }

    if (SeState != EState::None) {
        PrintW("Incorrect State: ", (int)SeState, " != ", (int)EState::None);
        return;
    }
    Print("Registering Steam Server...");
    ensure(SsSessionName == SessionName);
    InitTracker(MoTrackSubsystemReady);

    if (!SteamGameServer())
    {
        PrintE("SteamGameServer() is NULL - cannot register server");
        return;
    }

    const bool bAlreadyLoggedOn = SteamGameServer()->BLoggedOn();
    const uint64 SteamID = SteamGameServer()->GetSteamID().ConvertToUint64();


    FParse::Value(FCommandLine::Get(), TEXT("GameServerQueryPort="), GetSteamConfig().MnQueryPort);
    Print(
        "Game Port:  ", GetSteamConfig().MnGamePort, " && "
        "Query Port: ", GetSteamConfig().MnQueryPort
    )

        Print("Before: SteamGameServer: Valid"
            , " | LoggedOn: ", (bAlreadyLoggedOn || bSteamAuthenticated)
            , " | SteamID: ", SteamID
        );
    if (!bAlreadyLoggedOn && !bSteamAuthenticated)
    {
        Print("First time setup - configuring Steam Game Server...");


        int32 GameServerGamePort = 0;
        int32 GameServerQueryPort = 0;
        FString PortString;

        //if (GConfig->GetString(TEXT("URL"), TEXT("Port"), PortString, GEngineIni))
        //    GameServerGamePort = FCString::Atoi(*PortString);
        //ensure(SoSteamConfig.MnGamePort == GameServerGamePort);

        //if (GConfig->GetString(TEXT("OnlineSubsystemSteam"), TEXT("GameServerQueryPort"), PortString, GEngineIni))
        //    GameServerQueryPort = FCString::Atoi(*PortString);
        //ensure(SoSteamConfig.MnQueryPort == GameServerQueryPort);

        ensure(SteamGameServer_Init(
            GetSteamConfig().unIP,        // 0 = bind to all interfaces (most common)
            GetSteamConfig().MnGamePort,  // Game Port
            GetSteamConfig().MnQueryPort, // Query Port
            EServerMode::eServerModeAuthenticationAndSecure,
            "1.0.0.0" // Version string
        ));
         
        // Set all properties BEFORE logging on
        SteamGameServer()->SetProduct("DeadDread");
        SteamGameServer()->SetGameDescription("Hord Game");
        SteamGameServer()->SetDedicatedServer(true);
        SteamGameServer()->SetServerName(XF::FStringToRValChars(SessionName));
        SteamGameServer()->SetMapName(XF::FStringToRValChars(GetGameConfig().GetMap()));
        SteamGameServer()->SetMaxPlayerCount(MaxPlayers);
        SteamGameServer()->SetPasswordProtected(false);
        SteamGameServer()->SetRegion("na"); // North America
        SteamGameServer()->SetGameTags("Default,PVE");
        SteamGameServer()->SetGameData(TCHAR_TO_UTF8(*FString::Printf(TEXT("Mode=%s;Map=%s"), 
            *GetGameConfig().GetMode(), 
            *GetGameConfig().GetMap())));
        SteamGameServer()->SetKeyValue("PORT", XF::FStringToRValChars(FString::FromInt(GetGameConfig().GetGamePort())));

        //Print("Waiting for Steam Game Server authentication callback...");
         SteamGameServer()->LogOnAnonymous();
    }
    else
    {
        Print("Already logged on - updating server properties");

        // Only update dynamic properties
        SteamGameServer()->SetMapName(XF::FStringToRValChars(GetGameConfig().GetMap()));
        SteamGameServer()->SetServerName(XF::FStringToRValChars(SessionName));
        SteamGameServer()->SetKeyValue("PORT", XF::FStringToRValChars(FString::FromInt(GetGameConfig().GetGamePort())));
        Print("Server updated and advertising");
    }
    ensure(SsSessionName == SessionName);
#endif
}

// Highly liklely the reaosn you have double auth completes is because the engine
// is already making you logon. So check the RpAuth and see if that is triggering this.
// Also, if it isn't then see if removing your steam init/logon fix these double connection issues
// when you fix the double connect, then maybe your sessions will start correctly.
// Remember, you never needed this much manual work when you did the GDTV version of Steam Sessions.
// The only difference between a dedicated server and a regular one is that one of them just doesn't have a player
// It shouldn't require huge amounts of custom work-arounds

void ASteamManager::OnSteamAuthenticationComplete()
{
#ifdef BxSteam
    PrintStart();
    if(bSteamAuthenticated){
        return;
    }
    SeState = EState::RegisterServer;
    ensure(SsSessionName == SessionName);

    bool LbSuccess = true;
    RossPtr = AROSS::GetRossPtr();

    GET(Ross);
    Ross.SetupPostLogin(); // 2nd time just in case
    auto LoWorld = GetWorld();
    if(Ross.BxReady() == false)
    {
        LbSuccess = false;
        Print("ROSS not ready");
    }
    if (!LoWorld)
    {
        LbSuccess = false;
        Print("GetWorld == NULL");
    }
    GET(LoSteamServer, SteamGameServer());
    if (!LoSteamServer.BLoggedOn())
    {
        LbSuccess = false;
        PrintW("Steam authentication failed - not logged on");
    }
    if (!LoSteamServer.GetSteamID().IsValid())
    {
        LbSuccess = false;
        PrintW("Steam authentication failed - invalid SteamID");
    }
    if (Online::GetSubsystem(GetWorld())->GetSubsystemName() != "STEAM") {
        LbSuccess = false;
        PrintW("Steam authentication failed - OnlineSubsystem is not STEAM");
    }

    if (LbSuccess && LoSteamServer.BLoggedOn())
    {
        Print("Steam authentication complete - ready to create session: ", SessionName);

        // Get the Steam Online Subsystem
        IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get(STEAM_SUBSYSTEM);

        GET(LoOnlineSubsystem, IOnlineSubsystem::Get(STEAM_SUBSYSTEM));
        GET(LoSteamSubsystem, static_cast<FOnlineSubsystemSteam*>(LoOnlineSubsystemPtr));

        bool bSuccess = LoSteamSubsystem.InitSteamworksServer();
        if (bSuccess)
        {
            UE_LOG(LogTemp, Log, TEXT("Steam server API initialized successfully."));
            Print("Waiting for Steam Game Server authentication callback...");
            //LoSteamServer.LogOnAnonymous();
            InitTracker(MoTrackSubsystemReady);
            GET(MoTrackSubsystemReady);
            MoTrackSubsystemReady.Slingshot(AROSS::GetWorldPtr(), "ASteamManager::CreateSession");
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to initialize Steam server API."));
            LbSuccess = false;
        }
    }
    else if(LoSteamServer.BLoggedOn())
    {
        PrintW("Steam authentication failed - cannot create session: ", SessionName);
        LbSuccess = false;
    }

    if (!LbSuccess)
    {
        Print("Steam authentication NOT yet successful");
        Print("Slingshot >> ASteamManager::OnSteamAuthenticationComplete");
        Print("Map Name: ", AROSS::GetWorldDrf().GetMapName());
        InitTracker(MoTrackSubsystemReady);
        GET(MoTrackSubsystemReady);
        MoTrackSubsystemReady.Slingshot(AROSS::GetWorldPtr(), "OnSteamAuthenticationComplete");
        return;
    }
#endif
}

#ifdef BxSteam
void ASteamManager::CreateSession()
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

void ASteamManager::ServerCreateSession()
{
    PrintStart();
    if (SeState != EState::RegisterServer) {
        PrintW("Incorrect State: ", (int)SeState, " != ", (int)EState::RegisterServer);
        return;
    }
    if (GetWorld()->GetNetMode() != ENetMode::NM_DedicatedServer) {
        PrintW("Not a dedicated server - skipping server registration");
        return;
    }
    ensure(SsSessionName == SessionName);

    auto LbReady = true;
    if (!GetWorld())
    {
        LbReady = false;
        Print("GetWorld() is NULL");
        
    }
    else if (!AROSS::InitializeReady(GetWorld()))
    {
        LbReady = false;
        PrintW("AROSS not ready");
    }
    if (!SteamGameServer()->BLoggedOn())
    {
        LbReady = false;
        Print("Steam Not ready yet, looping back for Session: ", SessionName);
        SteamGameServer()->LogOnAnonymous();
    }

    if (LbReady)
    {
        SetupRoss();
        RossPtr = AROSS::GetRossPtr();
    }
    else {
        Print("Slingshot");
        GET(MoTrackSubsystemReady);
        MoTrackSubsystemReady.Slingshot(GetWorld(), __CLASS__);
        return;
    }

    RossPtr = AROSS::GetRossPtr();

    GET(Ross);
    ensure(SsSessionName == SessionName);
     auto LoResultPtr = Ross.GetSessions().ExeServerCreateSession(
        FOnCreateSessionCompleteDelegate(),
        SoSettings);

    SteamGameServer()->SetAdvertiseServerActive(true);
    bSteamAuthenticated = true;
}

void ASteamManager::P2PCreateSession()
{
    PrintStart();
    ASessionManager::P2PCreateSession();
}

void ASteamManager::OnCreateSessionComplete(FName FsSessionName, bool bWasSuccessful)
{
    PrintStart();
    Super::OnCreateSessionComplete(FsSessionName, bWasSuccessful);
}

void ASteamManager::StartSession()
{
    PrintStart();
    if (GetWorld()->GetNetMode() == ENetMode::NM_DedicatedServer)
        ServerStartSession();
    else
        P2PStartSession();
}

void ASteamManager::ServerStartSession()
{
    PrintStart();
    if(!LbServerLoggedOn = SteamGameServer()->BLoggedOn())
        throw BBB("Server Not Logged On");

    ASessionManager::ServerStartSession();
}

void ASteamManager::P2PStartSession()
{
    PrintStart();
    ASessionManager::P2PStartSession();
}

void ASteamManager::OnStartSessionComplete(FName FsSessionName, bool bWasSuccessful)
{
    PrintStart();
    Super::OnStartSessionComplete(FsSessionName, bWasSuccessful);
}

void ASteamManager::ServerTravelListen(const FString& FsMapPath, const FString& FsModePath)
{
    PrintStart()
    ASessionManager::ServerTravelListen(FsMapPath, FsModePath);
}

void ASteamManager::ServerTravelJoin(const FString& FsMapPath, const FString& FsModePath)
{
    PrintStart()
    ASessionManager::ServerTravelJoin(FsMapPath, FsModePath);
}

void ASteamManager::EndSession()
{
    PrintStart();
    if (GetWorld()->GetNetMode() == ENetMode::NM_DedicatedServer)
        ServerEndSession();
    else
        P2PEndSession();
}

void ASteamManager::ServerEndSession()
{
    PrintW("Not Programed Yet");
    throw BBB("Not Programed Yet");
}

void ASteamManager::P2PEndSession()
{
    ASessionManager::P2PEndSession();
}

void ASteamManager::OnAutoLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& Error)
{
    PrintStart();
    Super::OnAutoLoginComplete(LocalUserNum, bWasSuccessful, Error);
}

void ASteamManager::OnAutoLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
    PrintStart();
    Super::OnAutoLoginComplete(LocalUserNum, bWasSuccessful, UserId, Error);
}

void ASteamManager::RegisterPlayer(APlayerController* NewPlayerPtr, const FUniqueNetIdRepl& UniqueId, bool bWasFromInvite)
{
    PrintStart()
    ASessionManager::RegisterPlayer(NewPlayerPtr, UniqueID, bWasFromInvite);
}

void ASteamManager::UnregisterPlayer(FName FsSessionName, const FUniqueNetIdRepl &UniqueId)
{
    PrintStart()
    ASessionManager::UnregisterPlayer(FsSessionName, UniqueId);
}

void ASteamManager::UnregisterPlayer(const APlayerController* ExitingPlayerPtr)
{
    PrintStart()
    ASteamManager::UnregisterPlayer(ExitingPlayerPtr);
}

void ASteamManager::UnregisterPlayers(FName FsSessionName, const TArray<FUniqueNetIdRepl>& Players)
{
    PrintStart()
    for (const FUniqueNetIdRepl& UniqueId : Players)
        UnregisterPlayer(FsSessionName, UniqueId);
}

void ASteamManager::AddAdmin(APlayerController *AdminPlayerPtr)
{
    PrintStart();
    GET(AdminPlayer);
    GET(LoState, AdminPlayer.PlayerState);
    ASessionManager::AddAdmin(AdminPlayerPtr);
    throw BBB("ASteamManager::AddAdmin not yet Programmed");
}

void ASteamManager::RemoveAdmin(APlayerController *AdminPlayerPtr)
{
    PrintStart();
    GET(AdminPlayer);
    GET(LoState, AdminPlayer.PlayerState);
    ASessionManager::RemoveAdmin(AdminPlayerPtr);
    throw BBB("ASteamManager::RemoveAdmin not yet Programmed");
}

bool ASteamManager::BanPlayer(APlayerController *BannedPlayer, const FText &BanReason)
{
    PrintStart();
    /// NOT IMPLIMENTED YET
    PrintE("No Ban System Implimented Yet!");
    return false;
}

void ASteamManager::Restart()
{
    PrintStart()
    GET(World, GetWorld());
    PrintW("\n\nMap: ", GetGameConfig().GetMapPath(), "\n GameMode: ", GetGameConfig().GetModePath(), "\n Port: ", GetGameConfig().GetGamePort(), "\n");
    throw BBB("Not Programed Yet");
}

int32 ASteamManager::GetSteamAppID() const
{
    PrintStart();
    int32 LnSteamAppID = -1;

    if (!GConfig->GetInt(
        TEXT("OnlineSubsystemSteam"),
        TEXT("SteamAppId"),
        LnSteamAppID,
        GEngineIni))
    ensure(LnSteamAppID != -1);
    Print("Registering Steam App ID: ", LnSteamAppID);
    return LnSteamAppID;
}

void ASteamManager::RegisterServerFailed()
{
    PrintStart();
    PrintW("Register Server Failed, re-trying");
    The.RegisterServer();
    ATracker::DelayAction(GetWorld(), this, "RegisterServerFailed");
}

bool ASteamManager::GetSessionJoinability(FName FsSessionName, FJoinabilitySettings &OutSettings)
{
    PrintStart();
    PrintE("You need a SessionConfig object to track all the configurations, then return if it is joinable after checking max players");
    return false;
}

void ASteamManager::UpdateSessionJoinability(FName FsSessionName, bool bPublicSearchable, bool bAllowInvites, bool bJoinViaPresence, bool bJoinViaPresenceFriendsOnly)
{
    PrintStart();
    PrintE("You need a SessionConfig object to track all the configurations, then return if it is joinable after checking max players");
}

void ASteamManager::DumpSessionState()
{
    PrintStart();
    PrintE("You must create an object to store session stats, then add a stdout and write it to a file or DB");
    // #include "OnlineEngineInterfaceRedpointEOS.h"
    //if (UOnlineEngineInterfaceRedpointEOS* OnlineEngineInterface = UOnlineEngineInterfaceRedpointEOS::Get())
    //{
    //    OnlineEngineInterface->DumpSessionState(GetWorld());
    //}
    //else
    //{
    //    PrintW("Failed to get OnlineEngineInterface instance.");
    //}
}


void ASteamManager::OnEndSessionComplete(FName FsSessionName, bool bWasSuccessful)
{
    PrintStart();
    Super::OnEndSessionComplete(FsSessionName, bWasSuccessful);
}


void ASteamManager::SearchSessions()
{
    PrintStart();
    GET(Ross);
    Ross.GetSessions().SearchSessions();
}

void ASteamManager::OnSteamServersConnected(SteamServersConnected_t* pCallback)
{
    PrintStart();
    static bool bLoggedOnce = false;
    if (!bLoggedOnce)
    {
        bLoggedOnce = true;
        ensure(SsSessionName == SessionName);
        Print("Steam Game Server: Connected to Steam!");
        Print("SteamID: ", FString::Printf(TEXT("%llu"), SteamGameServer()->GetSteamID().ConvertToUint64()));
    }
}

void ASteamManager::OnSteamServerConnectFailure(SteamServerConnectFailure_t* pCallback)
{
    PrintStart();
    ensure(SsSessionName == SessionName);
    PrintE("Steam Game Server: Connection failed!");
    PrintE("Result: ", (int32)pCallback->m_eResult);
    PrintE("Still retrying: ", pCallback->m_bStillRetrying);
    
    // Optionally retry
    if (!pCallback->m_bStillRetrying)
        RegisterServerFailed();
}

void ASteamManager::OnSteamServersDisconnected(SteamServersDisconnected_t* pCallback)
{
    PrintW("Steam Game Server: Disconnected from Steam!");
    PrintW("Result: ", (int32)pCallback->m_eResult);
    bSteamAuthenticated = false;
}

void ASteamManager::OnGSPolicyResponse(GSPolicyResponse_t* pCallback)
{
    PrintStart();
    static bool bHandledOnce = false;
    if (bHandledOnce) return;
    bHandledOnce = true;

    ensure(SessionName != FName("None") && SsSessionName != FName("SessionName"));
    Print("Steam Game Server: GSPolicyResponse received");
    Print("Secure: ", pCallback->m_bSecure);
    
    if (SteamGameServer() && SteamGameServer()->BLoggedOn())
    {
        Print("Steam Game Server: Authentication complete!");
        Print("Steam Server ID: ", FString::Printf(TEXT("%llu"), SteamGameServer()->GetSteamID().ConvertToUint64()));
        Print("Steam App    ID: ", SteamGameServerUtils()->GetAppID())
        
        // Notify that Steam is ready for session creation
        OnSteamAuthenticationComplete();
    }
    else
    {
        PrintW("GSPolicyResponse received but BLoggedOn() still false");
    }
}

void ASteamManager::OnLobbyCreated(LobbyCreated_t* pCallback)
{
    PrintStart();
    if (pCallback->m_eResult == k_EResultOK)
    {
        Print("Lobby created successfully. Lobby ID: ", pCallback->m_ulSteamIDLobby);
        CSteamID lobbyId = pCallback->m_ulSteamIDLobby;
        // Set lobby data
        SteamMatchmaking()->SetLobbyData(lobbyId, "SessionName", TCHAR_TO_UTF8(*SessionName.ToString()));
        SteamMatchmaking()->SetLobbyData(lobbyId, "MaxPlayers",  TCHAR_TO_UTF8(*FString::FromInt(MaxPlayers)));
        // Add more data as needed from SoSettings
        SeState = EState::CreateSession;
        // Proceed to start session or notify
    }
    else
    {
        PrintW("Failed to create lobby. Result: ", (int32)pCallback->m_eResult);
    }
}
#endif
