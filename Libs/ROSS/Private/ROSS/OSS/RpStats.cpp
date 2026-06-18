// Fill out your copyright notice in the Description page of Project Settings.


#include "ROSS/OSS/RpStats.h"
#include "Interfaces/OnlineStatsInterface.h"
#include "OnlineSubsystemUtils.h"
URpStats::URpStats()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URpStats::BeginPlay()
{
	Super::BeginPlay();
}


sp<TNetResult<TArray<FStatState>>> URpStats::QueryStats()
{
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    auto Result = MakeThreadPtr(TNetResult<TArray<FStatState>>);
    auto Stats = OSS.GetStatsInterface();
    if (!Stats.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support stats."));
        return Result;
    }


    // Query the stats.
    Stats->QueryStats(
        UserId.ToSharedRef(),
        TArray<FUniqueNetIdRef>{UserId.ToSharedRef()},
        FStatState::GetStatNames(),
        FOnlineStatsQueryUsersStatsComplete::CreateWeakLambda(
            this,
            [UserId, ResultWk = TWeakPtr<TNetResult<TArray<FStatState>>>(Result)]
            (
                const FOnlineError& CallbackResult,
                const TArray<TSharedRef<const FOnlineStatsUserStats>>& CallbackUsersStats) 
            {
                    GetWeakSafe(Result);

                    // Return if the call failed.
                    if (!CallbackResult.bSucceeded || CallbackUsersStats.Num() == 0)
                    {
                        Result.OnResult(false, TEXT("QueryStats call failed."));
                        return;
                    }

                    // Convert the results.
                    auto EntriesPtr = MakeShared<TArray<FStatState>>();
                    auto& Entries = *EntriesPtr;
                    for (const auto& StatKV : CallbackUsersStats[0]->Stats)
                    {
                        FStatState Entry;
                        Entry.Name = StatKV.Key;
                        int32 Value;
                        StatKV.Value.GetValue(Value);
                        Entry.CurrentValue = Value;
                        Entries.Add(Entry);
                    }
                    Result.OnReturnResult(true, EntriesPtr, TEXT(""));
            }));
    return Result;
}

sp<TNetResult<>> URpStats::IngestStat(
    const FString& StatName,
    double StatValue)
{
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    auto Result = MakeThreadPtr(TNetResult<>);
    auto Stats = OSS.GetStatsInterface();
    if (!Stats.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support stats."));
        return Result;
    }

    // Create the stat update map.
    TArray<FOnlineStatsUserUpdatedStats> NewStats;
    NewStats.Add(FOnlineStatsUserUpdatedStats(
        UserId.ToSharedRef(),
        TMap<FString, FOnlineStatUpdate>{
            {StatName, FOnlineStatUpdate((int32)StatValue, FOnlineStatUpdate::EOnlineStatModificationType::Set)}}));

    // Update the stat.
    Stats->UpdateStats(
        UserId.ToSharedRef(),
        NewStats,
        FOnlineStatsUpdateStatsComplete::CreateWeakLambda(
            this,
            [ResultWk = TWeakPtr<TNetResult<>>(Result)](const FOnlineError& ResultState)
            {
                GetWeakSafe(Result);
                Result.OnResult(ResultState.bSucceeded, ResultState.ToLogString());
            }));
    return Result;
}

