#include "ROSS/SessionManager.h"
#include "GameFramework/PlayerState.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemSteam.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "EngineUtils.h"
#include "steam/steam_gameserver.h"
#include "Misc/ConfigCacheIni.h"
#include "ROSS/ROSS.h"
#include "ROSS/Util/NetResult.h"
#include "ROSS/OSS/RpAuth.h"
#include "ROSS/OSS/RpSessions.h"
#include "Access/Tracker.h"
#include "GameFramework/GameModeBase.h"
#include "Misc/ConfigCacheIni.h"

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
        throw BBB("Not a Dedicated Server")
    }

    int32 LnServerPort = 7777;  // defualt
    FParse::Value(FCommandLine::Get(), TEXT("-port="), LnServerPort);
    SetPort(LnServerPort);
    auto LsSessionName = FString(TEXT("GameSession_")) + FString::FromInt(LnServerPort);
    SetSessionName(LsSessionName);
    SetSessionSettings();
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
        Print("InitOptions: From SessionName: ", SsSessionName);
        //SetServerArguments();

        int32 LnServerPort = 7777;  // defualt
        FParse::Value(FCommandLine::Get(), TEXT("-port="), LnServerPort);
        SetPort(LnServerPort);
        auto LsSessionName = FString(TEXT("GameSession_")) + FString::FromInt(LnServerPort);
        SetSessionName(LsSessionName);
        SetSessionSettings();

        Print("InitOptions: Now  SessionName: ", SsSessionName);
    }
}

void ASessionManager::SetSessionSettings()
{
    PrintStart();

    GET(IOSS, Online::GetSubsystem(GetWorld()));
    GET(ISession, IOSS.GetSessionInterface());
    if (ISession.GetNamedSession(GetSessionName()) != nullptr)
    {
        PrintW("Existing session with name ", GetSessionName(), " found. Destroying before recreate.");
        ISession.DestroySession(GetSessionName());
    }

    FString MaxPlayersStr;
    FParse::Value(FCommandLine::Get(), TEXT("MaxPlayers="), MaxPlayersStr);
    MaxPlayers = XF::StringToInt(MaxPlayersStr);
    ensure(MaxPlayers > 0);

    const bool bPersistentWorld = FParse::Param(FCommandLine::Get(), TEXT("PersistentWorld"));
    Print("Persistent World: ", bPersistentWorld);
    MoSessionSettings = FOnlineSessionSettings();
    MoSessionSettings.NumPublicConnections = MaxPlayers;
    MoSessionSettings.NumPrivateConnections = 0; // invite only

    MoSessionSettings.bUsesStats = false; // TODO: switch to true for database updates

    if (bPersistentWorld)
    {
        MoSessionSettings.bAllowJoinInProgress = true;
        MoSessionSettings.bAllowInvites = true;
        MoSessionSettings.bShouldAdvertise = true;
    }
    else { // MTT, Sit-N-Go, Spins
        MoSessionSettings.bAllowJoinInProgress = false; // Good prevent mid-match joins
        MoSessionSettings.bAllowInvites = false; // Good keep control centralized
        MoSessionSettings.bShouldAdvertise = true;  // Must be true for session to show up in searches (lobby mode)
    }

    MoSessionSettings.bIsDedicated = true;
    MoSessionSettings.bIsLANMatch = false;
    MoSessionSettings.bUsesPresence = false; // False for Dedicated Servers b/c you can't show you are hosting if no player hosts
    MoSessionSettings.bUseLobbiesIfAvailable = false; // Use server browser path, not lobbies, for dedicated servers

    MoSessionSettings.bAllowJoinViaPresence = true; // so you can join friends
    MoSessionSettings.bAllowJoinViaPresenceFriendsOnly = false; // so you can ONLY join friends
    MoSessionSettings.bAntiCheatProtected = true;

    MoSessionSettings.BuildUniqueId = FMath::RandRange(1, INT32_MAX); // 0 means non-searchable
    // TODO: ensure we don't accidentally get the same value 2x for the same server

    ensure(SessionName == SsSessionName);
    Print("Set SessionName: ", SessionName);

    SoGameConfig.SetFromCLI();

    MoSessionSettings.Set(TEXT("PORT"),     (int32)SoGameConfig.GetGamePort(), EOnlineDataAdvertisementType::ViaOnlineService);
    MoSessionSettings.Set(TEXT("MAP"),      SoGameConfig.GetMap(),      EOnlineDataAdvertisementType::ViaOnlineService);
    MoSessionSettings.Set(TEXT("GAMEMODE"), SoGameConfig.GetMode(),     EOnlineDataAdvertisementType::ViaOnlineService);
}

void ASessionManager::RegisterServer()
{
    PrintStart();
    if (GetWorld()->GetNetMode() != ENetMode::NM_DedicatedServer) {
        PrintW("Not a dedicated server - skipping server registration");
        return;
    }
    if (SeState != EState::None) {
        return;
    }
    Print("Registering Server...");
    ensure(SsSessionName == SessionName);
    InitTracker(MoTrackSubsystemReady);

    GET(LoWorld, GetWorld());

    if (LoWorld.GetNetMode() != NM_DedicatedServer) {
        PrintE("Not a Dedicated Server");
        return;
    }

    Print("Dedicated Server Starting!!");
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


    FParse::Value(FCommandLine::Get(), TEXT("GameServerQueryPort="), SoSteamConfig.MnQueryPort);
    Print(
        "Game Port:  ", SoSteamConfig.MnGamePort, " && "
        "Query Port: ", SoSteamConfig.MnQueryPort
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
            SoSteamConfig.unIP,        // 0 = bind to all interfaces (most common)
            SoSteamConfig.MnGamePort,  // Game Port
            SoSteamConfig.MnQueryPort, // Query Port
            EServerMode::eServerModeAuthenticationAndSecure,
            "1.0.0.0" // Version string
        ));
         
        // Set all properties BEFORE logging on
        SteamGameServer()->SetProduct("DeadDread");
        SteamGameServer()->SetGameDescription("Hord Game");
        SteamGameServer()->SetDedicatedServer(true);
        SteamGameServer()->SetServerName(XF::FStringToRValChars(SessionName));
        SteamGameServer()->SetMapName(XF::FStringToRValChars(GetMapName()));
        SteamGameServer()->SetMaxPlayerCount(MaxPlayers);
        SteamGameServer()->SetPasswordProtected(false);
        SteamGameServer()->SetRegion("na"); // North America
        SteamGameServer()->SetGameTags("Default,PVE");
        SteamGameServer()->SetGameData(TCHAR_TO_UTF8(*FString::Printf(TEXT("Mode=%s;Map=%s"), *SoGameConfig.GetMode(), *SoGameConfig.GetMap())));
        SteamGameServer()->SetKeyValue("PORT", XF::FStringToRValChars(FString::FromInt(SoSteamConfig.MnGamePort)));

        //Print("Waiting for Steam Game Server authentication callback...");
         SteamGameServer()->LogOnAnonymous();
    }
    else
    {
        Print("Already logged on - updating server properties");

        // Only update dynamic properties
        SteamGameServer()->SetMapName(XF::FStringToRValChars(GetMapName()));
        SteamGameServer()->SetServerName(XF::FStringToRValChars(SessionName));
        SteamGameServer()->SetKeyValue("PORT", XF::FStringToRValChars(FString::FromInt(SoSteamConfig.MnGamePort)));
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
    SeState = EState::RegisterServer;
    if(bSteamAuthenticated){
        return;
    }
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
    if (!SteamGameServer()->BLoggedOn())
    {
        LbSuccess = false;
        PrintW("Steam authentication failed - not logged on");
    }
    if (!SteamGameServer()->GetSteamID().IsValid())
    {
        LbSuccess = false;
        PrintW("Steam authentication failed - invalid SteamID");
    }
    if (Online::GetSubsystem(GetWorld())->GetSubsystemName() != "STEAM") {
        LbSuccess = false;
        PrintW("Steam authentication failed - OnlineSubsystem is not STEAM");
    }

    if (LbSuccess && SteamGameServer()->BLoggedOn())
    {
        Print("Steam authentication complete - ready to create session: ", SessionName);

        // Get the Steam Online Subsystem
        IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get(STEAM_SUBSYSTEM);
        if (OnlineSubsystem)
        {
            FOnlineSubsystemSteam* SteamSubsystem = static_cast<FOnlineSubsystemSteam*>(OnlineSubsystem);
            if (SteamSubsystem)
            {
                // Call InitSteamworksServer() to initialize the server-side Steam APIs
                bool bSuccess = SteamSubsystem->InitSteamworksServer();
                if (bSuccess)
                {
                    UE_LOG(LogTemp, Log, TEXT("Steam server API initialized successfully."));
                    Print("Waiting for Steam Game Server authentication callback...");
                    //SteamGameServer()->LogOnAnonymous();
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
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Steam subsystem is not available."));
                LbSuccess = false;
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Online subsystem (Steam) not found."));
            LbSuccess = false;
        }
    }
    else if(SteamGameServer()->BLoggedOn())
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
    if (SeState != EState::RegisterServer) {
        return;
    }

    GET(Ross);
    ensure(SsSessionName == SessionName);

    //auto LoDelegate = FOnCreateSessionCompleteDelegate();
    //LoDelegate.BindUObject(this, &ASessionManager::OnCreateSessionComplete);

    //SoResults.MoCreateSessionPtr = Ross.GetSessions().ExeServerCreateSession(
    //    LoDelegate,
    //    SessionName,
    //    MoSessionSettings);


    // Use Steam API to create session
    // ---------------------------------------------------------------------------------
    SteamGameServer()->SetAdvertiseServerActive(true);
    SeState = EState::StartSession; // Only because this is the last stage for steam
    Print("Slingshot");
    GET(MoTrackSubsystemReady);
    MoTrackSubsystemReady.Slingshot(GetWorld(), "StartLevel");
    bSteamAuthenticated = true;
    // ---------------------------------------------------------------------------------

    //if (SteamMatchmaking())
    //{
    //    ELobbyType lobbyType = MoSessionSettings.bShouldAdvertise ? k_ELobbyTypePublic : k_ELobbyTypePrivate;
    //    SteamAPICall_t call = SteamMatchmaking()->CreateLobby(lobbyType, MaxPlayers);
    //    if (call != k_uAPICallInvalid)
    //    {
    //        Print("Creating lobby...");
    //        // The callback will handle the result
    //    }
    //    else
    //    {
    //        PrintW("Failed to initiate lobby creation");
    //    }
    //}
    //else
    //{
    //    PrintW("SteamMatchmaking not available");
    //}
}

void ASessionManager::OnCreateSessionComplete(FName InSessionName, bool bWasSuccessful)
{
    PrintStart();
    ensure(SessionName == InSessionName);
    if (bWasSuccessful) {
        Print("Session ", InSessionName, " Created successfully.");
        SeState = EState::CreateSession;
        // StartSession();
    }
    else {
        PrintW("Failed to start session ", InSessionName, ".");
        // CreateSession();
    }
}

void ASessionManager::StartSession()
{
    PrintStart();
    if (GetWorld()->GetNetMode() != ENetMode::NM_DedicatedServer) {
        PrintW("Not a dedicated server - skipping server registration");
        return;
    }
    if (SeState != EState::CreateSession) {
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

    auto LoDelegate = FOnStartSessionCompleteDelegate();
    LoDelegate.BindUObject(this, &ASessionManager::OnStartSessionComplete);

    SoResults.MoStartSessionPtr = Ross.GetSessions().ExeServerStartSession(
        LoDelegate,
        SessionName,
        MoSessionSettings);
}

void ASessionManager::OnStartSessionComplete(FName InSessionName, bool bWasSuccessful)
{
    PrintStart();
    ensure(SessionName == InSessionName);
    if (bWasSuccessful) {
        Print("Session ", InSessionName, " started successfully.");
        SeState = EState::StartSession;
        //StartLevel();
    }
    else {
        PrintW("Failed to start session ", InSessionName, ".");
        //StartSession();
    }
}

void ASessionManager::StartLevel()
{
    PrintStart()
    if (SeState != EState::StartSession) {
        return;
    }
    SeState = EState::StartLevel;
    ensure(SsSessionName == SessionName);

    if (AGameModeBase* GM = GetWorld()->GetAuthGameMode())
        GM->bUseSeamlessTravel = true;
    else {
        PrintW("No Seamless Travel");
    }

    SoGameConfig.SetMapPath("/Game/Levels/Maps/PuzzelPlatforms");
    SoGameConfig.SetModePath("/Game/Levels/Modes/BpDefaultGameMode.BpDefaultGameMode_C");

    GET(LoWorld, GetWorld());
    auto LsPort = FString::FromInt(SoGameConfig.MnGamePort);
    auto LsTravel = FString::Printf(TEXT("%s?game=%s"), *SoGameConfig.GetMapPath(), *SoGameConfig.GetModePath());
    if (LsTravel == SsTravel)
        return;
    SsTravel = LsTravel;
    Print("Server Travel...\n Map: ", SoGameConfig.GetMapPath(), "\n GameMode: ", SoGameConfig.GetModePath(), "\n Port: ", SoGameConfig.GetGamePort(), "\n");
    Print("SsTravel = ", SsTravel);
    LoWorld.ServerTravel(SsTravel);
    Print("Server Travel called with URL: ", SsTravel);

    /// also, make sure you check your query/game ports match your ini file with your CLI input with an ensure()
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
    FName LoSessionName = NAME_GameSession;
    if (SessionInterface.GetNamedSession(SessionName))
    {
        LoSessionName = SessionName;
    }
    SessionInterface.RegisterPlayer(LoSessionName, *UniqueId, bWasFromInvite);
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
    PrintW("\n\nMap: ", SoGameConfig.GetMapPath(), "\n GameMode: ", SoGameConfig.GetModePath(), "\n Port: ", SoGameConfig.GetGamePort(), "\n");
    FString TravelURL = FString::Printf(TEXT("%s?listen&Game=%s"), *SoGameConfig.GetMapPath(), *SoGameConfig.GetModePath());
    World.ServerTravel(TravelURL);
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

FString ASessionManager::GetMapName() const
{
    PrintStart();
    FString LsMap;
    MoSessionSettings.Get(TEXT("MAP"), LsMap);
    Print("Registering Map: ", LsMap);
    return LsMap;
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
        SessionName = SsSessionName;
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
        // Add more data as needed from MoSessionSettings
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
ASessionManager::SteamConfig ASessionManager::SoSteamConfig;
ASessionManager::GameConfig ASessionManager::SoGameConfig;
FName ASessionManager::SsSessionName = "Unset";
TSet<FUniqueNetIdRepl> ASessionManager::SvSessionPlayers;
TMap<FUniqueNetIdRepl, APlayerController*> ASessionManager::SmAdmins;
FString ASessionManager::SsTravel;

void ASessionManager::GameConfig::SetFromCLI()
{
    PrintStart();
    uint32 LnPort = 7777;
    FParse::Value(FCommandLine::Get(), TEXT("port="),     SoGameConfig.MnGamePort);
    FParse::Value(FCommandLine::Get(), TEXT("MapPath="),  SoGameConfig.MsMapPath);
    FParse::Value(FCommandLine::Get(), TEXT("ModePath="), SoGameConfig.MsModePath);

    SetMapPath(SoGameConfig.MsMapPath);
    SetModePath(SoGameConfig.MsModePath);
}

void ASessionManager::GameConfig::SetMapPath(const FString& FsMapPath)
{
    PrintStart();
    The.MsMapPath = FsMapPath;
    The.MsMap = FPackageName::GetShortName(FsMapPath);
    ensure(!SoGameConfig.MsMapPath.IsEmpty());
    ensure(!SoGameConfig.MsMap.IsEmpty());
    ensure(FPackageName::DoesPackageExist(SoGameConfig.MsMapPath));
}

void ASessionManager::GameConfig::SetModePath(const FString& FsModePath)
{
    PrintStart();
    The.MsModePath = FsModePath;
    The.MsMode = FPackageName::GetShortName(FsModePath);
    ensure(!SoGameConfig.MsModePath.IsEmpty());
    ensure(!SoGameConfig.MsMode.IsEmpty());
    ensure(XF::BxGameModeExists(SoGameConfig.MsModePath));
}
