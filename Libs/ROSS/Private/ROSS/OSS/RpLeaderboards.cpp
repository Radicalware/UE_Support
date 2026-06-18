// Fill out your copyright notice in the Description page of Project Settings.


#include "ROSS/OSS/RpLeaderboards.h"
#include "OnlineStats.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineLeaderboardInterface.h"

URpLeaderboards::URpLeaderboards()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URpLeaderboards::BeginPlay()
{
	Super::BeginPlay();
}

sp<TNetResult<TArray<FLeaderboardEntry>>> URpLeaderboards::QueryGlobalLeaderboards()
{
    auto Result = MakeThreadPtr(TNetResult<TArray<FLeaderboardEntry>>);
    auto& OSS = The.GetIOnlineSubsytem();
    auto& UserID = The.GetNetUserID();
    auto Leaderboards = OSS.GetLeaderboardsInterface();
    if (!Leaderboards.IsValid())
    {
        Result->OnReturnResult(
            false,
            nullptr,
            TEXT("Online subsystem does not support friends."));
        return Result;
    }

    // Construct the read object, which will be populated with our results.
    auto ReadObject = MakeShared<FOnlineLeaderboardRead>();
    ReadObject->LeaderboardName = FString(TEXT("TestScore"));

    // Register an event so we can receive the query outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle =
        Leaderboards->AddOnLeaderboardReadCompleteDelegate_Handle(FOnLeaderboardReadCompleteDelegate::CreateWeakLambda(
            this,
            [   Leaderboards,
                CallbackHandle,
                ReadObject,
                ResultWk = TWeakPtr<TNetResult<TArray<FLeaderboardEntry>>>(Result)
            ]
            (bool bCallbackWasSuccessful) 
            {
                // Check if this callback is for us.
                if (ReadObject->ReadState != EOnlineAsyncTaskState::Failed &&
                    ReadObject->ReadState != EOnlineAsyncTaskState::Done)
                {
                    // This callback isn't for our call.
                    return;
                }

                GetWeakSafe(Result); // Make sure the result callback is still valid.
                if (ReadObject->ReadState == EOnlineAsyncTaskState::Failed)
                {
                    Result.OnResult(
                        false,
                        TEXT("Leaderboard read failed."));
                    Leaderboards->ClearOnLeaderboardReadCompleteDelegate_Handle(*CallbackHandle);
                    return;
                }

                // Otherise, convert the results.
                auto ResultsPtr = MakeShared<TArray<FLeaderboardEntry>>();
                auto& Results = *ResultsPtr;
                for (const auto& Row : ReadObject->Rows)
                {
                    // @note: On EOS, when you perform a ranked search, the value is always in the "Score" column.
                    int Score;
                    Row.Columns["Score"].GetValue(Score);

                    auto Entry = FLeaderboardEntry{};
                    Entry.CurrentValue = Score;
                    Entry.PlayerName = FString::Printf(TEXT("%s - %s"), *Row.PlayerId->ToString(), *Row.NickName);
                    Entry.Rank = Row.Rank;
                    Results.Add(Entry);
                }

                // Return the results.
                Result.OnReturnResult(true, ResultsPtr, TEXT(""));

                // Unregister this callback since we've handled the call we care about.
                Leaderboards->ClearOnLeaderboardReadCompleteDelegate_Handle(*CallbackHandle);
            }));

    // Query the global leaderboard.
    if (!Leaderboards->ReadLeaderboardsAroundRank(0, 100, ReadObject))
    {
        Result->OnResult(
            false,
            TEXT("ReadLeaderboardsAroundRank call failed to start."));
        Leaderboards->ClearOnLeaderboardReadCompleteDelegate_Handle(*CallbackHandle);
    }
    return Result;
}

sp<TNetResult<TArray<FLeaderboardEntry>>> URpLeaderboards::QueryFriendsLeaderboards()
{
    auto Result = MakeThreadPtr(TNetResult<TArray<FLeaderboardEntry>>);
    auto& OSS = The.GetIOnlineSubsytem();
    auto& UserID = The.GetNetUserID();
    auto Leaderboards = OSS.GetLeaderboardsInterface();
    if (!Leaderboards.IsValid())
    {
        Result->OnResult(
            false,
            TEXT("Online subsystem does not support friends."));
        return Result;
    }

    // Construct the read object, which will be populated with our results.
    auto ReadObject = MakeShared<FOnlineLeaderboardRead>();
    ReadObject->ColumnMetadata.Add(FColumnMetaData(FString(TEXT("TestScore")), EOnlineKeyValuePairDataType::Int32));

    // Register an event so we can receive the query outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle =
        Leaderboards->AddOnLeaderboardReadCompleteDelegate_Handle(FOnLeaderboardReadCompleteDelegate::CreateWeakLambda(
            this,
            [Leaderboards,
            CallbackHandle,
            ReadObject,
            ResultWk = TWeakPtr<TNetResult<TArray<FLeaderboardEntry>>>(Result)
            ]
            (bool bCallbackWasSuccessful)
            {
                // Check if this callback is for us.
                if (ReadObject->ReadState != EOnlineAsyncTaskState::Failed &&
                    ReadObject->ReadState != EOnlineAsyncTaskState::Done)
                {
                    // This callback isn't for our call.
                    return;
                }

                GetWeakSafe(Result); // Make sure the result callback is still valid.

                // Return if the read failed.
                if (ReadObject->ReadState == EOnlineAsyncTaskState::Failed)
                {
                    Result.OnResult(
                        false,
                        TEXT("Leaderboard read failed."));
                    Leaderboards->ClearOnLeaderboardReadCompleteDelegate_Handle(*CallbackHandle);
                    return;
                }

                auto ResultsPtr = MakeShared<TArray<FLeaderboardEntry>>();
                auto& Results = *ResultsPtr;
                for (const auto& Row : ReadObject->Rows)
                {
                    // @note: On EOS, when you perform a friend search, 
                    //      the value is in a column that matches the stat name.
                    int Score;
                    Row.Columns["TestScore"].GetValue(Score);

                    auto Entry = FLeaderboardEntry{};
                    Entry.CurrentValue = Score;
                    Entry.PlayerName = FString::Printf(TEXT("%s - %s"), *Row.PlayerId->ToString(), *Row.NickName);
                    Entry.Rank = Row.Rank;
                    Results.Add(Entry);
                }

                // Return the results.
                Result.OnReturnResult(true, ResultsPtr, TEXT(""));

                // Unregister this callback since we've handled the call we care about.
                Leaderboards->ClearOnLeaderboardReadCompleteDelegate_Handle(*CallbackHandle);
            }));

    // Query the stats for our friends.
    if (!Leaderboards->ReadLeaderboardsForFriends(this->LocalUserNum, ReadObject))
    {
        Result->OnResult(
            true,
            TEXT("ReadLeaderboardsForFriends call failed to start."));
        Leaderboards->ClearOnLeaderboardReadCompleteDelegate_Handle(*CallbackHandle);
    }
    return Result;
}