// Fill out your copyright notice in the Description page of Project Settings.


#include "ROSS/OSS/RpCurrentUser.h"
#include "OnlineSubsystemUtils.h"

URpCurrentUser::URpCurrentUser()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URpCurrentUser::BeginPlay()
{
	Super::BeginPlay();
}


FString URpCurrentUser::GetCurrentUserDisplayName() const
{
    return GetIdentity().GetPlayerNickname(LocalUserNum);
}

FUniqueNetIdRepl URpCurrentUser::GetUniqueNetId() const
{
    return GetNetUserID(); // Identity->GetUniquePlayerId(LocalUserNum);
}

FString URpCurrentUser::GetCurrentUserSecondaryId() const
{
    if (!ShouldRenderUserSecondaryIdField())
        return TEXT("");
    auto UserAccount = GetIdentity().GetUserAccount(*GetNetUserID());
    checkf(UserAccount.IsValid(), TEXT("Expected GetUserAccount to return a valid account."));
    FString Value;
    return UserAccount->GetAuthAttribute(TEXT("epic.accountId"), Value) ? Value : TEXT("");
}


bool URpCurrentUser::ShouldRenderUserSecondaryIdField() const
{
    auto UserAccount = GetIdentity().GetUserAccount(*GetNetUserID());
    if (!UserAccount.IsValid())
        return false;
    FString Value;
    return UserAccount->GetAuthAttribute(TEXT("epic.accountId"), Value);
}

FString URpCurrentUser::GetCurrentUserAuthAttribute(const FString& Key) const
{
    auto UserAccount = GetIdentity().GetUserAccount(*GetNetUserID());
    checkf(UserAccount.IsValid(), TEXT("Expected GetUserAccount to return a valid account."));
    FString Value;
    return UserAccount->GetAuthAttribute(Key, Value) ? Value : TEXT("");
}