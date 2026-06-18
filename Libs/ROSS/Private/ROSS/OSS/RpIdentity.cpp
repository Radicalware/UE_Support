// Fill out your copyright notice in the Description page of Project Settings.


#include "ROSS/OSS/RpIdentity.h"
#include "OnlineSubsystemUtils.h"

URpIdentity::URpIdentity()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URpIdentity::BeginPlay()
{
	Super::BeginPlay();
}

FUniqueNetIdRepl URpIdentity::GetIdentityUniqueNetId(const FString& UniqueNetId)
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        return FUniqueNetIdRepl();
    }

    // Get the identity interface.
    auto Identity = OSS->GetIdentityInterface();
    if (!Identity.IsValid())
    {
        return FUniqueNetIdRepl();
    }

    // Construct the unique player ID from the string value.
    return Identity->CreateUniquePlayerId(UniqueNetId);
}
