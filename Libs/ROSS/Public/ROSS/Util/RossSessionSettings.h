#pragma once

#include "CoreMinimal.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"

#include "Access/General.h"
#include "ROSS/Util/GameConfig.h"


class FRossSessionSettings
{
public:
	/** The number of publicly available connections advertised */
	int32 NumPublicConnections;
	/** The number of connections that are private (invite/password) only */
	int32 NumPrivateConnections;
	/** Whether this match is publicly advertised on the online service */
	bool bShouldAdvertise;
	/** Whether joining in progress is allowed or not */
	bool bAllowJoinInProgress;
	/** This game will be lan only and not be visible to external players */
	bool bIsLANMatch;
	/** Whether the server is dedicated or player hosted */
	bool bIsDedicated;
	/** Whether the match should gather stats or not */
	bool bUsesStats;
	/** Whether the match allows invitations for this session or not */
	bool bAllowInvites;
	/** Whether to display user presence information or not */
	bool bUsesPresence;
	/** Whether joining via player presence is allowed or not */
	bool bAllowJoinViaPresence;
	/** Whether joining via player presence is allowed for friends only or not */
	bool bAllowJoinViaPresenceFriendsOnly;
	/** Whether the server employs anti-cheat (punkbuster, vac, etc) */
	bool bAntiCheatProtected;
	/** Whether to prefer lobbies APIs if the platform supports them */
	bool bUseLobbiesIfAvailable;
	/** Whether to create (and auto join) a voice chat room for the lobby, if the platform supports it */
	bool bUseLobbiesVoiceChatIfAvailable;
	/** Manual override for the Session Id instead of having one assigned by the backend. Its size may be restricted depending on the platform */
	FString SessionIdOverride;

	/** Used to keep different builds from seeing each other during searches */
	int32 BuildUniqueId;
	/** Array of custom session settings */
	FSessionSettings Settings;

	/** Map of custom settings per session member (Not currently used by every OSS) */
	TUniqueNetIdMap<FSessionSettings> MemberSettings;

	FString MsSessionName;
	FGameConfig MoGameConfig;
	FSteamConfig MoSteamConfig;

public:
	inline auto& GetGameConfig() { return MoGameConfig; }
	inline auto& GetSteamConfig() { return MoSteamConfig; }

	inline auto& GetGameConfig()  const { return MoGameConfig; }
	inline auto& GetSteamConfig() const { return MoSteamConfig; }

	/** Default constructor, used when serializing a network packet */
	FRossSessionSettings()
		: NumPublicConnections(0)
		, NumPrivateConnections(0)
		, bShouldAdvertise(false)
		, bAllowJoinInProgress(false)
		, bIsLANMatch(false)
		, bIsDedicated(false)
		, bUsesStats(false)
		, bAllowInvites(false)
		, bUsesPresence(false)
		, bAllowJoinViaPresence(false)
		, bAllowJoinViaPresenceFriendsOnly(false)
		, bAntiCheatProtected(false)
		, bUseLobbiesIfAvailable(false)
		, bUseLobbiesVoiceChatIfAvailable(false)
		, BuildUniqueId(0)
	{
	}

	FRossSessionSettings(const FRossSessionSettings&) = default;
	FRossSessionSettings(FRossSessionSettings&&) = default;
	FRossSessionSettings& operator=(const FRossSessionSettings&) = default;
	FRossSessionSettings& operator=(FRossSessionSettings&&) = default;
	~FRossSessionSettings() = default;

	FRossSessionSettings(const FOnlineSessionSettings& FoSettings)
		: NumPublicConnections(FoSettings.NumPublicConnections)
		, NumPrivateConnections(FoSettings.NumPrivateConnections)
		, bShouldAdvertise(FoSettings.bShouldAdvertise)
		, bAllowJoinInProgress(FoSettings.bAllowJoinInProgress)
		, bIsLANMatch(FoSettings.bIsLANMatch)
		, bIsDedicated(FoSettings.bIsDedicated)
		, bUsesStats(FoSettings.bUsesStats)
		, bAllowInvites(FoSettings.bAllowInvites)
		, bUsesPresence(FoSettings.bUsesPresence)
		, bAllowJoinViaPresence(FoSettings.bAllowJoinViaPresence)
		, bAllowJoinViaPresenceFriendsOnly(FoSettings.bAllowJoinViaPresenceFriendsOnly)
		, bAntiCheatProtected(FoSettings.bAntiCheatProtected)
		, bUseLobbiesIfAvailable(FoSettings.bUseLobbiesIfAvailable)
		, bUseLobbiesVoiceChatIfAvailable(FoSettings.bUseLobbiesVoiceChatIfAvailable)
		, SessionIdOverride(FoSettings.SessionIdOverride)
		, BuildUniqueId(FoSettings.BuildUniqueId)
		, Settings(FoSettings.Settings)
		, MemberSettings(FoSettings.MemberSettings)
	{
		int32 LnPort;
		FoSettings.Get(TEXT("PORT"), LnPort);
		FString LsMapPath;
		FoSettings.Get(TEXT("MAP_PATH"), LsMapPath);
		FString LsModePath;
		FoSettings.Get(TEXT("MODE_PATH"), LsModePath);

		MoGameConfig.SetMapPath(LsMapPath);
		MoGameConfig.SetModePath(LsModePath);

		InitUniqueBuildID();
	}

	void SetSessionSettingsMapMode(FOnlineSessionSettings& FoSettings) const
	{
		FoSettings.Set(TEXT("PORT"), (int32)GetGameConfig().GetGamePort(), EOnlineDataAdvertisementType::ViaOnlineService);
		FoSettings.Set(TEXT("MAP"), GetGameConfig().GetMap(), EOnlineDataAdvertisementType::ViaOnlineService);
		FoSettings.Set(TEXT("MODE"), GetGameConfig().GetMode(), EOnlineDataAdvertisementType::ViaOnlineService);


		FoSettings.Set(TEXT("MAP_PATH"), GetGameConfig().GetMapPath(), EOnlineDataAdvertisementType::ViaOnlineService);
		FoSettings.Set(TEXT("MODE_PATH"), GetGameConfig().GetModePath(), EOnlineDataAdvertisementType::ViaOnlineService);
	}

	FRossSessionSettings(FOnlineSessionSettings&& FoSettings)
		: NumPublicConnections(FoSettings.NumPublicConnections)
		, NumPrivateConnections(FoSettings.NumPrivateConnections)
		, bShouldAdvertise(FoSettings.bShouldAdvertise)
		, bAllowJoinInProgress(FoSettings.bAllowJoinInProgress)
		, bIsLANMatch(FoSettings.bIsLANMatch)
		, bIsDedicated(FoSettings.bIsDedicated)
		, bUsesStats(FoSettings.bUsesStats)
		, bAllowInvites(FoSettings.bAllowInvites)
		, bUsesPresence(FoSettings.bUsesPresence)
		, bAllowJoinViaPresence(FoSettings.bAllowJoinViaPresence)
		, bAllowJoinViaPresenceFriendsOnly(FoSettings.bAllowJoinViaPresenceFriendsOnly)
		, bAntiCheatProtected(FoSettings.bAntiCheatProtected)
		, bUseLobbiesIfAvailable(FoSettings.bUseLobbiesIfAvailable)
		, bUseLobbiesVoiceChatIfAvailable(FoSettings.bUseLobbiesVoiceChatIfAvailable)
		, SessionIdOverride(MoveTemp(FoSettings.SessionIdOverride))
		, BuildUniqueId(FoSettings.BuildUniqueId)
		, Settings(MoveTemp(FoSettings.Settings))
		, MemberSettings(MoveTemp(FoSettings.MemberSettings))
	{
		SetSessionSettingsMapMode(FoSettings);
		InitUniqueBuildID();
	}

	FOnlineSessionSettings ToOnlineSessionSettings() const
	{
		auto Out = FOnlineSessionSettings();
		Out.NumPublicConnections = NumPublicConnections;
		Out.NumPrivateConnections = NumPrivateConnections;
		Out.bShouldAdvertise = bShouldAdvertise;
		Out.bAllowJoinInProgress = bAllowJoinInProgress;
		Out.bIsLANMatch = bIsLANMatch;
		Out.bIsDedicated = bIsDedicated;
		Out.bUsesStats = bUsesStats;
		Out.bAllowInvites = bAllowInvites;
		Out.bUsesPresence = bUsesPresence;
		Out.bAllowJoinViaPresence = bAllowJoinViaPresence;
		Out.bAllowJoinViaPresenceFriendsOnly = bAllowJoinViaPresenceFriendsOnly;
		Out.bAntiCheatProtected = bAntiCheatProtected;
		Out.bUseLobbiesIfAvailable = bUseLobbiesIfAvailable;
		Out.bUseLobbiesVoiceChatIfAvailable = bUseLobbiesVoiceChatIfAvailable;
		Out.SessionIdOverride = SessionIdOverride;
		Out.BuildUniqueId = BuildUniqueId;
		Out.Settings = Settings;
		Out.MemberSettings = MemberSettings;
		SetSessionSettingsMapMode(Out);
		return Out;
	}

	operator FOnlineSessionSettings()
	{
		return ToOnlineSessionSettings();
	}

	void InitUniqueBuildID()
	{
		BuildUniqueId = GetBuildUniqueId();
	}
};


