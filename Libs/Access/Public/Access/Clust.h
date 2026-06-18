#pragma once

#include "CoreMinimal.h"
#include "Macros.h"
#include <utility>

#define TFirstRest     template <typename F, typename... R>
#define TElemFirstRest template <typename E, typename F, typename... R>
#define TElemFirst     template <typename E, typename F>
#define TMapFirst      template <typename M, typename  FF>
#define TMapFirstRest  template <typename M, typename FF, typename ...RR>

class Clust
{
public:
    TElemFirst      static void      AddElements(TArray<E*>& Array, F* First);
    TElemFirst      static void      AddElements(TArray<E >& Array, F&& First);

    TElemFirstRest  static void      AddElements(TArray<E*>& Array, F*  First, R&&... Rest);
    TElemFirstRest  static void      AddElements(TArray<E >& Array, F&& First, R&&... Rest);

    TFirstRest      static TArray<F> NewArray(F&& First, R&&... Rest);


    TMapFirst      static void       AddPairs(M& Map, FF&& FirstPair);
    TMapFirstRest  static void       AddPairs(M& Map, FF&& FirstPair, RR&&... Rest);

    template<typename K, typename  V> static TPair<K, V>  MakePair(K&& Key, V&& Value);


    template <typename T, typename I>
    static T& GetIdx(TArray<T> Array, I Idx);
    template <typename T, typename I>
    static T* GetIdx(TArray<T*> Array, I Idx);
};


template <typename E, typename F>
void Clust::AddElements(TArray<E*>& Array, F* First)
{
    Array.Add(Cast<E>(First));
}

template <typename E, typename F>
void Clust::AddElements(TArray<E>& Array, F&& First)
{
    Array.Add(First);
}


template <typename E, typename F, typename ... R>
void Clust::AddElements(TArray<E*>& Array, F* First, R&&... Rest)
{
    AddElements(Array, Cast<E>(First));
    AddElements(Array, std::forward<R>(Rest)...);
}

template <typename E, typename F, typename ... R>
void Clust::AddElements(TArray<E>& Array, F&& First, R&&... Rest)
{
    AddElements(Array, std::forward<F>(First));
    AddElements(Array, std::forward<R>(Rest)...);
}

template <typename F, typename ... R>
TArray<F> Clust::NewArray(F&& First, R&&... Rest)
{
    TArray<F> Array;
    AddElements(Array, std::forward<F>(First), std::forward<R>(Rest)...);
    return Array;
}

template<typename M, typename FF>
inline void Clust::AddPairs(M& Map, FF&& FirstPair)
{
    Map.Add(FirstPair);
}

template<typename M, typename FF, typename ...RR>
inline void Clust::AddPairs(M& Map, FF&& FirstPair, RR&& ...Rest)
{
    AddPairs(Map, std::forward<FF>(FirstPair));
    AddPairs(Map, std::forward<RR>(Rest)...);
}

template<typename K, typename V>
inline TPair<K, V> Clust::MakePair(K&& Key, V&& Value)
{
    return TPair<K, V>(Key, Value);
}

template <typename T, typename I>
T& Clust::GetIdx(TArray<T> Array, I Idx)
{
    if (Idx >= Array.Num())
        throw FString(TEXT("Clust::GetIdx(TArray<T> Array, D Idx) >> Array Idx is out of range!!"));
    return Array[Idx];
}

template <typename T, typename I>
T* Clust::GetIdx(TArray<T*> Array, I Idx)
{
    if (Idx >= Array.Num())
        throw FString(TEXT("Clust::GetIdx(TArray<T> Array, D Idx) >> Array Idx is out of range!!"));
    ensure(Array[Idx]);
    return Array[Idx];
}
