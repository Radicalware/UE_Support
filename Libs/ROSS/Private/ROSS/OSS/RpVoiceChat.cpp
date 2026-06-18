// Fill out your copyright notice in the Description page of Project Settings.


#include "ROSS/OSS/RpVoiceChat.h"
#include "OnlineSubsystemUtils.h"

URpVoiceChat::URpVoiceChat()
{

}

void URpVoiceChat::BeginPlay()
{
	Super::BeginPlay();
}

URpVoiceChat::~URpVoiceChat()
{
    if (this->CachedVoiceChatUser != nullptr)
    {
        auto* VC = IVoiceChat::Get();
        if (VC != nullptr)
        {
            VC->ReleaseUser(this->CachedVoiceChatUser);
            this->CachedVoiceChatUser = nullptr;
        }
    }
}

bool URpVoiceChat::GetIsLoggedIn() const
{
    return CachedVoiceChatUser != nullptr && CachedVoiceChatUser->IsLoggedIn();
}

FString URpVoiceChat::GetLoggedInPlayerName() const
{
    if (CachedVoiceChatUser == nullptr || !CachedVoiceChatUser->IsLoggedIn())
    {
        return TEXT("");
    }

    // Return the signed in player name.
    return CachedVoiceChatUser->GetLoggedInPlayerName();
}

sp<TNetResult<>> URpVoiceChat::ChatLogin()
{
    auto& OSS = GetIOnlineSubsytem();
    auto& Identity = GetIdentity();
    auto Result = MakeThreadPtr(TNetResult<>);

    auto* VC = IVoiceChat::Get();
    if (VC == nullptr)
    {
        Result->OnResult(false, TEXT("Voice chat service not available."));
        return Result;
    }

    // If we don't already have a voice chat user, create one.
    if (CachedVoiceChatUser == nullptr)
    {
        CachedVoiceChatUser = VC->CreateUser();
        if (CachedVoiceChatUser == nullptr)
        {
            Result->OnResult(false, TEXT("Unable to create voice chat user."));
            return Result;
        }
    }

    // Attempt to log the voice chat user in.
    CachedVoiceChatUser->Login(
        Identity.GetPlatformUserIdFromLocalUserNum(this->LocalUserNum),
        Identity.GetUniquePlayerId(this->LocalUserNum)->ToString(),
        TEXT(""),
        FOnVoiceChatLoginCompleteDelegate::CreateWeakLambda(
            this,
            [ResultWk = TWeakPtr<TNetResult<>>(
                Result)](const FString&, const FVoiceChatResult& VoiceChatResult) 
            {
                    GetWeakSafe(Result);
                    Result.OnResult(VoiceChatResult.IsSuccess(), VoiceChatResult.ErrorDesc);
            }));
    return Result;
}

sp<TNetResult<>> URpVoiceChat::ChatLogout()
{

    auto Result = MakeThreadPtr(TNetResult<>);
    if (CachedVoiceChatUser == nullptr || !CachedVoiceChatUser->IsLoggedIn())
    {
        Result->OnResult(false, TEXT("Not signed in."));
        return Result;
    }

    // Attempt to log the voice chat user out.
    CachedVoiceChatUser->Logout(FOnVoiceChatLogoutCompleteDelegate::CreateWeakLambda(
        this,
        [ResultWk = TWeakPtr<TNetResult<>>(Result)](const FString&, const FVoiceChatResult& VoiceChatResult) 
        {
            GetWeakSafe(Result);
            Result.OnResult(VoiceChatResult.IsSuccess(), VoiceChatResult.ErrorDesc);
        }));
    return Result;
}

TArray<FVoiceChatDeviceInfo> URpVoiceChat::GetInputDevices() const
{
    if (CachedVoiceChatUser == nullptr || !CachedVoiceChatUser->IsLoggedIn())
    {
        return TArray<FVoiceChatDeviceInfo>();
    }

    return CachedVoiceChatUser->GetAvailableInputDeviceInfos();
}

FString URpVoiceChat::GetCurrentInputDevice() const
{
    if (CachedVoiceChatUser == nullptr || !CachedVoiceChatUser->IsLoggedIn())
    {
        return TEXT("");
    }

    // Return the current input device ID.
    return CachedVoiceChatUser->GetInputDeviceInfo().Id;
}

void URpVoiceChat::SetCurrentInputDevice(const FString& Id)
{
    if (CachedVoiceChatUser == nullptr || !CachedVoiceChatUser->IsLoggedIn())
    {
        return;
    }

    // Change the input device ID.
    CachedVoiceChatUser->SetInputDeviceId(Id);
}

float URpVoiceChat::GetInputVolume() const
{
    if (CachedVoiceChatUser == nullptr || !CachedVoiceChatUser->IsLoggedIn())
    {
        return 0.0f;
    }

    // Return the volume.
    return CachedVoiceChatUser->GetAudioInputVolume();
}

void URpVoiceChat::SetInputVolume(float Volume)
{
    if (CachedVoiceChatUser == nullptr || !CachedVoiceChatUser->IsLoggedIn())
    {
        return;
    }

    // Change the volume.
    CachedVoiceChatUser->SetAudioInputVolume(Volume);
}

bool URpVoiceChat::GetInputMuted() const
{
    if (CachedVoiceChatUser == nullptr || !CachedVoiceChatUser->IsLoggedIn())
    {
        return false;
    }

    // Return the mute status.
    return CachedVoiceChatUser->GetAudioInputDeviceMuted();
}

void URpVoiceChat::SetInputMuted(bool bIsMuted)
{
    if (CachedVoiceChatUser == nullptr || !CachedVoiceChatUser->IsLoggedIn())
    {
        return;
    }

    // Change the mute status.
    CachedVoiceChatUser->SetAudioInputDeviceMuted(bIsMuted);
}

TArray<FVoiceChatDeviceInfo> URpVoiceChat::GetOutputDevices() const
{
    if (CachedVoiceChatUser == nullptr || !CachedVoiceChatUser->IsLoggedIn())
    {
        return TArray<FVoiceChatDeviceInfo>();
    }

    return CachedVoiceChatUser->GetAvailableOutputDeviceInfos();
}

FString URpVoiceChat::GetCurrentOutputDevice() const
{
    if (CachedVoiceChatUser == nullptr || !CachedVoiceChatUser->IsLoggedIn())
    {
        return TEXT("");
    }

    // Return the current output device ID.
    return CachedVoiceChatUser->GetOutputDeviceInfo().Id;
}

void URpVoiceChat::SetCurrentOutputDevice(const FString& Id)
{
    if (CachedVoiceChatUser == nullptr || !CachedVoiceChatUser->IsLoggedIn())
    {
        return;
    }

    // Change the output device ID.
    CachedVoiceChatUser->SetOutputDeviceId(Id);
}

float URpVoiceChat::GetOutputVolume() const
{
    if (CachedVoiceChatUser == nullptr || !CachedVoiceChatUser->IsLoggedIn())
    {
        return 0.0f;
    }

    // Return the volume.
    return CachedVoiceChatUser->GetAudioOutputVolume();
}

void URpVoiceChat::SetOutputVolume(float Volume)
{
    if (CachedVoiceChatUser == nullptr || !CachedVoiceChatUser->IsLoggedIn())
    {
        return;
    }

    // Change the volume.
    CachedVoiceChatUser->SetAudioOutputVolume(Volume);
}

bool URpVoiceChat::GetOutputMuted() const
{
    if (CachedVoiceChatUser == nullptr || !CachedVoiceChatUser->IsLoggedIn())
    {
        return false;
    }

    // Return the mute status.
    return CachedVoiceChatUser->GetAudioOutputDeviceMuted();
}

void URpVoiceChat::SetOutputMuted(bool bIsMuted)
{
    if (CachedVoiceChatUser == nullptr || !CachedVoiceChatUser->IsLoggedIn())
    {
        return;
    }

    // Change the mute status.
    CachedVoiceChatUser->SetAudioOutputDeviceMuted(bIsMuted);
}

EVoiceChatStatus URpVoiceChat::GetConnectionStatus() const
{
    // Make sure we have a voice chat implementation.
    auto* VC = IVoiceChat::Get();
    if (VC == nullptr)
    {
        return EVoiceChatStatus::NotConnected;
    }

    // Return the connection status.
    if (VC->IsConnecting())
    {
        return EVoiceChatStatus::Connecting;
    }
    else if (VC->IsConnected())
    {
        return EVoiceChatStatus::Connected;
    }
    else
    {
        return EVoiceChatStatus::NotConnected;
    }
}

FString URpVoiceChat::GetSetting(const FString& SettingKey) const
{
    if (CachedVoiceChatUser == nullptr || !CachedVoiceChatUser->IsLoggedIn())
    {
        return TEXT("");
    }

    // Return the voice chat setting.
    return CachedVoiceChatUser->GetSetting(SettingKey);
}

void URpVoiceChat::SetSetting(const FString& SettingKey, const FString& SettingValue)
{
    if (CachedVoiceChatUser == nullptr || !CachedVoiceChatUser->IsLoggedIn())
    {
        return;
    }

    // Change the voice chat setting.
    CachedVoiceChatUser->SetSetting(SettingKey, SettingValue);
}

TArray<FString> URpVoiceChat::GetChannels() const
{
    if (CachedVoiceChatUser == nullptr || !CachedVoiceChatUser->IsLoggedIn())
    {
        return TArray<FString>();
    }

    // Return the voice chat channels.
    return CachedVoiceChatUser->GetChannels();
}

