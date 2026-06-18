// Fill out your copyright notice in the Description page of Project Settings.


#include "ROSS/OSS/RpAuth.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineAuthHandlerSteam.h"

#include "ROSS/ROSS.h"


URpAuth::URpAuth()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void URpAuth::BeginPlay()
{
    Super::BeginPlay();
}

bool URpAuth::BxAuthReady() const
{
    auto OSSPtr = Online::GetSubsystem(The.GetWorld());
    if (!OSSPtr)
        return false;
    auto IdentityPtr = OSSPtr->GetIdentityInterface();
    return IdentityPtr.IsValid();
}

bool URpAuth::BxLoggedIn() const
{
    auto OSSPtr = Online::GetSubsystem(The.GetWorld());
    if(!OSSPtr)
        return false;
    auto IdentityPtr = OSSPtr->GetIdentityInterface();
    if(!IdentityPtr.IsValid())
        return false;
    auto UserIdPtr = IdentityPtr->GetUniquePlayerId(The.LocalUserNum);
    return UserIdPtr.IsValid();
}

bool URpAuth::BxCanLinkCrossPlatformAccount() const
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(The.GetWorld());
    if (OSS == nullptr)
    {
        return false;
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(The.LocalUserNum);
    if (!UserId.IsValid())
    {
        return false;
    }

    // Get the signed in user's account.
    auto UserAccount = Identity->GetUserAccount(*UserId);
    checkf(UserId.IsValid(), TEXT("Expected GetUserAccount to return a valid account."));

    // Get the value of the 'crossPlatform.canLink' authentication attribute.
    FString Value;
    if (!UserAccount->GetAuthAttribute(TEXT("crossPlatform.canLink"), Value))
    {
        return false;
    }

    // If this attribute is true, then the user can link a cross-platform account.
    return Value == TEXT("true");
}

void URpAuth::CheckDedicatedServer(UWorld* FoWorldPtr)
{
    Print("EOS Logging In")
    Print("Authentication Subsystem: ", Online::GetSubsystem(FoWorldPtr)->GetSubsystemName());

    auto LoWorld = (FoWorldPtr) ? FoWorldPtr : GetWorld();
    if (not FoWorldPtr->IsNetMode(NM_DedicatedServer))
    {
        PrintW("Not a Dedicated Server");
        return;
    }
    IOnlineSubsystem* OSS = IOnlineSubsystem::Get(STEAM_SUBSYSTEM);
    UE_LOG(LogTemp, Log, TEXT("OSS: %s"), OSS ? TEXT("STEAM") : TEXT("NULL"));

    const bool bGSInit   = (SteamGameServerUtils() != nullptr);
    const bool bLoggedOn = (SteamGameServer() && SteamGameServer()->BLoggedOn());
    const uint32 AppId   = SteamGameServerUtils() ? SteamGameServerUtils()->GetAppID() : 0;
    UE_LOG(LogTemp, Warning, TEXT("[AppId:%u] GameServerUtils: %s, LoggedOn: %s"),
        AppId,
        bGSInit   ? TEXT("YES") : TEXT("NO"),
        bLoggedOn ? TEXT("YES") : TEXT("NO"));
    if (not bGSInit or not bLoggedOn) {
        PrintE("This Will Fail");
    }
}


sp<TNetResult<void, FOnLoginCompleteDelegate>> URpAuth::ExAutoLogin(UWorld* FoWorldPtr, FOnLoginCompleteDelegate& FoDelegate)
{
    Print("Authentication Starting")
    Print("Authentication Subsystem: ", Online::GetSubsystem(FoWorldPtr)->GetSubsystemName());
    auto Result = MakeShared<TNetResult<void, FOnLoginCompleteDelegate>>();
    Result->SetExternalCallback(FoDelegate);
    auto WorldPtr = GetWorld();
    if (WorldPtr) {
        WorldPtr = this->GetNewWorldPtr();
    }
    
    WorldPtr = (WorldPtr) ? WorldPtr : FoWorldPtr;
    auto LoSubsystemPtr = Online::GetSubsystem(WorldPtr);
    ensure(LoSubsystemPtr);
    GET(OSS, LoSubsystemPtr);

    Print("OSS->GetSubsystemName: ", OSS.GetSubsystemName());
    auto Identity = OSS.GetIdentityInterface();
    ensure(Identity.IsValid());

    // get the others fixed like this, we use TWeakPtr for Non-UObjects version of TSoftObjectPtr
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle = Identity->AddOnLoginCompleteDelegate_Handle(
        The.LocalUserNum,
        FOnLoginCompleteDelegate::CreateWeakLambda(
            this,
            [this, Identity, CallbackHandle, FoWorldPtr, ResultWk = TWeakPtr<TNetResult<void, FOnLoginCompleteDelegate>>(Result)](
                int32 FnLocalUserNum,
                bool bCallbackWasSuccessful,
                const FUniqueNetId& FoNetID,
                const FString& CallbackError)
            {
                    // Check if this callback is for us.
                    if (FnLocalUserNum != The.LocalUserNum){
                        Print("Callback not for us");
                        return; // This callback isn't for our call.
                    }

                    GetWeakSafe(Result);
                    Result.OnResult(bCallbackWasSuccessful, CallbackError,
                        FnLocalUserNum, bCallbackWasSuccessful, FoNetID, CallbackError);
                    if (!bCallbackWasSuccessful) {
                        PrintW("Login failed: ", CallbackError);
                    }
                    else{
                         Print("Login Details...",
                             "\n Local User ID: ", FnLocalUserNum,
                             "\n Net ID:        ", FoNetID.ToString(),
                             "\n Successs:      ", bCallbackWasSuccessful,
                             "\n Net Mode:      ", this->GetWorld()->GetNetMode()
                         )
                    }
                    // Unregister this callback since we've handled the call we care about.
                    Identity->ClearOnLoginCompleteDelegate_Handle(The.LocalUserNum, *CallbackHandle);
            }));

    Print("EOS Subsystem login call starting...");
    if (Identity->AutoLogin(LocalUserNum))
    {
        Print("Login started successfully (async).");
        return Result;
    }
    else
    {
        Identity->ClearOnLoginCompleteDelegate_Handle(The.LocalUserNum, *CallbackHandle);
        PrintW("Login call failed to start.");
    }
    return Result;
}

sp<TNetResult<>> URpAuth::ExLogout()
{
    Print("EOS Logging Out")
    // Get the online subsystem.
    auto& OSS = GetIOnlineSubsytem();
    auto& Identity = GetIdentity();
    auto Result = MakeThreadPtr(TNetResult<>);

    // Register an event so we can receive the logout outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    
    *CallbackHandle = Identity.AddOnLogoutCompleteDelegate_Handle(
        The.LocalUserNum,
        FOnLogoutCompleteDelegate::CreateWeakLambda(
            this,
            [this, &Identity, CallbackHandle, 
            ResultWk = TWeakPtr<TNetResult<void>>(Result)
            ]
            (
                int32 CallbackLocalUserNum,
                bool bCallbackWasSuccessful) 
            {
                    // Check if this callback is for us.
                    if (CallbackLocalUserNum != The.LocalUserNum)
                    {
                        // This callback isn't for our call.
                        return;
                    }

                    GetWeakSafe(Result);

                    // Return whether the logout was successful.
                    Result.OnResult(
                        bCallbackWasSuccessful,
                        bCallbackWasSuccessful 
                            ? TEXT("") 
                            : TEXT("The Logout callback failed."));

                    // Unregister this callback since we've handled the call we care about.
                    Identity.ClearOnLogoutCompleteDelegate_Handle(The.LocalUserNum, *CallbackHandle);
            }));

    // Attempt to sign out of the online subsystem.
    if (!Identity.Logout(The.LocalUserNum))
    {
        // The call failed to start; unregister callback handler.
        Identity.ClearOnLogoutCompleteDelegate_Handle(The.LocalUserNum, *CallbackHandle);
        Result->OnResult(false, TEXT("Logout call failed to start."));
    }
    return Result;
}



