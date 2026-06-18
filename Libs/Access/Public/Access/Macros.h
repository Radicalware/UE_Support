#pragma once
#include "CoreMinimal.h"
#include "Templates/SharedPointerFwd.h"
#include <regex>
#ifdef _WIN32
 #define __CLASS__ __FUNCTION__
 #define __func__ __FUNCTION__
#else
 #define __CLASS__ __PRETTY_FUNCTION__
 #ifndef __func__
 #define __func__ __PRETTY_FUNCTION__
 #endif
#endif

enum class ESwitch
{
 On,
 Off
};

#define The (*this)
#define INL inline
#define TTT template<typename T>

#define MakeThreadPtr(__TYPE__) MakeShared<__TYPE__, ESPMode::ThreadSafe>()

#define RefWeak(__PTR__) (__PTR__.Pin().Get())

template<typename T>
using sp = TSharedPtr<T, ESPMode::ThreadSafe>;

template<typename T>
using wp = TWeakPtr<T>;


//------------- MULTI-ARGUMENT FUNCTION HANDLING --------------------------------------------------------------------------------------------------------------

#define _my_BUGFX(x) x

#define _my_NARG2(...) _my_BUGFX(_my_NARG1(__VA_ARGS__,_my_RSEQN()))
#define _my_NARG1(...) _my_BUGFX(_my_ARGSN(__VA_ARGS__))
#define _my_ARGSN(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,N,...) N
#define _my_RSEQN()9,8,7,6,5,4,3,2,1,0

#define _my_FUNC2(name,n) name ## n
#define _my_FUNC1(name,n) _my_FUNC2(name,n)
#define GET_MACRO(_FunctionName_,...) _my_FUNC1(_FunctionName_,_my_BUGFX(_my_NARG2(__VA_ARGS__))) (__VA_ARGS__)

//------------- DEBUGGING -------------------------------------------------------------------------------------------------------------------------------------

struct THEGAME_API FLogCategoryLocalLog
 : public FLogCategory<ELogVerbosity::Verbose, ELogVerbosity::All>
{
 FLogCategoryLocalLog();
};
extern THEGAME_API FLogCategoryLocalLog LocalLog;


#define SSS(...) XF::CombineToString(__VA_ARGS__)
#define Print(...) \
 { UE_LOG(LocalLog, Log, \
 TEXT(GREEN "%s >> %s" WHITE), \
 *XF::CombineToString(__CLASS__, ':', __LINE__), \
 *XF::CombineToString(__VA_ARGS__) \
 ); }

/// Print No Color
#define PrintNC(...) \
 { UE_LOG(LocalLog, Log, \
 TEXT("%s >> %s"), \
 *XF::CombineToString(__CLASS__, ':', __LINE__), \
 *XF::CombineToString(__VA_ARGS__) \
 ); }

#define PrintW(...) \
 { UE_LOG(LocalLog, Warning, TEXT("WARNING: (%s:%s) >> %s"), *FString(__CLASS__), *FString::FromInt(__LINE__), *XF::CombineToString(__VA_ARGS__)); }

#define PrintE(...) \
 { UE_LOG(LocalLog, Error, TEXT("ERROR: (%s:%s) >> %s"), *FString(__CLASS__), *FString::FromInt(__LINE__), *XF::CombineToString(__VA_ARGS__)); }

#define PrintErrNoTail(...) \
 { UE_LOG(LocalLog, Error, TEXT("ERROR: %s"), *XF::CombineToString(__VA_ARGS__)); }

#define PrintLine() UE_LOG(LocalLog, Log, TEXT(GREEN "--------------------------------------------------" WHITE));

#define PrintCatchMessage(...) GET_MACRO(PrintCatchMessage, void(), ##__VA_ARGS__)
#define PrintCatchMessage0(VD) \
 PrintLine() \
 UE_LOG(LocalLog, Log, TEXT(GREEN "XXX: (%s:%s) >> %s" WHITE), *FString(__CLASS__), *FString::FromInt(__LINE__), *FString(LsError));
#define PrintCatchMessage1(VD,__PREFIX_MESSAGE__) \
 PrintLine() \
 Print(__PREFIX_MESSAGE__) \
 UE_LOG(LocalLog, Log, TEXT(GREEN "(%s:%s) >> %s" WHITE), *FString(__CLASS__), *FString::FromInt(__LINE__), *FString(LsError));

#define PrintErrorLine() UE_LOG(LocalLog, Warning, TEXT(" ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR "));


#define PrintStart() Print("STARTING >> ", FString(__CLASS__));
#define PrintFinish() Print("FINISHING >> ", FString(__CLASS__));

#define PrintBool(__DESC__, __NUM__) Print(FString(__DESC__) + (__NUM__ ? "True" : "False"))

//------------- ERROR HANDLING --------------------------------------------------------------------------------------------------------------------------------
// TODO: Make all macros follow these3 mechanics
// Get -- normal with throw
// GetCheck -- normal but with easy return
// GetEnsure -- like check but wrapped with an ensure macro


#define NullThrow(...) GET_MACRO(NullThrow, void(), ##__VA_ARGS__)
#define NullThrow1(VD, __PARAM__) if (!__PARAM__) { throw BBB("Null Object Thrown, \"" #__PARAM__ "\""); }
#define NullThrow2(VD, __PARAM__,__ERROR_STR__) if (!__PARAM__) \
{ throw throw SSS("Null Object Thrown, \"" ## #__PARAM__ "\" >>> " __ERROR_STR__); }

#define NullCheck(...) GET_MACRO(NullCheck, void(), ##__VA_ARGS__)
#define NullCheck1(VD, __PARAM__) if (!__PARAM__) { return; }
#define NullCheck2(VD, __PARAM__, __RET_TYPE__) if (!__PARAM__) { return __RET_TYPE__ ; }
#define NullCheck3(VD, __PARAM__, __RET_TYPE__, __ERROR_MESSAGE___) if (!__PARAM__) { PrintW(__ERROR_MESSAGE___); return __RET_TYPE__ ; }

#define NullEnsure(...) GET_MACRO(NullEnsure, void(), ##__VA_ARGS__)
#define NullEnsure1(VD, __PARAM__) if (!ensure(__PARAM__)) { return; }
#define NullEnsure2(VD, __PARAM__, __RET_TYPE__) if (!ensure(__PARAM__)) { return __RET_TYPE__ ; }
#define NullEnsure3(VD, __PARAM__, __RET_TYPE__, __ERROR_MESSAGE___) if (!ensure(__PARAM__)) { PrintW(__ERROR_MESSAGE___); return __RET_TYPE__ ; }

// Big Bad Buuug
#define BBB(...) \
 UDebug::WriteErrorToFile(SSS("(", __CLASS__, ":", __LINE__, ") >> ", __VA_ARGS__));

#define ThrowIt(...) \
{ \
 auto LsError = SSS("(", __CLASS__, ":", __LINE__, ") >> ", __VA_ARGS__); \
 PrintE(LsError); \
 throw UDebug::WriteErrorToFile(LsError); \
}

//------------- SETTERS ---------------------------------------------------------------------------------------------------------------------------------------

#define SetTheOwningPlayerState(...) GET_MACRO(SetTheOwningPlayerState, void(), ##__VA_ARGS__)
#define SetTheOwningPlayerState0(VD) \
 ThePlayerStatePtr = GetOwningPlayerState<AThePlayerState>(); \
 NullThrow1(VD, ThePlayerStatePtr); \
 AThePlayerState& ThePlayerState = *ThePlayerStatePtr;
#define SetTheOwningPlayerState1(VD, __RET_TYPE__) \
 ThePlayerStatePtr = GetOwningPlayerState<AThePlayerState>(); \
 NullEnsure2(VD, ThePlayerStatePtr, __RET_TYPE__); \
 AThePlayerState& ThePlayerState = *ThePlayerStatePtr;

#define SetTheGameState(...) GET_MACRO(SetTheGameState, void(), ##__VA_ARGS__)
#define SetTheGameState0(VD) \
 GSPtr = GetWorld()->GetGameState<ATheGameState>(); \
 NullThrow1(VD, GSPtr); \
 ATheGameState& GS = *GSPtr;
#define SetTheGameState1(VD, __RET_TYPE__) \
 GSPtr = GetWorld()->GetGameState<ATheGameState>(); \
 NullEnsure2(VD, GSPtr, __RET_TYPE__); \
 ATheGameState& GS = *GSPtr;

//------------- GETTERS ---------------------------------------------------------------------------------------------------------------------------------------

// Just a duplicate from the one above, I figure it is just understood what would be going on

#define SET(...) GET_MACRO(SET, void(), ##__VA_ARGS__)
#define SET2(VD,__VARNAME__,__PARAM__) \
 __VARNAME__##Ptr = __PARAM__; \
 if(!(__VARNAME__##Ptr)) { \
    throw BBB("Null Object Thrown, \"" # __PARAM__ "\" In Class \"" __CLASS__ "\""); \
 } \
 auto& __VARNAME__ = *__VARNAME__##Ptr;

#define SET3(VD,__VARNAME__,__MEMBER_NAME__,__PARAM__) \
 __MEMBER_NAME__ = __PARAM__; \
 if(!(__MEMBER_NAME__)) { \
    throw BBB("Null Object Thrown, \"" ## #__PARAM__ "\" In Class \"" __CLASS__ "\""); \
 } \
 auto& __VARNAME__ = *__MEMBER_NAME__;

#define SET4(VD,__VARNAME__,__MEMBER_NAME__,__PARAM__, __RETURN__) \
 __MEMBER_NAME__ = __PARAM__; \
 if(!__MEMBER_NAME__) { \
    return __RETURN__; \
 } \
auto& __VARNAME__ = *__MEMBER_NAME__;
// ------------------------------------------------------------------------------------------------------------------
#define GetRef(...) GET_MACRO(GetRef, void(), ##__VA_ARGS__)
// Same as GetRef2(VarName, PtrVarName);

#define GetRef1(VD,__VARNAME__) \
 if(!(__VARNAME__##Ptr != nullptr)) { \
    throw BBB("Null Object Thrown, \"" #__VARNAME__ "Ptr\" In Class \"" __CLASS__ "\""); \
 } \
 auto& __VARNAME__ = *__VARNAME__##Ptr;

//2 Args throws exception
#define GetRef2(VD,__VARNAME__,__PARAM__) \
 auto __VARNAME__##Ptr = __PARAM__; \
 if(!ensure(__VARNAME__##Ptr != nullptr)) { \
    throw BBB("Null Object Thrown, \"" #__PARAM__ "\" In Class \"" __CLASS__ "\""); \
 } \
 auto& __VARNAME__ = *__VARNAME__##Ptr;

//3 Args returns type
#define GetRef3(VD,__VARNAME__,__PARAM__,__RETURN__) \
 auto __VARNAME__##Ptr = __PARAM__; \
 if(!ensure(__VARNAME__##Ptr != nullptr)) { \
    return __RETURN__; \
 } \
 auto& __VARNAME__ = *__VARNAME__##Ptr;

//4 Args returns type with reason for issue
#define GetRef4(VD,__VARNAME__,__PARAM__,__RETURN__,__MESSAGE__) \
 auto __VARNAME__##Ptr = __PARAM__; \
 if(!ensure(__VARNAME__##Ptr != nullptr)) { \
     PrintW(__MESSAGE__); \
     return __RETURN__; \
 } \
 auto& __VARNAME__ = *__VARNAME__##Ptr;

// ------------------------------------------------------------------------------------------------------------------
#define GetCRef(...) GET_MACRO(GetCRef, void(), ##__VA_ARGS__)
// Same as GetCRef2(VarName, PtrVarName);
#define GetCRef1(VD,__VARNAME__) \
 if(!(__VARNAME__##Ptr != nullptr)) { \
    throw BBB("Null Object Thrown, \"" #__VARNAME__ "Ptr\" In Class \"" __CLASS__ "\""); \
 } \
 const auto& __VARNAME__ = *__VARNAME__##Ptr;

//2 Args throws exception
#define GetCRef2(VD,__VARNAME__,__PARAM__) \
 const auto __VARNAME__##Ptr = __PARAM__; \
 if(!ensure(__VARNAME__##Ptr != nullptr)) { \
    throw BBB("Null Object Thrown, \"" #__PARAM__ "\" In Class \"" __CLASS__ "\""); \
 } \
 const auto& __VARNAME__ = *__VARNAME__##Ptr;

//3 Args returns type
#define GetCRef3(VD,__VARNAME__,__PARAM__,__RETURN__) \
 const auto __VARNAME__##Ptr = __PARAM__; \
 if(!ensure(__VARNAME__##Ptr != nullptr)) { \
    return __RETURN__; \
 } \
 const auto& __VARNAME__ = *__VARNAME__##Ptr;

//4 Args returns type with reason for issue
#define GetCRef4(VD,__VARNAME__,__PARAM__,__RETURN__,__MESSAGE__) \
 const auto __VARNAME__##Ptr = __PARAM__; \
 if(!ensure(__VARNAME__##Ptr != nullptr)) { \
     PrintW(__MESSAGE__); \
     return __RETURN__; \
 } \
 const auto& __VARNAME__ = *__VARNAME__##Ptr;


// ----------------------------------------------
// Get
#define GET(...) GetRef(__VA_ARGS__);
// Get as Constant Reference
#define CGET(...) GetCRef(__VA_ARGS__);




#define GetWeak(...) GET_MACRO(GetWeak, void(), ##__VA_ARGS__)
//1 Get reference
#define GetWeak1(VD,__VARNAME__) \
 if(__VARNAME__##Wk == nullptr || __VARNAME__##Wk.IsValid() == false) { \
 throw BBB("Null Object Thrown, \"" #__VARNAME__ "Wk\" In Class \"" __CLASS__ "\""); \
 } \
 auto& __VARNAME__ = *__VARNAME__##Wk.Pin().Get();
//2 Args throws exception
#define GetWeak2(VD,__VARNAME__,__PARAM__) \
 auto __VARNAME__##Wk = __PARAM__; \
 if(!ensure(__VARNAME__##Wk != nullptr && __VARNAME__##Wk.IsValid())) { \
 throw BBB("Null Object Thrown, \"" #__PARAM__ "\" In Class \"" __CLASS__ "\""); \
 } \
 auto& __VARNAME__ = *__VARNAME__##Wk.Pin().Get();

//3 Args returns type
#define GetWeak3(VD,__VARNAME__,__PARAM__,__RETURN__) \
 auto __VARNAME__##Wk = __PARAM__; \
 if(!ensure(__VARNAME__##Wk != nullptr && __VARNAME__##Wk.IsValid())) { \
 return __RETURN__; \
 } \
 auto& __VARNAME__ = *__VARNAME__##Wk.Pin().Get();



#define GetWeakSafe(...) GET_MACRO(GetWeakSafe, void(), ##__VA_ARGS__)
#define GetWeakSafe1(VD,__VARNAME__) \
 auto __VARNAME__##_Wk = __VARNAME__##Wk; \
 if(__VARNAME__##_Wk == nullptr || __VARNAME__##_Wk.IsValid() == false) { \
 return void(); \
 } \
 auto& __VARNAME__ = *__VARNAME__##_Wk.Pin().Get();

#define GetWeakSafe2(VD,__VARNAME__,__PARAM__) \
 auto __VARNAME__##_Wk = __PARAM__; \
 if(__VARNAME__##_Wk == nullptr || __VARNAME__##_Wk.IsValid() == false) { \
 return void(); \
 } \
 auto& __VARNAME__ = *__VARNAME__##_Wk.Pin().Get();


#define GetWeakSafe3(VD,__VARNAME__,__PARAM__,__RETURN__) \
 auto __VARNAME__##_Wk = __PARAM__; \
 if(__VARNAME__##_Wk == nullptr || __VARNAME__##_Wk.IsValid() == false) { \
 return __RETURN__; \
 } \
 auto& __VARNAME__ = *__VARNAME__##_Wk.Pin().Get();

// -------------------------------------------------------------------------------------------------------------------------------------------------------------
// Game Mode
// Only works when you are on the server

#define GetTheAuthGameMode() \
 ATheGameMode* TheGameModePtr = GetWorld()->GetAuthGameMode<ATheGameMode>(); \
 if(!TheGameModePtr) \
 throw BBB("GetWorld()->GetAuthGameMode<ATheGameMode>()"); \
 ATheGameMode& TheGameMode = *TheGameModePtr;

// -------------------------------------------------------------------------------------------------------------------------------------------------------------
// Controller

#define GetControllerFromOwningLocalPlayer \
 if (!ensure(Cast<AHoldemController>(GetOwningLocalPlayer()->GetPlayerController(GetWorld())))) \
 return; \
 AHoldemController& Controller = *Cast<AHoldemController>(GetOwningLocalPlayer()->GetPlayerController(GetWorld()));

// -------------------------------------------------------------------------------------------------------------------------------------------------------------
// Player State & Game State Control

// #define GetThePlayerStates(...) GET_MACRO(GetThePlayerStates, void(), __VA_ARGS__)
// #define GetThePlayerStates0(VD) \
// GetTheGameState0(VD); \
// TArray<AThePlayerState*> ThePlayerStates = GS.Rock().GetOrderedPlayerStates();
// #define GetThePlayerStates1(VD, __RET__) \
// GetTheGameState1(VD,__RET__); \
// TArray<AThePlayerState*> ThePlayerStates = GS.Rock().GetOrderedPlayerStates();

#define GetDoubleThePlayerStates() \
 GetTheGameState(); \
 TArray<AThePlayerState*> ThePlayerStates = GS.Rock().GetOrderedPlayerStates(); \
 const TArray<AThePlayerState*> PlayerStatesSingle = ThePlayerStates; \
 ThePlayerStates += PlayerStatesSingle;

#define LoopThePlayerStates() \
 for(AThePlayerState* ThePlayerStatePtr : ThePlayerStates)

#define LoopBraces() \
 for(auto& Pair : MmBraces)

#define LoopBraces1() \
 for(auto& Pair1 : MmBraces)

#define LoopBraces2() \
 for(auto& Pair2 : MmBraces)

#define LoopTheOtherPlayerStates() \
 for(AThePlayerState* TheOtherPlayerStatePtr : ThePlayerStates)

#define GetAndLoopThePlayerStates() \
 GetThePlayerStates(); \
 for(AThePlayerState* ThePlayerStatePtr : ThePlayerStates)

#define ForeachPlayerState(__ACTION__) \
 LoopBraces() \
 { \
 RefState(); \
 LoState.__ACTION__; \
 }

#define RefThePlayerState(...) GET_MACRO(RefThePlayerState, void(), ##__VA_ARGS__)
#define RefThePlayerState0(VD) \
 GetRef1(VD, ThePlayerState);
#define RefThePlayerState1(VD, __RET__) \
 GetRef3(VD, ThePlayerState, ThePlayerStatePtr, __RET__);

#define GetBraces() \
 GS.LocalUpdatePlayerBraces(); \
 OMap<int32, FHoldemBrace>& MmBraces = GS.GetPlayers();

#define RefState() \
 auto& LoBrace = Pair.second; \
 auto& LoState = LoBrace.GetState();

#define RefState1() \
 auto& LoBrace1 = Pair1.second; \
 auto& LoState1 = LoBrace1.GetState();

#define RefState2() \
 auto& LoBrace2 = Pair2.second; \
 auto& LoState2 = LoBrace2.GetState();

#define RefTheOtherPlayerState(...) GET_MACRO(RefTheOtherPlayerState, void(), ##__VA_ARGS__)
#define RefTheOtherPlayerState0(VD) \
 GetRef1(VD, TheOtherPlayerState);
#define RefTheOtherPlayerState1(VD, __RET__) \
 GetRef3(VD, TheOtherPlayerState, TheOtherPlayerStatePtr, __RET__);

#define GetTheGameState(...) GET_MACRO(GetTheGameState, void(), ##__VA_ARGS__)
#define GetTheGameState0(VD) \
 NullThrow1(VD,GetWorld()); \
 ATheGameState* GSPtr = GetWorld()->GetGameState<ATheGameState>(); \
 if(!GSPtr) \
 throw BBB("GetWorld()->GetGameState<ATheGameState>()"); \
 ATheGameState& GS = *GSPtr;
#define GetTheGameState1(VD,__RET__) \
 NullEnsure2(VD, GetWorld(), __RET__); \
 ATheGameState* GSPtr = GetWorld()->GetGameState<ATheGameState>(); \
 if(!GSPtr) \
 return __RET__; \
 ATheGameState& GS = *GSPtr;


#define BxHasGameState() \
 (GetWorld()->GetGameState<ATheGameState>() != nullptr)

// Can't take VA args because GetTheGameState takses specifically1 or2 depending on inputs
// so you will need to make GetGS1 & GetGS2 and then merge to GetGS
#define GetGS() \
 GetTheGameState()
#define GetGamePlayers() \
 GetTheGameState(); \
 GetBraces();

#define GetThePlayerState() \
 AThePlayerState* ThePlayerStatePtr = GetPlayerState<AThePlayerState>(); \
 if(!ThePlayerStatePtr) \
 throw BBB("GetWorld()->GetPlayerState<AThePlayerState>()"); \
 AThePlayerState& ThePlayerState = *ThePlayerStatePtr;

#define GetThePlayerStateFromOwner() \
 AThePlayerState* ThePlayerStatePtr = GetOwningPlayerState<AThePlayerState>(); \
 if(!ThePlayerStatePtr) \
 throw BBB("GetWorld()->GetPlayerState<AThePlayerState>()"); \
 AThePlayerState& ThePlayerState = *ThePlayerStatePtr;

// -------------------------------------------------------------------------------------------------------------------------------------------------------------
// Array Handling

#define xRock GS.Rock()
#define xDealers GS.GetDealers()
#define xDealer GS.GetDealer(MnTableID)
#define xDealerAtTable(__IDX__) GS.GetDealer(__IDX__)
#define xCurrentDealer xDealerAtTable(GetCurrentTableID())
#define xCurrentSeat ThePlayerState.GetCurrentSeat()

// -------------------------------------------------------------------------------------------------------------------------------------------------------------
// Array Handling

#define ThrowOutOfRange(...) GET_MACRO(ThrowOutOfRange, void(), ##__VA_ARGS__)
#define ThrowOutOfRange2(VD,__ARRAY__, __IDX__) \
 if!( __IDX__ < static_cast<int32>(__ARRAY__.Num()))) \
 throw BBB("\n", #__ARRAY__, ".Num() = ", __ARRAY__.Num(), \
 "\n", #__IDX__, " = ", __IDX__);
#define ThrowOutOfRange3(VD,__ARRAY__, __IDX__,__MESSAGE__) \
 if!( __IDX__ < static_cast<int32>(__ARRAY__.Num()))) \
 throw BBB("\n", #__ARRAY__, ".Num() = ", __ARRAY__.Num(), \
 "\n", #__IDX__, " = ", __IDX__, "\n", __MESSAGE__);

#define MSetElement(__ARRAY__,__IDX__,__ELEMENT__) \
 ThrowOutOfRange(__ARRAY__, __IDX__) \
 __ARRAY__[__IDX__] = __ELEMENT__;


#define MSetVariableFromIdx(__VAR__,__ARRAY__,__IDX__) \
 ThrowOutOfRange(__ARRAY__, __IDX__) \
 __VAR__ = __ARRAY__[__IDX__];

// -------------------------------------------------------------------------------------------------------------------------------------------------------------
// Place this around code that you want to loop until it works
// You InitNetSource consider adding a2nd argument (lambda) that res-ets values if needed

// ErrorLooperPtr = CreateDefaultSubobject<UTracker>("ErrorLooper");
// NullEnsure(ErrorLooperPtr); \

inline bool UsingEditor()
{
#if WITH_EDITOR
 return true;
#else
 return false;
#endif
}

// SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn; \
// ErrorLooperPtr = GetWorld()->SpawnActor<ATracker>(SpawnParameters); \


#define InitTracker(__TRACKER_NAME__) \
    if(!__TRACKER_NAME__##Ptr){ \
        __TRACKER_NAME__##Ptr = NewObject<ATracker>(this, ATracker::StaticClass()); \
    if(__TRACKER_NAME__##Ptr){ \
        __TRACKER_NAME__##Ptr->User = this; \
        __TRACKER_NAME__##Ptr->SetObjectName(__CLASS__); \
    }else \
        Print("Could Not Instantiate ErrorLooper"); \
    }
#define InitErrorTracker() InitTracker(ErrorTracker);

#define GetTracker(__TRACKER_NAME__) \
    InitTracker(__TRACKER_NAME__) \
    GET(__TRACKER_NAME__)

#define GetErrorTracker() \
    InitTracker(ErrorTracker) \
    GET(ErrorTracker)

#define InitNetSource() InitTracker(ErrorTracker);

#define Boomerang(__RET_TYPE__, ...) { \
 ATracker::DelayAction(GetWorld(), this, __CLASS__,0.1, ## __VA_ARGS__); \
 return __RET_TYPE__; \
 }

#define INIT_BODY() \
    UPROPERTY() class ATracker* ErrorTrackerPtr = nullptr;


#define HandleCatch(...) \
 if(!ErrorLooperPtr) { InitNetSource() } \
 ErrorLooperPtr->User = this; \
 ErrorLooperPtr->Slingshot(GetWorld(), __CLASS__, ## __VA_ARGS__); 

#define StartNet() \
 try \
 {

#define EndNet(...) \
 }catch(const FString& LsError) \
 { \
 PrintCatchMessage(); \
 HandleCatch(); \
 }catch(const char* LsError) \
 { \
 PrintCatchMessage(); \
 HandleCatch(); \
 }catch(...) \
 { \
 PrintW("Unknown Error"); \
 HandleCatch(); \
 }


#define CUSTOM_BODY() \
 InitNetHeader();

#define CustomBeginPlay(...) GET_MACRO(CustomBeginPlay, void(), ##__VA_ARGS__)
#define CustomBeginPlay0(VD) \
 InitNetSource();
#define CustomBeginPlay1(VD, __RET_TYPE__) \
 InitNetSource();


namespace RXM {
 using namespace std::regex_constants;
 using Type = syntax_option_type;
}

// -------------------------------------------------------------------------------------------------------------------------------------------------------------
// Misc

#define GetRock() \
 NullThrow1(void(), RockPtr); \
 URock& Rock = *RockPtr;

#define GetWorldRef(...) GET_MACRO(GetWorldRef, void(), ##__VA_ARGS__)
#define GetWorldRef0(VD) \
 GetRef2(VD,World, GetWorld());
#define GetWorldRef1(VD, __RET__) \
 NullCheck2(VD,GetWorld(), __RET__); \
 GetRef3(VD,World, GetWorld(),__RET__);

#define ExitIfEditor(__PARAM__) if(GIsEditor && !GIsPlayInEditorWorld) return __PARAM__;
#define IsInEditor GIsEditor && !GIsPlayInEditorWorld

#define Eto8(__PARAM__) static_cast<uint8>(__PARAM__)

#define ComponentHasAuthority (GetOwnerRole() == ROLE_Authority)

// GetNetMode() == NM_DedicatedServer

#define MReturnOnServer() \
 if(GetNetMode() == NM_DedicatedServer) return;

// Must Be The Servers
#define MBxServer() \
 (GetNetMode() == NM_Standalone \
 || GetNetMode() == NM_DedicatedServer \
 || GetNetMode() == NM_ListenServer)

// Can be Autonomous or Simulated Proxy (Player Clients)
#define MBxOnClient() \
 (GetNetMode() == NM_Client)

#define MBxLocal() \
 (!MBxServer())

#define BxOwningConnection (GetLocalRole() != ENetRole::ROLE_SimulatedProxy)
// If you are simulated, you don't have a network owning role

#define ReturnOnClient() \
 if(MBxLocal()) return; \
 if(HasNull()) return;

#define MakeText(__PARAM__) FText::FromString(TEXT(__PARAM__))
#define MakeName(__PARAM__) FName(TEXT(__PARAM__))

#define BText(__PARAM__) (__PARAM__ == true) ? FString(TEXT("True ")) : FString(TEXT("False"))

#define BxIsNullServer (IOnlineSubsystem::Get()->GetSubsystemName() == "NULL")
#define SnMaxSteamSearch100

// -------------------------------------------------------------------------------------------------------------------------------------------------------------
