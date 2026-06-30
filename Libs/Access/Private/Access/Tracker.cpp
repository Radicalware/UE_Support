// Copyright by Shepherd Dowling under the Apache v2 licence

#include "Access/Tracker.h"
#include "Access/Macros.h"
#include "Access/XF.h"
#include "Access/Debug.h"
#include "Async/Async.h"

#include "GameFramework/GameState.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Access/Color.h"

#include <regex>

bool ATracker::bIsExiting = false;
const std::regex ATracker::MoRegexClassAndColons = std::regex(R"((.*::)|(_Implementation))", RXM::ECMAScript);

void ATracker::ServerDbgExitAll_Implementation()
{
    GET(World, GetWorld()->GetGameState());
    for (auto& Player : UGameplayStatics::GetGameState(GetWorld())->PlayerArray)
        Player.Get()->GetPlayerController()->ConsoleCommand(TEXT("quit"));
}

void ATracker::SetObjectName(const FString& InObjectName)
{
    const std::regex Pattern(R"((::.*$))");
    const std::string ClassName = std::regex_replace(TCHAR_TO_ANSI(ToCStr(InObjectName)), Pattern, "");
    ObjectName = ANSI_TO_TCHAR(ClassName.c_str());
}


void ATracker::DbgExit(const FName& FunctionName, const uint32 LineMacro, const FString& ErrorMessage)
{
    const FString ErrorOut = SSS("Failed Function: ", ObjectName, "::", FunctionName, "::", LineMacro, " >> ", ErrorMessage, '\n');
    PrintE(ErrorOut);
    if (ATracker::bIsExiting)
        return;

    if (GIsEditor)
    {
        ExitIfEditor(void());
        UDebug::WriteErrorToFile(ErrorOut);
        UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, false);
        PrintE("Closing Clients!!");
        ATracker::bIsExiting = true;
    }
}


void ATracker::Slingshot(UWorld* WorldPtr, const char* FunctionName)
{
    GetRef(LoWorld, WorldPtr, void());
    NullEnsure(User, void());
    if (!ensure(strlen(FunctionName) != 0))
        return;

    auto LsFunctionName = std::regex_replace(FunctionName, MoRegexClassAndColons, "");
    Print("Slingshot: ", LsFunctionName);
    Delegate.BindUFunction(User, LsFunctionName.c_str());
    if (LoopInSeconds < FirstDelay)
        LoopInSeconds = FirstDelay;
    if (MoHandle.IsValid() && LoWorld.GetTimerManager().IsTimerActive(MoHandle))
        LoWorld.GetTimerManager().ClearTimer(MoHandle);
    LoWorld.GetTimerManager().SetTimer(MoHandle, Delegate, LoopInSeconds, false, FirstDelay);
}

void ATracker::StartLoop(UWorld* WorldPtr, const char* FunctionName, const uint32 LineMacro)
{
    GetRef(LoWorld, WorldPtr, void());
    NullEnsure(User, void());
    if (!ensure(strlen(FunctionName) != 0))
        return;

    auto LsFunctionName = std::regex_replace(FunctionName, MoRegexClassAndColons, "");

    Delegate.BindUFunction(User, LsFunctionName.c_str());
    if (LoopInSeconds < FirstDelay)
        LoopInSeconds = FirstDelay;
    if (MoHandle.IsValid() && LoWorld.GetTimerManager().IsTimerActive(MoHandle))
        LoWorld.GetTimerManager().ClearTimer(MoHandle);
    LoWorld.GetTimerManager().SetTimer(MoHandle, Delegate, LoopInSeconds, bRepeateLoop, FirstDelay);
}

void ATracker::DelayAction(UWorld* WorldPtr, UObject* User, const char* FunctionName, float DelaySecs)
{
    GetRef(LoWorld, WorldPtr, void());
    NullEnsure(User, void());
    if (!ensure(strlen(FunctionName) != 0))
        return;
    auto LsFunctionName = std::regex_replace(FunctionName, MoRegexClassAndColons, "");

    FTrace Trace;
    Trace.Delegate.BindUFunction(User, LsFunctionName.c_str());
    LoWorld.GetTimerManager().SetTimer(Trace.Handle, Trace.Delegate, DelaySecs, false, DelaySecs);
}

bool ATracker::StopLoop(UWorld* WorldPtr)
{
    GetRef(LoWorld, WorldPtr, false);
    if (MoHandle.IsValid() && LoWorld.GetTimerManager().IsTimerActive(MoHandle))
        LoWorld.GetTimerManager().ClearTimer(MoHandle);
    return true;
}

bool ATracker::StopLoop(UWorld* WorldPtr, const char* FromClassName, const uint32 LineNum)
{
    if (!WorldPtr)
    {
        PrintW("Failed to deref WorldPtr from ", FromClassName, ":", LineNum);
        return false;
    }
    GetRef(LoWorld, WorldPtr, false);
    // This will fail when in the editor mode and you save a widget
    // GetRef(World, WorldPtr, void(), "ATracker::StopLoop failed to deref WorldPtr");
    if (MoHandle.IsValid() && LoWorld.GetTimerManager().IsTimerActive(MoHandle))
        LoWorld.GetTimerManager().ClearTimer(MoHandle);
    return true;
}
