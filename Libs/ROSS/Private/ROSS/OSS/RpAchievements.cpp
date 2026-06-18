// Fill out your copyright notice in the Description page of Project Settings.


#include "ROSS/OSS/RpAchievements.h"
#include "Interfaces/OnlineAchievementsInterface.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"

URpAchievements::URpAchievements()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URpAchievements::BeginPlay()
{
	Super::BeginPlay();
}


sp<TNetResult<TArray<FAchievementState>>> URpAchievements::QueryAchievements()
{
    auto  Result = MakeThreadPtr(TNetResult<TArray<FAchievementState>>);
    auto& OSS = The.GetIOnlineSubsytem();
    auto& UserId = The.GetNetUserID();
    auto  Achievements = OSS.GetAchievementsInterface();
    if (!Achievements.IsValid())
    {
        Result->OnResult(
            false,
            TEXT("Online subsystem does not support achievements."));
        return Result;
    }

    // Query the achievement descriptions.
    Achievements->QueryAchievementDescriptions(
        *UserId,
        FOnQueryAchievementsCompleteDelegate::CreateWeakLambda(
            this,
            [this,
            Achievements,
            UserId,
            ResultWk = TWeakPtr<TNetResult<TArray<FAchievementState>>>(Result)]
            (const FUniqueNetId&, const bool bWasDescriptionSuccessful) 
            {
                GetWeakSafe(Result);
                if (!bWasDescriptionSuccessful)
                {
                    Result.OnResult(
                        false,
                        TEXT("QueryAchievementDescriptions call failed."));
                    return;
                }

                    // Now query the achievement state for this user.
                Achievements->QueryAchievements(
                    *UserId,
                    FOnQueryAchievementsCompleteDelegate::CreateWeakLambda(
                        this,
                        [Achievements, UserId, ResultWk](const FUniqueNetId&, const bool bWasSuccessful) 
                        {
                            GetWeakSafe(Result);
                            if (!bWasSuccessful)
                            {
                                Result.OnResult(
                                    false,
                                    TEXT("QueryAchievements call failed."));
                                return;
                            }

                            TArray<FOnlineAchievement> CurrentAchievements;
                            if (Achievements->GetCachedAchievements(*UserId, CurrentAchievements) == EOnlineCachedResult::NotFound)
                            {
                                Result.OnResult(
                                    false,
                                    TEXT("GetCachedAchievements call failed."));
                                return;
                            }

                            // Now we have cached both the achievement descriptions and the achievement states, we can                            // put both of them together to return a list of achievements and the current user's      
                            // progress.
                            auto StatesPtr = MakeShared<TArray<FAchievementState>>();
                            auto& States = *StatesPtr;
                            for (const auto& CurrentAchievement : CurrentAchievements)
                            {
                                FOnlineAchievementDesc AchievementDescription;
                                if (Achievements->GetCachedAchievementDescription(
                                        CurrentAchievement.Id, 
                                        AchievementDescription
                                    ) == EOnlineCachedResult::Success)
                                {
                                    FAchievementState State;
                                    State.Id = CurrentAchievement.Id;
                                    State.DisplayName = AchievementDescription.Title;
                                    State.Progress = static_cast<float>(CurrentAchievement.Progress);
                                    State.bUnlocked = CurrentAchievement.Progress > 100.0 ||
                                                        FMath::IsNearlyEqual(CurrentAchievement.Progress, 100.0);
                                    States.Add(State);
                                }
                            }

                            // Return the results.
                            Result.OnReturnResult(true, StatesPtr);
                        }));
            }));
    return Result;
}

sp<TNetResult<>> URpAchievements::UnlockAchievement(const FString& Id)
{
    auto Result = MakeThreadPtr(TNetResult<>);
    auto& OSS = The.GetIOnlineSubsytem();
    auto& UserId = The.GetNetUserID();
    auto Achievements = OSS.GetAchievementsInterface();
    if (!Achievements.IsValid())
    {
        Result->OnResult(
            false,
            TEXT("Online subsystem does not support achievements."));
        return Result;
    }

    // Create the "achievement write" object, which is how we specify what achievements will be unlocked.
    auto WriteObject = MakeShared<FOnlineAchievementsWrite>();

    WriteObject->SetFloatStat(Id, 100.0f);
    Achievements->WriteAchievements(
        *UserId,
        WriteObject,
        FOnAchievementsWrittenDelegate::CreateWeakLambda(
            this,
            [ResultWk = TWeakPtr<TNetResult<>>(Result)](const FUniqueNetId&, bool bWasSuccessful) {
                GetWeakSafe(Result);
                Result.OnResult(bWasSuccessful, bWasSuccessful ? TEXT("") : TEXT("WriteAchievements call failed."));
            }));
    return Result;
}


