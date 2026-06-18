// Copyright 2020 Shepherd Dowling All Rights Reserved

#pragma once

#include "Macros.h"
#include "Debug.h"
#include "XF.h"
#include <unordered_map>
#include <map>
template<typename  K, typename V>
using UMap = std::unordered_map<K, V>;
template<typename  K, typename V>
using OMap = std::map<K, V>;
#include "Access/PlayerBrace.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"

#define MPlayerBrace FPlayerBrace<P,C,S>

template<typename P, typename C, typename S>
class BaseRock
{
public:
    BaseRock() {}

    // Get Player's (State / Controller / Pawn)
    TArray<uint8> GetPlayerIds() const;
    TArray<MPlayerBrace> GetPlayersNoThrow() const;
    int32 GetTruePlayerCount() const;
    MPlayerBrace GetPlayer(const uint8 SeatID) const;
    OMap<int32, MPlayerBrace> GetNewPlayerBraces() const;
    S& GetPlayerState(uint8 TarGetPlayerId) const;
    S* GetPlayerStatePtr(uint8 TarGetPlayerId) const;
    TArray<S*> GetNewPlayerStates() const;
    TArray<S*> GetOrderedPlayerStates() const;
    TArray<S*> GetReverseOrderPlayerStates() const;
    
    UPROPERTY() UWorld* MoWorld = nullptr;
};
// ------------------------------------------------------------------------------------------------------------------

template<typename P, typename C, typename S>
TArray<uint8> BaseRock<P, C, S>::GetPlayerIds() const
{
    TArray<uint8> LvPlayerStates;
    for (APlayerState* BasePlayerStatePtr : MoWorld->GetGameState()->PlayerArray)
    {
        GET(BasePlayerState)
            LvPlayerStates.Add(BasePlayerState.GetPlayerId());
    }
    return LvPlayerStates;
}

template<typename P, typename C, typename S>
TArray<MPlayerBrace> BaseRock<P, C, S>::GetPlayersNoThrow() const
{
    TArray<MPlayerBrace> Players;
    for (APlayerState* BasePlayerState : MoWorld->GetGameState()->PlayerArray)
    {
        S* State = Cast<S>(BasePlayerState);
        // if (!State) continue;
        // P* HoldemPawn = State->GetPawn<P>();
        // if (!HoldemPawn) continue;
        // C* HoldemController = HoldemPawn->GetController<C>();
        // if (!HoldemController) continue;

        Players.Add(MPlayerBrace(Cast<S>(BasePlayerState)));
    }
    return Players;
}

template<typename P, typename C, typename S>
int32 BaseRock<P, C, S>::GetTruePlayerCount() const
{
    return MoWorld->GetGameState()->PlayerArray.Num();
}

template<typename P, typename C, typename S>
MPlayerBrace BaseRock<P, C, S>::GetPlayer(const uint8 SeatID) const
{
    for (APlayerState* BasePlayerState : MoWorld->GetGameState()->PlayerArray)
    {
        S* State = Cast<S>(BasePlayerState);
        // if (!ensure(State)) { PrintW("Error: BaseRock<P,C,S>::GetPlayersAndEnsure State");  continue; }
        // if (State->GetPlayerId() != SeatID)
        //     continue;
        // P* HoldemPawn = State->GetPawn<P>();
        // if (!ensure(HoldemPawn)) { PrintW("Error: BaseRock<P,C,S>::GetPlayersAndEnsure Pawn");  continue; };
        // C* HoldemController = HoldemPawn->GetController<C>();
        // if (!ensure(HoldemController)) { PrintW("Error: BaseRock<P,C,S>::GetPlayersAndEnsure Controller");  continue; };

        return MPlayerBrace(Cast<S>(BasePlayerState));
    }
    throw BBB("No Player found with ID: ", SeatID);
}

template<typename P, typename C, typename S>
OMap<int32, MPlayerBrace> BaseRock<P, C, S>::GetNewPlayerBraces() const
{
    OMap<int32, MPlayerBrace> LmPlayers;
    for (APlayerState* BasePlayerState : MoWorld->GetGameState()->PlayerArray)
    {
        auto LoBrace = MPlayerBrace(Cast<S>(BasePlayerState));
        LmPlayers.insert_or_assign(LoBrace.GetPlayerID(), LoBrace);
    }

    if(!LmPlayers.size())
        return LmPlayers;
    
    // Create Linked List
    {
        MPlayerBrace* LoLastBracePtr = nullptr;
        auto LnFirst = 0;
        auto LnLast = 0;
        for (auto& Pair : LmPlayers)
        {
            if (!LoLastBracePtr)
            {
                LoLastBracePtr = &Pair.second;
                LnFirst = Pair.first;
                continue;
            }

            Pair.second.SetLastLink(LoLastBracePtr);
            LoLastBracePtr->SetNextLink(&Pair.second);
            LoLastBracePtr = &Pair.second;
            LnLast = Pair.first;
        }
        if(LmPlayers.size() == 1)
            LnLast = LnFirst;
        LmPlayers[LnLast].SetNextLink(&LmPlayers[LnFirst]); // last loops back to the first
        LmPlayers[LnFirst].SetLastLink(&LmPlayers[LnLast]); // first links back to the last
    }

    return LmPlayers;
}

template<typename P, typename C, typename S>
S& BaseRock<P, C, S>::GetPlayerState(uint8 TarGetPlayerId) const
{
    for (APlayerState* BasePlayerState : MoWorld->GetGameState()->PlayerArray)
    {
        S* PlayerStatePtr = Cast<S>(BasePlayerState);
        GetRef(PlayerState);
        if (PlayerState.GetPlayerId() == TarGetPlayerId)
            return PlayerState;
    }
    throw BBB("Could not find player state with ID = ", TarGetPlayerId);
}

template<typename P, typename C, typename S>
S* BaseRock<P, C, S>::GetPlayerStatePtr(uint8 TarGetPlayerId) const
{
    for (APlayerState* BasePlayerState : MoWorld->GetGameState()->PlayerArray)
    {
        S* PlayerStatePtr = Cast<S>(BasePlayerState);
        GetRef(PlayerState);
        if (PlayerState.GetPlayerId() == TarGetPlayerId)
            return PlayerStatePtr;
    }
    return nullptr;
}

template<typename P, typename C, typename S>
TArray<S*> BaseRock<P, C, S>::GetNewPlayerStates() const
{
    TArray<S*> PlayerStates;
    for (APlayerState* BasePlayerState : MoWorld->GetGameState()->PlayerArray)
    {
        S* PlayerStatePtr = Cast<S>(BasePlayerState);
        NullThrow(PlayerStatePtr);
        PlayerStates.Add(PlayerStatePtr);
    }
    return PlayerStates;
}

template<typename P, typename C, typename S>
TArray<S*> BaseRock<P, C, S>::GetOrderedPlayerStates() const
{
    TArray<S*> PlayerStates = GetNewPlayerStates();
    PlayerStates.Sort([](const S& First, const S& Second) ->bool
        {
            return  First.GetPlayerId() < Second.GetPlayerId();
        });
    return PlayerStates;
}

template<typename P, typename C, typename S>
TArray<S*> BaseRock<P, C, S>::GetReverseOrderPlayerStates() const
{
    TArray<S*> PlayerStates = GetNewPlayerStates();
    PlayerStates.Sort([](const S& First, const S& Second) ->bool
        {
            return  First.GetPlayerId() > Second.GetPlayerId();
        });
    return PlayerStates;
}
