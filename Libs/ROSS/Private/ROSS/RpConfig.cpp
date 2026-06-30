#include "ROSS/RpConfig.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "GameFramework/PlayerState.h"

URpConfig::URpConfig() = default;
URpConfig::~URpConfig() = default;

void URpConfig::BeginPlay()
{
    Super::BeginPlay();
}

void URpConfig::Setup()
{
    if (!GetWorld())
        return;
    auto& SoWorld = GetNewWorld();
    if (SoWorld.GetNetMode() == NM_DedicatedServer) {
        LocalUserNum = 0;
    }
    else {
        GET(LoPC, SoWorld.GetFirstPlayerController());
        GET(LoLocalPlayer, LoPC.GetLocalPlayer());
        LocalUserNum =  LoLocalPlayer.GetControllerId();
    }

    MoOnlineSubsytemPtr = Online::GetSubsystem(GetWorld());
    GET(OSS, MoOnlineSubsytemPtr);
    MoIdentityPtr = OSS.GetIdentityInterface();
    checkf(MoIdentityPtr.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));

    auto* LoControllerPtr = SoWorld.GetFirstLocalPlayerFromController();
    auto LnControllerNum = LoControllerPtr ? LoControllerPtr->GetControllerId() : 0;

    MoUserIDPtr = MoIdentityPtr->GetUniquePlayerId(LnControllerNum);
    if (SoWorld.GetNetMode() != NM_DedicatedServer) {
        ensure(MoUserIDPtr.IsValid() && "The local user is not signed in.");
    }
}

UWorld* URpConfig::GetNewWorldPtr()
{
    auto LoNewWorldPtr = GetWorld();
    if (LoNewWorldPtr)
        SoWorldPtr = LoNewWorldPtr;
    ensure(SoWorldPtr);
    return SoWorldPtr;
}

UWorld& URpConfig::GetNewWorld()
{
    auto LoNewWorldPtr = GetNewWorldPtr();
    ensure(LoNewWorldPtr);
    return *LoNewWorldPtr;
}

IOnlineSubsystem& URpConfig::GetIOnlineSubsytem()
{
    if (MoOnlineSubsytemPtr == nullptr)
        Setup();
    ensure(MoOnlineSubsytemPtr);
    return *MoOnlineSubsytemPtr;
}

IOnlineIdentity& URpConfig::GetIdentity()
{
    if (!MoIdentityPtr.IsValid())
        Setup();
    ensure(MoIdentityPtr);
    return *MoIdentityPtr;
}

IOnlineIdentityPtr& URpConfig::GetIdentityPtr()
{
    if (!MoIdentityPtr.IsValid())
        Setup();
    ensure(MoIdentityPtr);
    return MoIdentityPtr;
}

FUniqueNetIdPtr& URpConfig::GetNetUserID()
{
    if (!MoUserIDPtr.IsValid())
        Setup();
    ensure(MoUserIDPtr);
    return MoUserIDPtr;
}

const IOnlineSubsystem& URpConfig::GetIOnlineSubsytem() const
{
    if (!MoOnlineSubsytemPtr)
        throw BBB("No valid pointer!!");
    return *MoOnlineSubsytemPtr;
}

const IOnlineIdentityPtr& URpConfig::GetIdentityPtr() const
{
    if (!MoIdentityPtr || !MoIdentityPtr.IsValid())
        throw BBB("No valid pointer!!");
    return MoIdentityPtr;
}

const IOnlineIdentity& URpConfig::GetIdentity() const
{
    if (!MoIdentityPtr || !MoIdentityPtr.IsValid())
        throw BBB("No valid pointer!!");
    return *MoIdentityPtr;
}

const FUniqueNetIdPtr& URpConfig::GetNetUserID() const
{
    if (!MoUserIDPtr || !MoUserIDPtr.IsValid())
        throw BBB("No valid pointer!!");
    return MoUserIDPtr;
}
