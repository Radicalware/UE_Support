#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Access/General.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionDelegates.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Access/General.h"
#include "ROSS.generated.h"

// Forward declarations for OSS wrappers
class URpAchievements;
class URpAuth;
class URpAvatar;
class URpCurrentUser;
class URpEcommerce;
class URpEvents;
class URpFriends;
class URpIdentity;
class URpLeaderboards;
class URpParties;
class URpPresence;
class URpSessions;
class URpStats;
class URpTitleFile;
class URpUserCloud;
class URpUsers;
class URpVoiceChat;

UCLASS()
class THEGAME_API UROSS : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UROSS();
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
	virtual ~UROSS();
	
	static auto& GetSubsystemName() { return SsSybsystem; }
	static auto& GetSubsystem()     { return *IOnlineSubsystem::Get(SsSybsystem); }

	UFUNCTION() void SetupPostLogin(UWorld* LoadedWorldPtr = nullptr);
	static bool InitializeReady(UWorld* FoWorldPtr);
	       bool BxReady();
	static void SetWorld(UWorld* FoWorldPtr);
protected:	
	INL static UWorld* SoWorldPtr = nullptr;

	UPROPERTY() class URpAchievements* MoAchievementsPtr = nullptr;
	UPROPERTY() class URpAuth* MoAuthPtr = nullptr;
	UPROPERTY() class URpAvatar* MoAvatarPtr = nullptr;
	UPROPERTY() class URpCurrentUser* MoCurrentUser = nullptr;
	UPROPERTY() class URpEcommerce* MoEcommercePtr = nullptr;
	UPROPERTY() class URpEvents* MoEventsPtr = nullptr;
	UPROPERTY() class URpFriends* MoFriendsPtr = nullptr;
	UPROPERTY() class URpIdentity* MoIdentityPtr = nullptr;
	UPROPERTY() class URpLeaderboards* MoLeaderboardsPtr= nullptr;
	UPROPERTY() class URpParties* MoPartiesPtr = nullptr;
	UPROPERTY() class URpPresence* MoPresencePtr = nullptr;
	UPROPERTY() class URpSessions* MoSessionsPtr = nullptr;
	UPROPERTY() class URpStats* MoStatsPtr = nullptr;
	UPROPERTY() class URpTitleFile* MoTitleFilePtr = nullptr;
	UPROPERTY() class URpUserCloud* MoUserCloudPtr = nullptr;
	UPROPERTY() class URpUsers* MoUsersPtr = nullptr;
	UPROPERTY() class URpVoiceChat* MoVoiceChatPtr = nullptr;

public:
	inline auto& GetAuth() const { return *MoAuthPtr; }
	inline auto& GetAuth() { return *MoAuthPtr; }
	
	inline auto& GetSessions() const { return *MoSessionsPtr; }
	inline auto& GetSessions() { return *MoSessionsPtr; }

	inline auto& GetAchievements() const { return *MoAchievementsPtr; }
	inline auto& GetAchievements() { return *MoAchievementsPtr; }

	inline auto& GetAvatar() const { return *MoAvatarPtr; }
	inline auto& GetAvatar() { return *MoAvatarPtr; }

	inline auto& GetCurrentUser() const { return *MoCurrentUser; }
	inline auto& GetCurrentUser() { return *MoCurrentUser; }

	inline auto& GetEcommerce() const { return *MoEcommercePtr; }
	inline auto& GetEcommerce() { return *MoEcommercePtr; }

	inline auto& GetEvents() const { return *MoEventsPtr; }
	inline auto& GetEvents() { return *MoEventsPtr; }

	inline auto& GetFriends() const { return *MoFriendsPtr; }
	inline auto& GetFriends() { return *MoFriendsPtr; }

	inline auto& GetIdentity() const { return *MoIdentityPtr; }
	inline auto& GetIdentity() { return *MoIdentityPtr; }

	inline auto& GetLeaderboards() const { return *MoLeaderboardsPtr; }
	inline auto& GetLeaderboards() { return *MoLeaderboardsPtr; }

	inline auto& GetParties() const { return *MoPartiesPtr; }
	inline auto& GetParties() { return *MoPartiesPtr; }

	inline auto& GetPresence() const { return *MoPresencePtr; }
	inline auto& GetPresence() { return *MoPresencePtr; }

	inline auto& GetStats() const { return *MoStatsPtr; }
	inline auto& GetStats() { return *MoStatsPtr; }

	inline auto& GetTitleFile() const { return *MoTitleFilePtr; }
	inline auto& GetTitleFile() { return *MoTitleFilePtr; }

	inline auto& GetUserCloud() const { return *MoUserCloudPtr; }
	inline auto& GetUserCloud() { return *MoUserCloudPtr; }

	inline auto& GetUsers() const { return *MoUsersPtr; }
	inline auto& GetUsers() { return *MoUsersPtr; }

	inline auto& GetVoiceChat() const { return *MoVoiceChatPtr; }
	inline auto& GetVoiceChat() { return *MoVoiceChatPtr; }

private:	
    bool bReady = false;
	static FName SsSybsystem;

	ATracker* MoTrackAuthPtr = nullptr;
	ATracker& GetAuthTracker();
};
