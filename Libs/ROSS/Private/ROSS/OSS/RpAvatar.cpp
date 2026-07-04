// Fill out your copyright notice in the Description page of Project Settings.


#include "ROSS/OSS/RpAvatar.h"
#include "OnlineSubsystemUtils.h"
#include "ROSS/Util/AvatarInterface.h"

URpAvatar::URpAvatar()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void URpAvatar::BeginPlay()
{
    Super::BeginPlay();
}


// From: UCPlusPlusOSSv1OnlineAPI::ExecuteAvatarGetAvatar
TSharedPtr<TNetResult<UTexture>> URpAvatar::GetAvatar(
    const FUniqueNetIdRepl& TargetUserId)
{
    auto ResultPtr = MakeThreadPtr(TNetResult<UTexture>);
    auto Result = *ResultPtr;

    IOnlineSubsystem& OSS = GetIOnlineSubsytem();
    auto LocalUserId = GetNetUserID();

    auto Avatar = Online::GetAvatarInterface(&OSS); // Custom Avatar Ptr lives on Subsystem
    if (!Avatar.IsValid())
    {
        Result.OnResult(false, TEXT("Online subsystem does not support avatars."));
        return ResultPtr;
    }

    // Direct lambda capture (intentional per user request). Assumes this component lives until callback.
    // Use Weak Pointer for Auto safety: It stores a weak reference to the UObject you pass. 
    // If that UObject (e.g. this component/actor) is destroyed or GC�d before the delegate fires, 
    // the lambda is simply skipped. No crash from a dangling pointer.
    Avatar->GetAvatar(
        *LocalUserId,
        *TargetUserId,
        nullptr, // default texture param
        FOnGetAvatarComplete::CreateWeakLambda( // Avatar->GetAvatar calls the lambda with (bool, TSoftObjectPtr<UTexture>)
            this,
            [ResultWk = TWeakPtr<TNetResult<UTexture>>(ResultPtr)]
            (bool bWasSuccessful, TSoftObjectPtr<UTexture> SoftResultPtr)
            {
                GetWeakSafe(Result);
                if(ResultWk.IsValid() == false)
                {
                    Result.OnResult(false, TEXT("GetAvatar callback failed: Result pointer is no longer valid."));
                    return;
                }

                // Convert SoftObjectPtr -> StrongObjectPtr (load if necessary)
                UTexture* Texture = SoftResultPtr.Get();
                if (!Texture && !SoftResultPtr.IsNull())
                    Texture = SoftResultPtr.LoadSynchronous();
                TStrongObjectPtr<UTexture> StrongResultPtr;
                if (Texture)
                    StrongResultPtr = TStrongObjectPtr<UTexture>(Texture);

                Result.OnReturnResult(
                    bWasSuccessful,
                    StrongResultPtr,
                    FString(bWasSuccessful ? TEXT("") : TEXT("The GetAvatar call failed.")));
            }));

    return ResultPtr;
}


