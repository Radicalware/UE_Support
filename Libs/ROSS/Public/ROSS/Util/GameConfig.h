#pragma once

#include "CoreMinimal.h"
#include "Access/General.h"

// ------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------
class FBaseConfig
{
public:
    FBaseConfig() = default;
    FBaseConfig(const FBaseConfig&) = default;
    FBaseConfig(FBaseConfig&&) = default;
    FBaseConfig& operator=(const FBaseConfig&) = default;
    FBaseConfig& operator=(FBaseConfig&&) = default;
    ~FBaseConfig() = default;

    uint32 MnGamePort = 7777;
};
// ------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------
struct FSteamConfig : public FBaseConfig
{
    FSteamConfig() = default;
    FSteamConfig(const FSteamConfig&) = default;
    FSteamConfig(FSteamConfig&&) = default;
    FSteamConfig& operator=(const FSteamConfig&) = default;
    FSteamConfig& operator=(FSteamConfig&&) = default;
    ~FSteamConfig() = default;

    uint32 unIP = 0;             // 0 = bind to all interfaces (most common)
    uint16 MnEphemeralPort = 0;  // 0 = Steam picks an ephemeral port (recommended)
    uint16 MnQueryPort = 27018;  // Source Query / A2S port for server browser
    //uint16 MnGamePort = 27017; // your game's main UDP port (clients connect here)
};
// ------------------------------------------------------------------------------------------------------
class FGameConfig : public FBaseConfig
{
public:
    FGameConfig() = default;
    FGameConfig(const FGameConfig&) = default;
    FGameConfig(FGameConfig&&) = default;
    FGameConfig& operator=(const FGameConfig&) = default;
    FGameConfig& operator=(FGameConfig&&) = default;
    ~FGameConfig() = default;

    void SetMapModeFromCLI();

    void SetMapPath(const FString& FsMapPath);
    void SetModePath(const FString& FsModePath);

    auto GetGamePort() const { return MnGamePort; }
    void SetGamePort(uint32 InPort) { MnGamePort = InPort; }
    const auto& GetMap()  const { return MsMap; }
    const auto& GetMode() const { return MsMode; }
    const auto& GetMapPath()  const { return MsMapPath; }
    const auto& GetModePath() const { return MsModePath; }

    const auto BxHasValues() const { return !MsMapPath.IsEmpty() && !MsModePath.IsEmpty(); }

private:
    FString MsMapPath;
    FString MsModePath;

    FString MsMap;
    FString MsMode;
};
// ------------------------------------------------------------------------------------------------------
