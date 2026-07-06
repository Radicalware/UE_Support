#pragma once

#include "CoreMinimal.h"
#include "Access/General.h"
#include "Ross/Util/TDelegate.h"
#include "Components/SceneComponent.h"
#include "ROSS/Util/NetResult.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSubsystem.h"

#include "RpConfig.generated.h"


/*
* This base class that handles all the RpX classes
* ROSS has all the Rpx class instances that use this URpConfig class
*/
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class THEGAME_API URpConfig : public USceneComponent
{
    GENERATED_BODY()
public:
    URpConfig();
    virtual ~URpConfig() override;
    virtual void BeginPlay() override;

    void Setup();
protected:
    UWorld* GetNewWorldPtr();
    UWorld& GetNewWorld();

public:
    IOnlineSubsystem& GetIOnlineSubsytem();
    IOnlineIdentity& GetIdentity();
    IOnlineIdentityPtr& GetIdentityPtr();
    FUniqueNetIdPtr& GetNetUserID();

    const IOnlineSubsystem& GetIOnlineSubsytem() const;
    const IOnlineIdentityPtr& GetIdentityPtr() const;
    const IOnlineIdentity& GetIdentity() const;
    const FUniqueNetIdPtr& GetNetUserID() const;

    INL const auto GetLocalUserNum() const { return LocalUserNum; }

protected:
    UWorld* GetOuterWorld();

    UPROPERTY(
        BlueprintReadOnly,
        VisibleAnywhere,
        meta = (DisplayName = "LocalUserNum", Category = "1Config"))
    int32 LocalUserNum = 0;

    INL static UWorld* SoWorldPtr = nullptr;
    IOnlineSubsystem*  MoOnlineSubsytemPtr = nullptr;
    IOnlineIdentityPtr MoIdentityPtr = nullptr;
    FUniqueNetIdPtr    MoUserIDPtr = nullptr;
    FName              MsSubsystem;
};
