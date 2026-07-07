#include "ROSS/Manager/SessionManager.h"

// -----------------------------------------------
// Libs
#include "ROSS/ROSS.h"
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
ASessionManager::ASessionManager() : AGameSession()
{
    PrintStart();
    SessionName = SsSessionName;
}

ASessionManager::~ASessionManager()
{
    PrintStart();

    Print("Server shutting down not included (commented out)");
}

void ASessionManager::Tick(float DeltaSeconds)
{
        Super::Tick(DeltaSeconds);
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
    UROSS::SetWorld(GetWorld());
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

    const bool bPersistentWorld = FParse::Param(FCommandLine::Get(), TEXT("InstanceWorld")) == false;
    Print("Persistent World: ", bPersistentWorld);
    if (bPersistentWorld)
    {
        SoSettings.bAllowJoinInProgress = true;
        SoSettings.bAllowInvites = true;
    }
    else { // MTT, Sit-N-Go, Spins
        SoSettings.bAllowJoinInProgress = false; // Good prevent mid-match joins
        SoSettings.bAllowInvites = false; // Good keep control centralized
    }

    if (bUsingDedicatedServer) 
    {
        SoSettings.bIsDedicated = true;
        SoSettings.bIsLANMatch = false;
        SoSettings.bUsesPresence = true;
        SoSettings.bAllowJoinViaPresence = true;
        SoSettings.bUseLobbiesIfAvailable = false; // Use server browser path, not lobbies, for dedicated servers
    }
    else
    {
        SoSettings.bIsDedicated = false;
        SoSettings.bIsLANMatch = true;
        SoSettings.bUsesPresence = false;
        SoSettings.bAllowJoinViaPresence = false;
        SoSettings.bUseLobbiesIfAvailable = true;

    }

    SoSettings.bShouldAdvertise = true;
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

void ASessionManager::SetPort(uint32 FnPort)
{
    GetGameConfig().SetGamePort(FnPort);
}

void ASessionManager::RegisterServer()
{
    PrintStart();
    AGameSession::RegisterServer();
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
}

void ASessionManager::P2PCreateSession()
{
    PrintStart();
    if (GetWorld()->GetNetMode() == ENetMode::NM_DedicatedServer) {
        PrintW("Should be a Client NOT a dedicated server in P2P Sessions");
        return;
    }
    if (MeState != EState::None) {
        PrintW("Incorrect State: ", (int)MeState, " != ", (int)EState::None);
        return;
    }
    ensure(SsSessionName == SessionName);

    GET(GI, GetGameInstance());
    GET(Ross, GI.GetSubsystem<UROSS>());
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
        MeState = EState::CreateSession;
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
    if (MeState != EState::CreateSession) {
        PrintW("Incorrect State: ", (int)MeState, " != ", (int)EState::CreateSession);
        return;
    }
    ensure(SsSessionName == SessionName);

    if (GetWorld() && UROSS::InitializeReady(GetWorld()))
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

    auto LoResultPtr = GetROSS().GetSessions().ExeServerStartSession(
        FOnStartSessionCompleteDelegate::CreateUObject(this, &ASessionManager::OnStartSessionComplete),
        SoSettings);
}

void ASessionManager::P2PStartSession()
{
    if (GetWorld()->GetNetMode() == ENetMode::NM_DedicatedServer) {
        PrintW("P2P, not a dedicated server");
        return;
    }
    if (MeState != EState::CreateSession) {
        PrintW("Incorrect State: ", (int)MeState, " != ", (int)EState::CreateSession);
        return;
    }

    auto LoResultPtr = GetROSS().GetSessions().ExeServerStartSession(
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
    AGameSession::OnStartSessionComplete(InSessionName, bWasSuccessful);
    ensure(SessionName == InSessionName);
    if (bWasSuccessful) {
        Print("Session ", InSessionName, " started successfully.");
        MeState = EState::StartSession;
        ServerTravelListen();
    }
    else {
        PrintW("Failed to start session ", InSessionName, ".");
    }
}

void ASessionManager::ServerTravelListen(const FString& FsMapPath, const FString& FsModePath)
{
    PrintStart()
    if (MeState != EState::StartSession) {
        PrintW("Incorrect State: ", (int)MeState, " != ", (int)EState::StartSession);
        return;
    }
    ensure(SsSessionName == SessionName);

    GetROSS().GetSessions().ExeServerTravelToMapAndMode(ASessionManager::SoSettings);
    MeState = EState::ServerTravelListen;
}

void ASessionManager::ServerTravelJoin(const FString& FsMapPath, const FString& FsModePath)
{
    PrintStart()
    if (MeState != EState::StartSession) {
        return;
    }
    MeState = EState::ServerTravelListen;
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
    GET(LoWorld, GetWorld());
    GET(LoPC, LoWorld.GetFirstPlayerController());

    if (LoPC.HasAuthority() && IOnlineSubsystem::Get())
    {
        auto SessionInterface = IOnlineSubsystem::Get()->GetSessionInterface();
        if (SessionInterface.IsValid()) {
            SessionInterface->DestroySession(*ASessionManager::GetSettings().MsSessionName);
            MeState = EState::None;
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

//void ASessionManager::OnAutoLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
//{
//    PrintStart();
//    if (bWasSuccessful) {
//        Print("AutoLogin successful for LocalUserNum: ", LocalUserNum);
//    }
//    else {
//        PrintW("AutoLogin failed for LocalUserNum: ", LocalUserNum, ", Error: ", Error);
//    }
//}

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
    GetROSS().GetSessions().SearchSessions();
}

const UROSS& ASessionManager::GetROSS() const
{
    GET(GI, GetGameInstance());
    GET(Ross, GI.GetSubsystem<UROSS>());
    return Ross;
}

UROSS& ASessionManager::GetROSS()
{
    GET(GI, GetGameInstance());
    GET(Ross, GI.GetSubsystem<UROSS>());
    return Ross;
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

// Static member definitions
FName ASessionManager::SsSessionName = "Unset";
FRossSessionSettings ASessionManager::SoSettings;
TSet<FUniqueNetIdRepl> ASessionManager::SvSessionPlayers;
TMap<FUniqueNetIdRepl, APlayerController*> ASessionManager::SmAdmins;
FString ASessionManager::SsTravel;
