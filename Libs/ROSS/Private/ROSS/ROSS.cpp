#include "ROSS/ROSS.h"
#include "ROSS/RpConfig.h"
#include "Access/General.h"
#include "ROSS/Manager/SessionManager.h"
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

#include "OnlineSubsystem.h"
#include "OnlineSubsystemModule.h"
#include "OnlineSubsystemNames.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineSessionInterface.h"



#if defined(BxROSS)
	FName UROSS::SsSybsystem = NULL_SUBSYSTEM; // As of right now ROSS is a UE Subsystem, not a full Online Subsystem
#elif (BxSteam)
	#include "steam/steam_api.h"
	FName UROSS::SsSybsystem = STEAM_SUBSYSTEM;
#elif defined(BxEpic)
	FName UROSS::SsSybsystem = EOS_SUBSYSTEM;
#endif

UROSS::UROSS()
{
    PrintStart();
	MoAchievementsPtr = CreateDefaultSubobject<URpAchievements>(TEXT("Achievements"));
	MoAuthPtr = CreateDefaultSubobject<URpAuth>(TEXT("Auth"));
	MoAvatarPtr = CreateDefaultSubobject<URpAvatar>(TEXT("Avatar"));
	MoCurrentUser = CreateDefaultSubobject<URpCurrentUser>(TEXT("CurrentUser"));
	MoEcommercePtr = CreateDefaultSubobject<URpEcommerce>(TEXT("Ecommerce"));
	MoEventsPtr = CreateDefaultSubobject<URpEvents>(TEXT("Events"));
	MoFriendsPtr = CreateDefaultSubobject<URpFriends>(TEXT("Friends"));
	MoIdentityPtr = CreateDefaultSubobject<URpIdentity>(TEXT("Identity"));
	MoLeaderboardsPtr = CreateDefaultSubobject<URpLeaderboards>(TEXT("Leaderboards"));
	MoPartiesPtr = CreateDefaultSubobject<URpParties>(TEXT("Parties"));
	MoPresencePtr = CreateDefaultSubobject<URpPresence>(TEXT("Presence"));
	MoSessionsPtr = CreateDefaultSubobject<URpSessions>(TEXT("Sessions"));
	MoStatsPtr = CreateDefaultSubobject<URpStats>(TEXT("Stats"));
	MoTitleFilePtr = CreateDefaultSubobject<URpTitleFile>(TEXT("TitleFile"));
	MoUserCloudPtr = CreateDefaultSubobject<URpUserCloud>(TEXT("UserCloud"));
	MoUsersPtr = CreateDefaultSubobject<URpUsers>(TEXT("Users"));
	MoVoiceChatPtr = CreateDefaultSubobject<URpVoiceChat>(TEXT("VoiceChat"));
}


void UROSS::Initialize(FSubsystemCollectionBase& Collection)
{
    PrintStart();
    Print("ROSS Initializing");

    Super::Initialize(Collection);
	
    Print("ROSS Subsystem Initialized");

	if (GetWorld()) {
		SoWorldPtr = GetWorld();
	}
}

void UROSS::Deinitialize()
{
	SoWorldPtr = nullptr;
}

UROSS::~UROSS()
{
	Print("Destroying UROSS");
	SoWorldPtr = nullptr;
}

void UROSS::SetWorld(UWorld* FoWorldPtr)
{
	SoWorldPtr = FoWorldPtr;
	if(not SoWorldPtr)
        BBB("No World Ptr Set in UROSS::SetWorld");
}

void UROSS::SetupPostLogin(UWorld* LoadedWorldPtr)
{
	PrintStart();
    if (LoadedWorldPtr != nullptr)
        SoWorldPtr = LoadedWorldPtr;
	if (!InitializeReady(SoWorldPtr))
	{
        auto& LoTrackAuth = GetAuthTracker();
		LoTrackAuth.Slingshot(SoWorldPtr, "UROSS::SetupPostLogin");
		return;
	}
	auto Init = [](URpConfig* Comp)
	{
		if (ensure(Comp))
			Comp->Setup();
	};

	Init(The.MoAuthPtr);
	Init(The.MoAchievementsPtr);
	Init(The.MoAvatarPtr);
	Init(The.MoCurrentUser);
	Init(The.MoEcommercePtr);
	Init(The.MoEventsPtr);
	Init(The.MoFriendsPtr);
	Init(The.MoIdentityPtr);
	Init(The.MoLeaderboardsPtr);
	Init(The.MoPartiesPtr);
	Init(The.MoPresencePtr);
	Init(The.MoSessionsPtr);
	Init(The.MoStatsPtr);
	Init(The.MoTitleFilePtr);
	Init(The.MoUserCloudPtr);
	Init(The.MoUsersPtr);
	Init(The.MoVoiceChatPtr);

	bReady = true;
}

bool UROSS::InitializeReady(UWorld* FoWorldPtr)
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
		Print("Success Server: UROSS::InitializeReady");
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
	Print("Success Client: UROSS::InitializeReady");
    return true;
}

bool UROSS::BxReady()
{
	if (!SoWorldPtr)
		return false;
    if (!bReady)
        return false;
	return true;
}

ATracker& UROSS::GetAuthTracker()
{
	GetTracker(MoTrackAuth);
	return MoTrackAuth;
}


