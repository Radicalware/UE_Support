#pragma once

#include "CoreMinimal.h"

template<typename P, typename C, typename S>
class FPlayerBrace
{
private:
    P* MoPawnPtr = nullptr;
    C* MoControllerPtr = nullptr;
    S* MoStatePtr = nullptr;
    FPlayerBrace* MoLastBracePtr = nullptr;
    FPlayerBrace* MoNextBracePtr = nullptr;
public:
    FPlayerBrace() {};
    FPlayerBrace(S* FoStatePtr);
    FPlayerBrace(FPlayerBrace&& Other);
    FPlayerBrace(const FPlayerBrace& Other);
    void operator=(FPlayerBrace&& Other);
    void operator=(const FPlayerBrace& Other);

    P& GetPawn();
    C& GetController();
    S& GetState();

    const P& GetPawn()       const;
    const C& GetController() const;
    const S& GetState()      const;

    bool operator!() const;
    bool BxUsable()  const { return !!(*this); }
    bool HasNulls()  const;
    bool NoNulls()   const;

    void  SetLinks(FPlayerBrace* FoLastPtr, FPlayerBrace* FoNextPtr);
    void  SetLastLink(FPlayerBrace* FoLastPtr);
    void  SetNextLink(FPlayerBrace* FoNextPtr);

    auto& GetLastLink() const { return *MoLastBracePtr; }
    auto& GetNextLink() const { return *MoNextBracePtr; }

    auto GetPlayerID() const { return GetState().GetPlayerId(); }
};

template<typename P, typename C, typename S>
inline FPlayerBrace<P, C, S>::FPlayerBrace(S* FoStatePtr):
MoStatePtr(FoStatePtr)
{
    ensure(MoStatePtr);
    MoPawnPtr = MoStatePtr->GetPawn<P>();
    auto LoOwningController = MoStatePtr->GetOwningController();
    if (LoOwningController)
    MoControllerPtr = Cast<C>(LoOwningController);
    if (!MoControllerPtr && MoPawnPtr)
    MoControllerPtr = MoPawnPtr->GetController<C>();
}

template<typename P, typename C, typename S>
inline FPlayerBrace<P, C, S>::FPlayerBrace(FPlayerBrace&& Other):
    MoPawnPtr(Other.MoPawnPtr), 
    MoControllerPtr(Other.MoControllerPtr), 
    MoStatePtr(Other.MoStatePtr)
{
}

template<typename P, typename C, typename S>
inline FPlayerBrace<P, C, S>::FPlayerBrace(const FPlayerBrace& Other) :
    MoPawnPtr(Other.MoPawnPtr),
    MoControllerPtr(Other.MoControllerPtr),
    MoStatePtr(Other.MoStatePtr)
{
}

template<typename P, typename C, typename S>
inline void FPlayerBrace<P, C, S>::operator=(FPlayerBrace&& Other)
{
    MoPawnPtr = Other.MoPawnPtr;
    MoControllerPtr = Other.MoControllerPtr;
    MoStatePtr = Other.MoStatePtr;
}

template<typename P, typename C, typename S>
inline void FPlayerBrace<P, C, S>::operator=(const FPlayerBrace& Other)
{
    MoPawnPtr = Other.MoPawnPtr;
    MoControllerPtr = Other.MoControllerPtr;
    MoStatePtr = Other.MoStatePtr;
}

template<typename P, typename C, typename S>
inline P& FPlayerBrace<P, C, S>::GetPawn()
{
    ensure(MoPawnPtr);
    return *MoPawnPtr;
}

template<typename P, typename C, typename S>
inline C& FPlayerBrace<P, C, S>::GetController()
{
    ensure(MoControllerPtr);
    return *MoControllerPtr;
}

template<typename P, typename C, typename S>
inline S& FPlayerBrace<P, C, S>::GetState()
{
    ensure(MoStatePtr);
    return *MoStatePtr;
}

template <typename P, typename C, typename S>
const P& FPlayerBrace<P, C, S>::GetPawn() const
{
    ensure(MoPawnPtr);
    return *MoPawnPtr;
}

template <typename P, typename C, typename S>
const C& FPlayerBrace<P, C, S>::GetController() const
{
    ensure(MoControllerPtr);
    return *MoControllerPtr;
}

template <typename P, typename C, typename S>
const S& FPlayerBrace<P, C, S>::GetState() const
{
    ensure(MoStatePtr);
    return *MoStatePtr;
}

template<typename P, typename C, typename S>
inline bool FPlayerBrace<P, C, S>::operator!() const
{
    // controller not included in compare because
    // because only the authority and the owner will have a controller
    // but everyone will want to be able to use the Player Brace
    if (!MoPawnPtr)
        return true;
    if (!MoStatePtr)
        return true;
    return false;
}


template<typename P, typename C, typename S>
bool FPlayerBrace<P, C, S>::HasNulls() const
{
    if (MoPawnPtr)
        return true;
    if (MoControllerPtr)
        return true;
    if (MoStatePtr)
        return true;

    return false;
}

template<typename P, typename C, typename S>
bool FPlayerBrace<P, C, S>::NoNulls() const
{
    return !HasNulls();
}

template<typename P, typename C, typename S>
inline void FPlayerBrace<P, C, S>::SetLinks(FPlayerBrace* FoLastPtr, FPlayerBrace* FoNextPtr)
{
    MoLastBracePtr = FoLastPtr;
    MoNextBracePtr = FoNextPtr;
}

template<typename P, typename C, typename S>
inline void FPlayerBrace<P, C, S>::SetLastLink(FPlayerBrace* FoLastPtr)
{
    MoLastBracePtr = FoLastPtr;
}

template<typename P, typename C, typename S>
inline void FPlayerBrace<P, C, S>::SetNextLink(FPlayerBrace* FoNextPtr)
{
    MoNextBracePtr = FoNextPtr;
}


