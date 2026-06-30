#include "ROSS/SessionManager.h"

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
#include "steam/steam_gameserver.h"
#include "Misc/ConfigCacheIni.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemSteam.h"
#include "OnlineSubsystemUtils.h"
// -----------------------------------------------
ASessionManager::ASessionManager()
{
    PrintStart();
    SessionName = SsSessionName;
}

ASessionManager::~ASessionManager()
{
    PrintStart();

    Print("Steam shutting down not included (commented out)");
    RossPtr = nullptr;
    //if (SteamGameServer())
    //{
    //    SteamGameServer()->LogOff();
    //    SteamGameServer_Shutdown();
    //}
}

void ASessionManager::Tick(float DeltaSeconds)
{
        Super::Tick(DeltaSeconds);
        if (bSteamAuthenticated)
            SteamGameServer_RunCallbacks();
}

void ASessionManager::SetServerArguments()
{
    PrintStart();
    if (GetWorld()->GetNetMode() != ENetMode::NM_DedicatedServer){
        throw BBB("needs to be a dedicated server")
    }
    bUsingDedicatedServer = true;

    FString MaxPlayersStr;
    MaxPlayers = FParse::Value(FCommandLine::Get(), TEXT("MaxPlayers="), MaxPlayersStr);
    int32 LnServerPort = 7777;  // defualt
    FParse::Value(FCommandLine::Get(), TEXT("-port="), LnServerPort);
    ensure(LnServerPort > 0);
    SetPort(LnServerPort);
    auto LsSessionName = FString(TEXT("GameSession_")) + FString::FromInt(LnServerPort);
    SetSessionName(LsSessionName);
    //SetSessionSettings(GetWorld()); // TODO: the map/mode should be added as arguments to SetServerArguments()
    ensure(SessionName == SsSessionName);
    ensure(MaxPlayers > 0);
}

void ASessionManager::SetupRoss()
{
    PrintStart();
    AROSS::SetWorld(GetWorld());
    AROSS::Setup();
    RossPtr = AROSS::GetRossPtr();
}

void ASessionManager::InitOptions(const FString& Options)
{
    AGameSession::InitOptions(Options);
    PrintStart();
    if(GetWorld()->GetNetMode() == ENetMode::NM_DedicatedServer)
    {
        if (SsSessionName == SsSessionNameUnset)
        {
            int32 LnServerPort = 7777;  // defualt
            FParse::Value(FCommandLine::Get(), TEXT("-port="), LnServerPort);
            ensure(LnServerPort > 0);
            SetPort(LnServerPort);
            auto LsSessionName = FString(TEXT("GameSession_")) + FString::FromInt(LnServerPort);
            SetSessionName(LsSessionName);
            // SetSessionSettings(GetWorld(), FoGameConfig); // TODO: needs game config
        }
        else {
            SessionName = SsSessionName;
        }

        Print("InitOptions: Now  SessionName: ", SsSessionName);
    }else
        SessionName = SsSessionName;
    RossPtr = AROSS::GetRossPtr();
    ensure(RossPtr != nullptr);
}

void ASessionManager::SetSessionSettings(UWorld* FoWorldPtr, const FGameConfig& FoGameConfig)
{
    PrintStart();
    SoSettings = FRossSessionSettings(FoGameConfig);

    if(!ensure(SessionName == SsSessionName)) // sanity to ensure it is still holding up
        Print("(SessionName: ", SessionName, ") != (SsSessionName: ", SsSessionName, ")");
    SetSessionName(SoSettings.MoGameConfig.GetSessionName());
    ensure(SessionName == SsSessionName);

    GET(IOSS, Online::GetSubsystem(FoWorldPtr));
    GET(ISession, IOSS.GetSessionInterface());
    if (ISession.GetNamedSession(GetSessionName()) != nullptr)
    {
        PrintW("Existing session with name ", GetSessionName(), " found. Destroying before recreate.");
        ISession.DestroySession(GetSessionName());
    }

    if (bUsingDedicatedServer) 
    {
        FString MaxPlayersStr;
        FParse::Value(FCommandLine::Get(), TEXT("MaxPlayers="), MaxPlayersStr);
        SoSettings.MoGameConfig.SetMaxPublicPlayers(XF::StringToInt(MaxPlayersStr));
        ensure(SoSettings.MoGameConfig.GetMaxPublicPlayers() > 0);
        // MaxPlayers = LnMaxPlayers; // static function

        SoSettings.NumPublicConnections = SoSettings.MoGameConfig.GetMaxPublicPlayers();
        SoSettings.NumPrivateConnections = SoSettings.MoGameConfig.GetMaxPrivatePlayers();
    }

    SoSettings.bUsesStats = false; // TODO: switch to true for database updates

    const bool bPersistentWorld = FParse::Param(FCommandLine::Get(), TEXT("PersistentWorld"));
    Print("Persistent World: ", bPersistentWorld);
    if (bPersistentWorld)
    {
        SoSettings.bAllowJoinInProgress = true;
        SoSettings.bAllowInvites = true;
        SoSettings.bShouldAdvertise = true;
    }
    else { // MTT, Sit-N-Go, Spins
        SoSettings.bAllowJoinInProgress = false; // Good prevent mid-match joins
        SoSettings.bAllowInvites = false; // Good keep control centralized
        SoSettings.bShouldAdvertise = true;  // Must be true for session to show up in searches (lobby mode)
    }

    if (bUsingDedicatedServer) 
    {
        SoSettings.bIsDedicated = true;
        SoSettings.bIsLANMatch = false;
        SoSettings.bUsesPresence = false; // False for Dedicated Servers b/c you can't show you are hosting if no player hosts
        SoSettings.bUseLobbiesIfAvailable = false; // Use server browser path, not lobbies, for dedicated servers
    }
    else
    {
        SoSettings.bIsDedicated = false;
        SoSettings.bIsLANMatch = true;
        SoSettings.bUsesPresence = true;
        SoSettings.bUseLobbiesIfAvailable = true;

    }

    SoSettings.bAllowJoinViaPresence = true; // so you can join friends
    SoSettings.bAllowJoinViaPresenceFriendsOnly = false; // so you can ONLY join friends
    SoSettings.bAntiCheatProtected = true;

    SoSettings.BuildUniqueId = FMath::RandRange(1, INT32_MAX); // 0 means non-searchable
    // TODO: ensure we don't accidentally get the same value 2x for the same server

    SoSettings.InitUniqueBuildID();
    if (bUsingDedicatedServer) {
        SoSettings.MoGameConfig.SetMapModeFromCLI();
    }
    else if(SoSettings.MoGameConfig.BxHasValues() == false) {
        PrintW("No MAP & MODE values set");
    }
}

void ASessionManager::SetSessionName(const FString& FsSessionName)
{
    SsSessionName = FName(*FsSessionName); 
    SessionName = SsSessionName;
    SoSettings.MsSessionName = FsSessionName;
    Print("SoSettings.MsSessionName = ", SoSettings.MsSessionName)
}

void ASessionManager::RegisterServer()
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
    ATracker::DelayAction(GetWorld(), this, "RegisterSteamServer", LnDelaySecs);
}

void ASessionManager::RegisterSteamServer()
{
    PrintStart();
#if USING_STEAM && PLATFORM_WINDOWS

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
#else
    PrintW("Steam Game Server not available (non-Steam build)");
#endif
    ensure(SsSessionName == SessionName);
}

// Highly liklely the reaosn you have double auth completes is because the engine
// is already making you logon. So check the RpAuth and see if that is triggering this.
// Also, if it isn't then see if removing your steam init/logon fix these double connection issues
// when you fix the double connect, then maybe your sessions will start correctly.
// Remember, you never needed this much manual work when you did the GDTV version of Steam Sessions.
// The only difference between a dedicated server and a regular one is that one of them just doesn't have a player
// It shouldn't require huge amounts of custom work-arounds

void ASessionManager::OnSteamAuthenticationComplete()
{
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
            MoTrackSubsystemReady.Slingshot(AROSS::GetWorldPtr(), "ASessionManager::CreateSession");
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
        Print("Slingshot >> ASessionManager::OnSteamAuthenticationComplete");
        Print("Map Name: ", AROSS::GetWorldDrf().GetMapName());
        InitTracker(MoTrackSubsystemReady);
        GET(MoTrackSubsystemReady);
        MoTrackSubsystemReady.Slingshot(AROSS::GetWorldPtr(), "OnSteamAuthenticationComplete");
        return;
    }
}

void ASessionManager::CreateSession()
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

void ASessionManager::ServerCreateSession()
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
        MoTrackSubsystemReady.Slingshot(GetWorld(), "CreateSession");
        return;
    }

    RossPtr = AROSS::GetRossPtr();

    GET(Ross);
    ensure(SsSessionName == SessionName);
    // auto LoResultPtr = Ross.GetSessions().ExeServerCreateSession(
    //    FOnCreateSessionCompleteDelegate(),
    //    SoSettings);


    SteamGameServer()->SetAdvertiseServerActive(true);
    bSteamAuthenticated = true;
}

void ASessionManager::P2PCreateSession()
{
    PrintStart();
    if (GetWorld()->GetNetMode() == ENetMode::NM_DedicatedServer) {
        PrintW("Should be a Client NOT a dedicated server in P2P Sessions");
        return;
    }
    if (SeState != EState::None) {
        PrintW("Incorrect State: ", (int)SeState, " != ", (int)EState::None);
        return;
    }
    ensure(SsSessionName == SessionName);

    RossPtr = AROSS::GetRossPtr();
    GET(Ross);
    auto LoResultsPtr = Ross.GetSessions().ExeServerCreateSession(
        FOnCreateSessionCompleteDelegate::CreateUObject(this, &ASessionManager::OnCreateSessionComplete),
        SoSettings);

    if (LoResultsPtr.IsValid()) {
        Print("P2P Session creation initiated: UMainMenu Local P2P Session")
            if (LoResultsPtr->BxSuccessful())
                Print("P2P Session created successfully: UMainMenu Local P2P Session")
            else
                PrintW("Failed to create P2P session: UMainMenu Local P2P Session")
    }
    else
        PrintW("Failed to initiate P2P session creation: UMainMenu Local P2P Session");
}

void ASessionManager::OnCreateSessionComplete(FName InSessionName, bool bWasSuccessful)
{
    PrintStart();
    ensure(SessionName == InSessionName);
    if (bWasSuccessful) {
        Print("Session ", InSessionName, " Created successfully.");
        SeState = EState::CreateSession;
        StartSession();
    }
    else {
        PrintW("Failed to create session ", InSessionName, ".");
    }
}

void ASessionManager::StartSession()
{
    PrintStart();
    if (GetWorld()->GetNetMode() == ENetMode::NM_DedicatedServer)
        ServerStartSession();
    else
        P2PStartSession();
}

void ASessionManager::ServerStartSession()
{
    PrintStart();
    if (GetWorld()->GetNetMode() != ENetMode::NM_DedicatedServer) {
        PrintW("Not a dedicated server - skipping server registration");
        return;
    }
    if (SeState != EState::CreateSession) {
        PrintW("Incorrect State: ", (int)SeState, " != ", (int)EState::CreateSession);
        return;
    }
    ensure(SsSessionName == SessionName);

    if (GetWorld()
        && AROSS::InitializeReady(GetWorld())
        && SteamGameServer()->BLoggedOn())
    {
        SetupRoss();
    }
    else {
        PrintW("Not ready for Start Session");
        Print("Slingshot");
        GET(MoTrackSubsystemReady);
        MoTrackSubsystemReady.Slingshot(GetWorld(), "StartSession");
        return;
    }
    RossPtr = AROSS::GetRossPtr();
    GET(Ross);

    auto LoResultPtr = Ross.GetSessions().ExeServerStartSession(
        FOnStartSessionCompleteDelegate::CreateUObject(this, &ASessionManager::OnStartSessionComplete),
        SoSettings);
}

void ASessionManager::P2PStartSession()
{
    if (GetWorld()->GetNetMode() == ENetMode::NM_DedicatedServer) {
        PrintW("P2P, not a dedicated server");
        return;
    }
    if (SeState != EState::CreateSession) {
        PrintW("Incorrect State: ", (int)SeState, " != ", (int)EState::CreateSession);
        return;
    }

    RossPtr = AROSS::GetRossPtr();
    GET(Ross);

    auto LoResultPtr = Ross.GetSessions().ExeServerStartSession(
        FOnStartSessionCompleteDelegate::CreateUObject(this, &ASessionManager::OnStartSessionComplete),
        SoSettings);


    if (LoResultPtr.IsValid()) {
        Print("P2P Session start initiated: UMainMenu Local P2P Session")
            if (LoResultPtr->BxSuccessful())
                Print("P2P Session started successfully: UMainMenu Local P2P Session")
            else
                PrintW("Failed to start P2P session: UMainMenu Local P2P Session")
    }
    else
        PrintW("Failed to initiate P2P session start: UMainMenu Local P2P Session")
}

void ASessionManager::OnStartSessionComplete(FName InSessionName, bool bWasSuccessful)
{
    PrintStart();
    Super::OnStartSessionComplete(InSessionName, bWasSuccessful);
    ensure(SessionName == InSessionName);
    if (bWasSuccessful) {
        Print("Session ", InSessionName, " started successfully.");
        SeState = EState::StartSession;
        ServerTravelListen();
    }
    else {
        PrintW("Failed to start session ", InSessionName, ".");
    }
}

void ASessionManager::ServerTravelListen(const FString& FsMapPath, const FString& FsModePath)
{
    PrintStart()
    if (SeState != EState::StartSession) {
        PrintW("Incorrect State: ", (int)SeState, " != ", (int)EState::StartSession);
        return;
    }
    ensure(SsSessionName == SessionName);

    GET(Ross);
    Ross.GetSessions().ExeServerTravelToMapAndMode(ASessionManager::SoSettings);
    SeState = EState::ServerTravelListen;
}

void ASessionManager::ServerTravelJoin(const FString& FsMapPath, const FString& FsModePath)
{
    PrintStart()
    if (SeState != EState::StartSession) {
        return;
    }
    SeState = EState::ServerTravelListen;
    ensure(SsSessionName == SessionName);

    if (AGameModeBase* GM = GetWorld()->GetAuthGameMode())
        GM->bUseSeamlessTravel = true;
    else {
        PrintW("No Seamless Travel");
    }

    if (FsMapPath.IsEmpty() == false)
        GetGameConfig().SetMapPath(FsMapPath);
    if (FsModePath.IsEmpty() == false)
        GetGameConfig().SetModePath(FsModePath);

    GET(LoWorld, GetWorld());
    GET(Ross);
    auto LsPort = FString::FromInt(GetGameConfig().MnGamePort);
    auto LsTravel = FString::Printf(TEXT("%s?game=%s"), *GetGameConfig().GetMapPath(), *GetGameConfig().GetModePath());
    if (LsTravel == SsTravel)
        return;
    SsTravel = LsTravel;
    Print("Client Travel...\n Map: ", GetGameConfig().GetMapPath(), "\n GameMode: ", GetGameConfig().GetModePath(), "\n Port: ", GetGameConfig().GetGamePort(), "\n");
    Print("SsTravel = ", SsTravel);

    PrintW("Client Travel not yet implemented");
    //LoWorld.ClientTravel(SsTravel);
}

void ASessionManager::EndSession()
{
    PrintStart();
    if (GetWorld()->GetNetMode() == ENetMode::NM_DedicatedServer)
        ServerEndSession();
    else
        P2PEndSession();
}

void ASessionManager::ServerEndSession()
{
    PrintW("Not Programed Yet");
    throw BBB("Not Programed Yet");
}

void ASessionManager::P2PEndSession()
{
    GET(Ross);
    GET(LoWorld, GetWorld());
    GET(LoPC, LoWorld.GetFirstPlayerController());

    if (LoPC.HasAuthority() && IOnlineSubsystem::Get())
    {
        auto SessionInterface = IOnlineSubsystem::Get()->GetSessionInterface();
        if (SessionInterface.IsValid()) {
            SessionInterface->DestroySession(*ASessionManager::GetSettings().MsSessionName);
            SeState = EState::None;
        }
    }
}

void ASessionManager::OnAutoLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& Error)
{
    PrintStart();
    if (bWasSuccessful) {
        Print("AutoLogin successful for LocalUserNum: ", LocalUserNum);
    }
    else {
        PrintW("AutoLogin failed for LocalUserNum: ", LocalUserNum, ", Error: ", Error);
    }
}

void ASessionManager::OnAutoLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
    PrintStart();
    if (bWasSuccessful) {
        Print("AutoLogin successful for LocalUserNum: ", LocalUserNum);
    }
    else {
        PrintW("AutoLogin failed for LocalUserNum: ", LocalUserNum, ", Error: ", Error);
    }
}

void ASessionManager::RegisterPlayer(APlayerController* NewPlayerPtr, const FUniqueNetIdRepl& UniqueId, bool bWasFromInvite)
{
    PrintStart()
    GET(OnlineSub, Online::GetSubsystem(GetWorld()));
    GET(SessionInterface, OnlineSub.GetSessionInterface());

    if (!UniqueId.IsValid())
    {
        PrintW("Unable to Register Player: ", UniqueId);
        return;
    }
    SvSessionPlayers.Add(UniqueId);
    SessionInterface.RegisterPlayer(SsSessionName, *UniqueId, bWasFromInvite);
}

bool ASessionManager::AtCapacity(bool bSpectator)
{
    PrintStart();
    if (bSpectator)
        return GetNumSpectators() >= MaxSpectators;
    else
        return GetNumPlayers() >= MaxPlayers;
}

void ASessionManager::UnregisterPlayer(FName InSessionName, const FUniqueNetIdRepl &UniqueId)
{
    PrintStart()
    try
    {
        GET(OnlineSub, Online::GetSubsystem(GetWorld()));
        GET(SessionInterface, OnlineSub.GetSessionInterface());

        if (!UniqueId.IsValid())
        {
            PrintW("UnregisterPlayer: UniqueId is invalid");
            return;
        }

        SessionInterface.UnregisterPlayer(InSessionName, *UniqueId);
    }
    catch(...){
        PrintW("UnregisterPlayer: Exception occurred while unregistering player with UniqueId: ", UniqueId);
    }
}

void ASessionManager::UnregisterPlayers(FName InSessionName, const TArray<FUniqueNetIdRepl>& Players)
{
    PrintStart()
    for (const FUniqueNetIdRepl& UniqueId : Players)
        UnregisterPlayer(InSessionName, UniqueId);
}

void ASessionManager::UnregisterPlayer(const APlayerController* ExitingPlayerPtr)
{
    PrintStart()
    try
    {
        GET(ExitingPlayer);
        GET(PlayerState, ExitingPlayer.PlayerState);
        const FUniqueNetIdRepl& UniqueId = PlayerState.GetUniqueId();
        if (!UniqueId.IsValid())
        {
            PrintW("UnregisterPlayer: PlayerState has invalid UniqueId");
            return;
        }
        GET(OnlineSub, Online::GetSubsystem(GetWorld()));
        GET(SessionInterface, OnlineSub.GetSessionInterface());
        SessionInterface.UnregisterPlayer(SessionName, *UniqueId);
    }
    catch (...){
        PrintW("UnregisterPlayer: Exception occurred while unregistering player");
    }
}

void ASessionManager::AddAdmin(APlayerController *AdminPlayerPtr)
{
    PrintStart();
    GET(AdminPlayer);
    GET(LoState, AdminPlayer.PlayerState);
    auto& UniqueId = LoState.GetUniqueId();
    if (!UniqueId.IsValid())
    {
        PrintW("AddAdmin: PlayerState has invalid UniqueId");
        return;
    }
    if (SmAdmins.Contains(UniqueId))
    {
        PrintW("AddAdmin: Player is already an admin: ", UniqueId);
        return;
    }
    SmAdmins.Add(UniqueId, AdminPlayerPtr);
}

void ASessionManager::RemoveAdmin(APlayerController *AdminPlayerPtr)
{
    PrintStart();
    GET(AdminPlayer);
    GET(LoState, AdminPlayer.PlayerState);
    auto& UniqueId = LoState.GetUniqueId();
    if (!UniqueId.IsValid())
    {
        PrintW("AddAdmin: PlayerState has invalid UniqueId");
        return;
    }
    if (SmAdmins.Contains(UniqueId))
    {
        SmAdmins.Remove(UniqueId);
        return;
    }
    PrintW("AddAdmin: Player is not an admin: ", UniqueId);
}

bool ASessionManager::BanPlayer(APlayerController *BannedPlayer, const FText &BanReason)
{
    PrintStart();
    /// NOT IMPLIMENTED YET
    PrintE("No Ban System Implimented Yet!");
    return false;
}

void ASessionManager::Restart()
{
    PrintStart()
    GET(World, GetWorld());
    PrintW("\n\nMap: ", GetGameConfig().GetMapPath(), "\n GameMode: ", GetGameConfig().GetModePath(), "\n Port: ", GetGameConfig().GetGamePort(), "\n");
    throw BBB("Not Programed Yet");
}

int32 ASessionManager::GetSteamAppID() const
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

void ASessionManager::RegisterServerFailed()
{
    PrintStart();
    PrintW("Register Server Failed, re-trying");
    The.RegisterServer();
    ATracker::DelayAction(GetWorld(), this, "RegisterServerFailed");
}

bool ASessionManager::GetSessionJoinability(FName InSessionName, FJoinabilitySettings &OutSettings)
{
    PrintStart();
    PrintE("You need a SessionConfig object to track all the configurations, then return if it is joinable after checking max players");
    return false;
}

void ASessionManager::UpdateSessionJoinability(FName InSessionName, bool bPublicSearchable, bool bAllowInvites, bool bJoinViaPresence, bool bJoinViaPresenceFriendsOnly)
{
    PrintStart();
    PrintE("You need a SessionConfig object to track all the configurations, then return if it is joinable after checking max players");
}

void ASessionManager::DumpSessionState()
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


void ASessionManager::OnEndSessionComplete(FName InSessionName, bool bWasSuccessful)
{
    PrintStart();
    PrintW("Todo: Add to Database Logs and for MTT, give Payouts");
}


void ASessionManager::SearchSessions()
{
    PrintStart();
    GET(Ross);
    Ross.GetSessions().SearchSessions();
}


int32 ASessionManager::GetNumPlayers() const
{
    int32 Count = 0;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if (PC && PC->PlayerState && !PC->PlayerState->IsSpectator()){
            ++Count;
        }
    }
    return Count;
}

int32 ASessionManager::GetNumSpectators() const
{
    int32 Count = 0;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if (PC && PC->PlayerState && PC->PlayerState->IsSpectator()){
            ++Count;
        }
    }
    return Count;
}

void ASessionManager::OnSteamServersConnected(SteamServersConnected_t* pCallback)
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

void ASessionManager::OnSteamServerConnectFailure(SteamServerConnectFailure_t* pCallback)
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

void ASessionManager::OnSteamServersDisconnected(SteamServersDisconnected_t* pCallback)
{
    PrintW("Steam Game Server: Disconnected from Steam!");
    PrintW("Result: ", (int32)pCallback->m_eResult);
    bSteamAuthenticated = false;
}

void ASessionManager::OnGSPolicyResponse(GSPolicyResponse_t* pCallback)
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

void ASessionManager::OnLobbyCreated(LobbyCreated_t* pCallback)
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

// Static member definitions
bool ASessionManager::bSteamAuthenticated = false;
FName ASessionManager::SsSessionName = "Unset";
FRossSessionSettings ASessionManager::SoSettings;
TSet<FUniqueNetIdRepl> ASessionManager::SvSessionPlayers;
TMap<FUniqueNetIdRepl, APlayerController*> ASessionManager::SmAdmins;
FString ASessionManager::SsTravel;

