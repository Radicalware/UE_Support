// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ROSS/RpConfig.h"
#include "OnlineSessionSettings.h"
#include "VoiceChat.h"
#include "RpVoiceChat.generated.h"


UENUM(BlueprintType)
enum class EVoiceChatStatus : uint8
{
    NotConnected     UMETA(DisplayName = "Not Connected"),
    Connecting       UMETA(DisplayName = "Connecting"),
    Connected        UMETA(DisplayName = "Connected"),
    Reconnecting     UMETA(DisplayName = "Reconnecting")
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class THEGAME_API URpVoiceChat : public URpConfig
{
    GENERATED_BODY()
public:
    URpVoiceChat();
protected:
    virtual void BeginPlay() override;
    virtual ~URpVoiceChat() override;
    // @note: We use this to cache the search results between finding sessions and joining sessions, as the Online API
    // Explorer can't accept FOnlineSessionSearchResult directly. In real game code, you should just store
    // FOnlineSessionSearchResult and pass it to JoinSession.
    TMap<FString, FOnlineSessionSearchResult> CachedSearchResults;

    // @note: We use this to track the last session ID that we followed a party into, so that when we get subsequent
    // "party data updated" events we don't join the session again.
    FString CachedLastPartySessionId;

    // @note: This is the voice chat user that has currently been created for interacting with voice chat.
    IVoiceChatUser* CachedVoiceChatUser;

public:
    bool GetIsLoggedIn() const;
    FString GetLoggedInPlayerName() const;
    sp<TNetResult<>> ChatLogin();
    sp<TNetResult<>> ChatLogout();
    TArray<FVoiceChatDeviceInfo> GetInputDevices() const;
    FString GetCurrentInputDevice() const;
    void SetCurrentInputDevice(const FString &Id);
    float GetInputVolume() const;
    void SetInputVolume(float Volume);
    bool GetInputMuted() const;
    void SetInputMuted(bool bIsMuted);
    TArray<FVoiceChatDeviceInfo> GetOutputDevices() const;
    FString GetCurrentOutputDevice() const;
    void SetCurrentOutputDevice(const FString &Id);
    float GetOutputVolume() const;
    void SetOutputVolume(float Volume);
    bool GetOutputMuted() const;
    void SetOutputMuted(bool bIsMuted);
    EVoiceChatStatus GetConnectionStatus() const;
    FString GetSetting(const FString &SettingKey) const;
    void SetSetting(const FString &SettingKey, const FString &SettingValue);
    TArray<FString> GetChannels() const;
};
