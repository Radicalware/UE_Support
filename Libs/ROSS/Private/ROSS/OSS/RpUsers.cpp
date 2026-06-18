// Fill out your copyright notice in the Description page of Project Settings.


#include "ROSS/OSS/RpUsers.h"
#include "Interfaces/OnlineUserInterface.h"
#include "OnlineSubsystemUtils.h"

URpUsers::URpUsers()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URpUsers::BeginPlay()
{
	Super::BeginPlay();
}

sp<TNetResult<FString>> URpUsers::QueryUserInfo(const FString& UserIdStr)
{

    auto& OSS = GetIOnlineSubsytem();
    auto& Identity = GetIdentity();
    auto UserId = GetNetUserID();
    auto Result = MakeThreadPtr(TNetResult<FString>);
    auto UserInfo = OSS.GetUserInterface();
    if (!UserInfo.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support user lookup."));
        return Result;
    }

    // Parse the target user ID.
    auto TargetUserId = Identity.CreateUniquePlayerId(UserIdStr);
    if (!TargetUserId.IsValid())
    {
        Result->OnResult(false, TEXT("The target user ID is not valid."));
        return Result;
    }

    // Register an event so we can receive the query outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle = UserInfo->AddOnQueryUserInfoCompleteDelegate_Handle(
        this->LocalUserNum,
        FOnQueryUserInfoCompleteDelegate::CreateWeakLambda(
            this,
            [   this,
                UserInfo,
                CallbackHandle,
                TargetUserId,
                ResultWk = TWeakPtr<TNetResult<FString>>(Result)
            ]
            (
                int32 CallbackLocalUserNum,
                bool bCallbackWasSuccessful,
                const TArray<FUniqueNetIdRef>& CallbackUserIds,
                const FString& CallbackError) 
            {
                    if (!CallbackUserIds.Contains(TargetUserId.ToSharedRef()))
                        return;

                    GetWeakSafe(Result);
                    if (!bCallbackWasSuccessful)
                    {
                        Result.OnResult(false, TEXT("User query failed."));
                        UserInfo->ClearOnQueryUserInfoCompleteDelegate_Handle(this->LocalUserNum, *CallbackHandle);
                        return;
                    }

                    auto TargetUser = UserInfo->GetUserInfo(this->LocalUserNum, *TargetUserId);
                    if (!TargetUser.IsValid())
                    {
                        Result.OnResult(false, TEXT("GetUserInfo call did not return a user."));
                        UserInfo->ClearOnQueryUserInfoCompleteDelegate_Handle(this->LocalUserNum, *CallbackHandle);
                        return;
                    }
                    Result.OnReturnResult(
                        true,
                        MakeShared<FString>(FString::Printf(
                            TEXT("%s = %s"),
                            *TargetUser->GetUserId()->ToString(),
                            *TargetUser->GetDisplayName())));

                    // Unregister this callback since we've handled the call we care about.
                    UserInfo->ClearOnQueryUserInfoCompleteDelegate_Handle(this->LocalUserNum, *CallbackHandle);
            }));

    // Query for the user.
    if (!UserInfo->QueryUserInfo(this->LocalUserNum, TArray<FUniqueNetIdRef>{TargetUserId.ToSharedRef()}))
    {
        Result->OnResult(false, TEXT("QueryUserInfo call failed to start."));
        UserInfo->ClearOnQueryUserInfoCompleteDelegate_Handle(this->LocalUserNum, *CallbackHandle);
    }
    return Result;
}

sp<TNetResult<FString>> URpUsers::QueryUserByDisplayName(const FString& DisplayName)
{
    auto& OSS = GetIOnlineSubsytem();
    auto& Identity = GetIdentity();
    auto UserId = GetNetUserID();
    auto Result = MakeThreadPtr(TNetResult<FString>);
    auto UserInfo = OSS.GetUserInterface();
    if (!UserInfo.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support user lookup."));
        return Result;
    }

    // Query for the user by their display name.
    if (!UserInfo->QueryUserIdMapping(
        *UserId,
        DisplayName,
        IOnlineUser::FOnQueryUserMappingComplete::CreateWeakLambda(
            this,
            [ResultWk = TWeakPtr<TNetResult<FString>>(Result)](
                bool bWasSuccessful,
                const FUniqueNetId&,
                const FString&,
                const FUniqueNetId& FoundUserId,
                const FString& Error) 
            {
                    GetWeakSafe(Result);
                    if (bWasSuccessful)
                        Result.OnReturnResult(bWasSuccessful, MakeShared<FString>(FoundUserId.ToString()));
                    else
                        Result.OnResult(bWasSuccessful, Error);
            })))
    {
        Result->OnResult(false, TEXT("QueryUserIdMapping call failed to start."));
    }
    return Result;
}

sp<TNetResult<FString>> URpUsers::QueryExternalIds(const FString& PlatformName, const TArray<FString>& ExternalIds)
{
    auto& OSS = GetIOnlineSubsytem();
    auto& Identity = GetIdentity();
    auto UserId = GetNetUserID();
    auto Result = MakeThreadPtr(TNetResult<FString>);
    auto UserInfo = OSS.GetUserInterface();
    if (!UserInfo.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support user lookup."));
        return Result;
    }

    // Query for external user IDs.
    if (!UserInfo->QueryExternalIdMappings(
        *UserId,
        FExternalIdQueryOptions(PlatformName, false),
        ExternalIds,
        IOnlineUser::FOnQueryExternalIdMappingsComplete::CreateWeakLambda(
            this,
            [UserInfo, PlatformName, ExternalIds, ResultWk = TWeakPtr<TNetResult<FString>>(Result)](
                bool bWasSuccessful,
                const FUniqueNetId&,
                const FExternalIdQueryOptions&,
                const TArray<FString>&,
                const FString& Error) 
            {

                    GetWeakSafe(Result);
                    TArray<FUniqueNetIdPtr> FoundUserIds;
                    UserInfo->GetExternalIdMappings(
                        FExternalIdQueryOptions(PlatformName, false),
                        ExternalIds,
                        FoundUserIds);

                    TArray<FString> Lines;
                    for (const auto& FoundUserId : FoundUserIds)
                    {
                        if (FoundUserId.IsValid())
                        {
                            Lines.Add(FoundUserId->ToString());
                        }
                    }

                    if (bWasSuccessful)
                        Result.OnReturnResult(bWasSuccessful, MakeShared<FString>(FString::Join(Lines, TEXT("\n"))));
                    else
                        Result.OnResult(bWasSuccessful, Error);
            })))
    {
        Result->OnResult(false, TEXT("QueryExternalIdMappings call failed to start."));
    }
    return Result;
}
