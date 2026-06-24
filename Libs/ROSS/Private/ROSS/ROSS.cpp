#include "ROSS/ROSS.h"
#include "ROSS/RpConfig.h"
#include "Access/General.h"
#include "ROSS/SessionManager.h"
#include <Kismet/GameplayStatics.h>

#include "GameFramework/PlayerState.h"

// Include all Rp* component headers so we can create default subobjects
#include "ROSS/OSS/RpAchievements.h"
#include "ROSS/OSS/RpAuth.h"
#include "ROSS/OSS/RpAvatar.h"
#include "ROSS/OSS/RpCurrentUser.h"
#include "ROSS/OSS/RpEcommerce.h"
#include "ROSS/OSS/RpEvents.h"
#include "ROSS/OSS/RpFriends.h"
#include "ROSS/OSS/RpIdentity.h"
#include "ROSS/OSS/RpLeaderboards.h"
#include "ROSS/OSS/RpParties.h"
#include "ROSS/OSS/RpPresence.h"
#include "ROSS/OSS/RpSessions.h"
#include "ROSS/OSS/RpStats.h"
#include "ROSS/OSS/RpTitleFile.h"
#include "ROSS/OSS/RpUserCloud.h"
#include "ROSS/OSS/RpUsers.h"
#include "ROSS/OSS/RpVoiceChat.h"



#if USING_STEAM
#include "steam/steam_api.h"
FName AROSS::SsSybsystem = STEAM_SUBSYSTEM;
#else
FName AROSS::SsSybsystem = EOS_SUBSYSTEM;
#endif

AROSS::AROSS()
{
	PrimaryActorTick.bCanEverTick = false;

	// Always create a root scene component
	RootSceneComponentPtr = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = RootSceneComponentPtr;

	// Create all OSS wrapper components as default subobjects and attach to root
	MoAchievementsPtr = CreateDefaultSubobject<URpAchievements>(TEXT("Achievements"));
	MoAchievementsPtr->SetupAttachment(RootSceneComponentPtr);

	MoAuthPtr = CreateDefaultSubobject<URpAuth>(TEXT("Auth"));
	MoAuthPtr->SetupAttachment(RootSceneComponentPtr);

	MoAvatarPtr = CreateDefaultSubobject<URpAvatar>(TEXT("Avatar"));
	MoAvatarPtr->SetupAttachment(RootSceneComponentPtr);

	MoCurrentUser = CreateDefaultSubobject<URpCurrentUser>(TEXT("CurrentUser"));
	MoCurrentUser->SetupAttachment(RootSceneComponentPtr);

	MoEcommercePtr = CreateDefaultSubobject<URpEcommerce>(TEXT("Ecommerce"));
	MoEcommercePtr->SetupAttachment(RootSceneComponentPtr);

	MoEventsPtr = CreateDefaultSubobject<URpEvents>(TEXT("Events"));
	MoEventsPtr->SetupAttachment(RootSceneComponentPtr);

	MoFriendsPtr = CreateDefaultSubobject<URpFriends>(TEXT("Friends"));
	MoFriendsPtr->SetupAttachment(RootSceneComponentPtr);

	MoIdentityPtr = CreateDefaultSubobject<URpIdentity>(TEXT("Identity"));
	MoIdentityPtr->SetupAttachment(RootSceneComponentPtr);

	MoLeaderboardsPtr = CreateDefaultSubobject<URpLeaderboards>(TEXT("Leaderboards"));
	MoLeaderboardsPtr->SetupAttachment(RootSceneComponentPtr);

	MoPartiesPtr = CreateDefaultSubobject<URpParties>(TEXT("Parties"));
	MoPartiesPtr->SetupAttachment(RootSceneComponentPtr);

	MoPresencePtr = CreateDefaultSubobject<URpPresence>(TEXT("Presence"));
	MoPresencePtr->SetupAttachment(RootSceneComponentPtr);

	MoSessionsPtr = CreateDefaultSubobject<URpSessions>(TEXT("Sessions"));
	MoSessionsPtr->SetupAttachment(RootSceneComponentPtr);

	MoStatsPtr = CreateDefaultSubobject<URpStats>(TEXT("Stats"));
	MoStatsPtr->SetupAttachment(RootSceneComponentPtr);

	MoTitleFilePtr = CreateDefaultSubobject<URpTitleFile>(TEXT("TitleFile"));
	MoTitleFilePtr->SetupAttachment(RootSceneComponentPtr);

	MoUserCloudPtr = CreateDefaultSubobject<URpUserCloud>(TEXT("UserCloud"));
	MoUserCloudPtr->SetupAttachment(RootSceneComponentPtr);

	MoUsersPtr = CreateDefaultSubobject<URpUsers>(TEXT("Users"));
	MoUsersPtr->SetupAttachment(RootSceneComponentPtr);

	MoVoiceChatPtr = CreateDefaultSubobject<URpVoiceChat>(TEXT("VoiceChat"));
	MoVoiceChatPtr->SetupAttachment(RootSceneComponentPtr);
}

AROSS::~AROSS()
{
	Print("Destroying AROSS");
	SoWorldPtr = nullptr;
	SoROSSPtr  = nullptr;
}


AROSS& AROSS::GetRoss()
{
	return *AROSS::GetRossPtr();
}

TWeakObjectPtr<AROSS> AROSS::GetRossWPtr()
{
	if (!SoROSSPtr)
	{
		if (SoWorldPtr)
			AROSS::Setup();
		else
			BBB("No World Ptr");
	}
	return SoROSSPtr.Get();
}

TObjectPtr<AROSS> AROSS::GetRossPtr()
{
	if (!SoROSSPtr)
	{
		if (SoWorldPtr)
			AROSS::Setup();
		else
			BBB("No World Ptr");
	}
	return SoROSSPtr;
}

void AROSS::SetWorld(UWorld* FoWorldPtr)
{
	SoWorldPtr = FoWorldPtr;
	if(not SoWorldPtr)
        BBB("No World Ptr Set in AROSS::SetWorld");
}

UWorld& AROSS::GetWorldDrf()
{
	return *AROSS::GetWorldPtr();
}

UWorld* AROSS::GetWorldPtr()
{
	if (SoWorldPtr)
		return SoWorldPtr;
	SoWorldPtr = GetRoss().GetWorld();
	if(!SoWorldPtr)
        BBB("No World Ptr");
	return SoWorldPtr;
}

void AROSS::Setup()
{
	PrintStart();
	GET(SoWorld);
#if USING_STEAM

	if (SoWorld.GetNetMode() == ENetMode::NM_DedicatedServer)
	{
		if (!SteamGameServerUtils()) {
			PrintW("SteamGameServerUtils interface is not available.");
		}
		else {
			Print("Steam is running. AppID: ", SteamGameServerUtils()->GetAppID());
		}
	}
	else if (SoWorld.GetNetMode() == ENetMode::NM_Client || SoWorld.GetNetMode() == ENetMode::NM_Standalone)
	{
		if (!SteamUtils()) {
			PrintW("SteamUtils interface is not available.");
		}
		else {
			Print("Steam is running. AppID: ", SteamUtils()->GetAppID());
		}
	}
	else {
        Print("Running P2P, No Dedicated Server or Client mode detected.");
	}

#endif
	if (!SoROSSPtr) {
		auto* SpawnedActor = SoWorld.SpawnActor<AROSS>(AROSS::StaticClass());
		SoROSSPtr = SpawnedActor; // Safe assignment
	}
	GET(SoROSS);

	auto& LoTrackAuth = SoROSS.GetAuthTracker();
	if (!SoROSS.GetAuth().BxAuthReady())
	{
		LoTrackAuth.Slingshot(SoWorldPtr, "AROSS::Setup");
		return;
	}

	SoROSSPtr->SetupPostLogin();
}

void AROSS::Shutdown()
{
    SoROSSPtr = nullptr;
	SoWorldPtr = nullptr;
}

void AROSS::SetupPostLogin()
{
	PrintStart();
	GET(SoROSS);
	if (!InitializeReady(SoWorldPtr))
	{
        auto& LoTrackAuth = SoROSS.GetAuthTracker();
		LoTrackAuth.Slingshot(SoWorldPtr, "AROSS::SetupPostLogin");
		return;
	}
	auto Init = [](URpConfig* Comp)
	{
		if (ensure(Comp))
			Comp->Setup();
	};

	Init(SoROSS.MoAuthPtr);
	Init(SoROSS.MoAchievementsPtr);
	Init(SoROSS.MoAvatarPtr);
	Init(SoROSS.MoCurrentUser);
	Init(SoROSS.MoEcommercePtr);
	Init(SoROSS.MoEventsPtr);
	Init(SoROSS.MoFriendsPtr);
	Init(SoROSS.MoIdentityPtr);
	Init(SoROSS.MoLeaderboardsPtr);
	Init(SoROSS.MoPartiesPtr);
	Init(SoROSS.MoPresencePtr);
	Init(SoROSS.MoSessionsPtr);
	Init(SoROSS.MoStatsPtr);
	Init(SoROSS.MoTitleFilePtr);
	Init(SoROSS.MoUserCloudPtr);
	Init(SoROSS.MoUsersPtr);
	Init(SoROSS.MoVoiceChatPtr);
	bReady = true;
}

bool AROSS::InitializeReady(UWorld* FoWorldPtr)
{
	PrintStart();
	GET(FoWorld);
	auto LocalUserNum = 0;
	if (FoWorld.GetNetMode() == NM_DedicatedServer) {
		LocalUserNum = 0;
	}
	else {
		if(!FoWorld.GetFirstPlayerController())
            return false;
		GET(LoPC, FoWorld.GetFirstPlayerController());
        if (!LoPC.PlayerState)
            return false;
		GET(LoState, LoPC.PlayerState);
		LocalUserNum = LoState.GetUniqueID();
	}

	auto LoSubsystemPtr = Online::GetSubsystem(FoWorldPtr);
	if (!LoSubsystemPtr) {
        Print("No Online Subsystem");
		return false;
	}
	GET(OSS, LoSubsystemPtr);
	auto IdentityPtr = OSS.GetIdentityInterface();
	if (!IdentityPtr || !IdentityPtr.IsValid()) {
		Print("No Identity Ptr");
		return false;
	}
	if (FoWorld.GetNetMode() == NM_DedicatedServer) {
		Print("Success Server: AROSS::InitializeReady");
		return true;
	}

	auto* LoControllerPtr = FoWorld.GetFirstLocalPlayerFromController();
	auto LnControllerNum = LoControllerPtr ? LoControllerPtr->GetControllerId() : 0;
	auto UserIDPtr = IdentityPtr->GetUniquePlayerId(LnControllerNum);
    Print("LocalUserNum: ", LnControllerNum);
	if (!UserIDPtr || !UserIDPtr.IsValid()) {
		Print("No User ID Ptr");
		return false;
	}
	Print("Success Client: AROSS::InitializeReady");
    return true;
}

bool AROSS::BxReady()
{
	if (!SoWorldPtr || !SoROSSPtr)
		return false;
    if (!bReady)
        return false;
	return true;
}

void AROSS::BeginPlay()
{
	AActor::BeginPlay();
}

ATracker& AROSS::GetAuthTracker()
{
	GetTracker(MoTrackAuth);
	return MoTrackAuth;
}


