// Copyright June Rhodes. MIT Licensed.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/OnlineReplStructs.h"
#include "FriendState.generated.h"


UENUM(BlueprintType)
enum class EFriendInvitationStatus : uint8
{
    Unknown,
    Accepted,
    PendingInbound,
    PendingOutbound,
    Blocked,
    Suggested,
};

UENUM(BlueprintType)
enum class EFriendPresenceStatus : uint8
{
    Online,
    Offline,
    Away,
    ExtendedAway,
    DoNotDisturb,
    Chat,
};

USTRUCT(BlueprintType)
struct THEGAME_API FFriendState
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
    FUniqueNetIdRepl Id;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
    FString DisplayName;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
    FString RealName;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
    EFriendInvitationStatus InvitationStatus = EFriendInvitationStatus::Unknown;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
    FUniqueNetIdRepl PresenceSessionId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
    FString PresencePartyId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
    bool bPresenceIsOnline = false;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
    bool bPresenceIsPlaying = false;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
    bool bPresenceIsPlayingThisGame = false;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
    bool bPresenceIsJoinable = false;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
    bool bPresenceHasVoiceSupport = false;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
    FDateTime PresenceLastOnline;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
    FString PresenceStatusString;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
    EFriendPresenceStatus PresenceStatusState = EFriendPresenceStatus::Offline;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
    TMap<FString, FString> PresenceStatusProperties;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Friend")
    TMap<FString, FString> Attributes;
};
