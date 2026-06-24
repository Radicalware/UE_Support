#pragma once

#include "CoreMinimal.h"
#include <regex>
#include "Macros.h"
#include "GameFramework/Actor.h"
#include "Tracker.generated.h"

USTRUCT()
struct FTrace
{
    GENERATED_BODY()
    UPROPERTY() FTimerHandle Handle;
    FTimerDelegate Delegate;
    INL FTrace(){}
    void operator=(const FTrace& Other){ Handle = Other.Handle; Delegate = Other.Delegate; }
    INL FTrace(const FTrace& Other) { *this = Other;}
};

UCLASS()
class ATracker : public AActor
{
    GENERATED_BODY()
    static bool bIsExiting;
    static FTimerHandle SoHandle;
    static FTimerDelegate SoDelegate;
public:
    UPROPERTY() FTimerHandle MoHandle;
    FTimerDelegate Delegate;
    static const std::regex MoRegexClassAndColons;

    UPROPERTY() TObjectPtr<UObject> User = nullptr;
    UPROPERTY() FString  ObjectName = ""; // Optional for DBG
    UPROPERTY() float    LoopInSeconds = 1.0;
    UPROPERTY() bool     bRepeateLoop = false;
    UPROPERTY() float    FirstDelay = -1.f;

    std::atomic<bool> bWaiting = false; // Used for external tracking (optional use case)
    // Used for recalling the same function but returning at different Idx within that function
    // Basically use this if you want a "delay" in the middle of your function
    UPROPERTY() uint8    Idx = 0;
    
    const bool bDbgMode = true; // true false

    void SetObjectName(const FString& InObjectName);
    UFUNCTION(Server, Reliable) void ServerDbgExitAll();
    UFUNCTION() void DbgExit(const FName& FunctionName, const uint32 LineMacro, const FString& ErrorMessage);
    
    void StartLoop(UWorld* WorldPtr, const char* FunctionName, const uint32 LineMacro);

    template <typename FF, typename... RR>
    INL void StartLoop(UWorld* WorldPtr, const char* FunctionName, const uint32 LineMacro, FF&& Frist, RR&& ...Rest);

    // Note: Boomerange is intentional while StartLoop is called from exceptions
    void Slingshot(UWorld* WorldPtr, const char* FunctionName);

    template <typename FF, typename... RR>
    INL void Slingshot(UWorld* WorldPtr, const char* FunctionName, FF&& Frist, RR&& ...Rest);

    template <typename FF, typename... RR>
    INL void static StaticSlingshot(UWorld* WorldPtr, FF&& Frist, RR&& ...Rest);

    template <typename FF, typename... RR>
    INL static void DelayAction(UWorld* WorldPtr, UObject* User, const char* FunctionName, float DelaySecs, FF&& Frist, RR&& ...Rest);
    
    static void DelayAction(UWorld* WorldPtr, UObject* User, const char* FunctionName, float DelaySecs = 0.1);

    bool StopLoop(UWorld* WorldPtr);
    bool StopLoop(UWorld* WorldPtr, const char* FromClassName, const uint32 LineNum);
};

template<typename FF, typename ...RR>
INL void ATracker::StartLoop(UWorld* WorldPtr, const char* FunctionName, const uint32 LineMacro, FF&& Frist, RR&& ...Rest)
{
    GET(LoWorld, WorldPtr, void());
    NullEnsure(User, void());
    if (!ensure(strlen(FunctionName) != 0))
        return;

    auto LsFunctionName = std::regex_replace(FunctionName, MoRegexClassAndColons, "");
    Delegate.BindUFunction(User, LsFunctionName.c_str(), Frist, Rest...);
    if (LoopInSeconds < FirstDelay)
        LoopInSeconds = FirstDelay;
    LoWorld.GetTimerManager().ClearTimer(MoHandle);
    LoWorld.GetTimerManager().SetTimer(MoHandle, Delegate, LoopInSeconds, bRepeateLoop, FirstDelay);
}

template<typename FF, typename ...RR>
INL void ATracker::Slingshot(UWorld* WorldPtr, const char* FunctionName, FF&& Frist, RR&& ...Rest)
{
    GET(LoWorld, WorldPtr, void());
    NullEnsure(User, void());
    if (!ensure(strlen(FunctionName) != 0))
        return;
    auto LsFunctionName = std::regex_replace(FunctionName, MoRegexClassAndColons, "");
    Delegate.BindUFunction(User, LsFunctionName.c_str(), Frist, Rest...);
    if (LoopInSeconds < FirstDelay)
        LoopInSeconds = FirstDelay;
    LoWorld.GetTimerManager().ClearTimer(MoHandle);
    LoWorld.GetTimerManager().SetTimer(MoHandle, Delegate, LoopInSeconds, false);
}

template<typename FF, typename ...RR>
INL void ATracker::StaticSlingshot(UWorld* WorldPtr, FF&& Frist, RR&& ...Rest)
{
    GET(LoWorld, WorldPtr, void());
    SoDelegate = FTimerDelegate::CreateStatic(std::forward<FF>(Frist), std::forward<RR>(Rest)...);
    LoWorld.GetTimerManager().ClearTimer(SoHandle);
    LoWorld.GetTimerManager().SetTimer(SoHandle, SoDelegate, 1.0f, false);
}

template<typename FF, typename ...RR>
INL void ATracker::DelayAction(UWorld* WorldPtr, UObject* User, const char* FunctionName, float DelaySecs, FF&& Frist, RR&& ...Rest)
{
    GET(LoWorld, WorldPtr, void());
    NullEnsure(User, void());
    if (!ensure(strlen(FunctionName) != 0))
        return;

    FTrace Trace;
    auto LsFunctionName = std::regex_replace(FunctionName, MoRegexClassAndColons, "");
    Trace.Delegate.BindUFunction(User, LsFunctionName.c_str(), std::forward<FF>(Frist), std::forward<RR>(Rest)...);
    LoWorld.GetTimerManager().SetTimer(Trace.Handle, Trace.Delegate, DelaySecs, false);
}

