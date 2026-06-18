// Fill out your copyright notice in the Description page of Project Settings.


#include "ROSS/OSS/RpUserCloud.h"
#include "OnlineSubsystemUtils.h"
#include "GameFramework/SaveGame.h"

URpUserCloud::URpUserCloud()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URpUserCloud::BeginPlay()
{
	Super::BeginPlay();
}

sp<TNetResult<TArray<FCloudFileHeader>>> URpUserCloud::QueryFiles()
{
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    auto Result = MakeThreadPtr(TNetResult<TArray<FCloudFileHeader>>);
    auto UserCloud = OSS.GetUserCloudInterface();
    if (!UserCloud.IsValid())
    {
        Result->OnResult(
            false,
            TEXT("Online subsystem does not support user cloud."));
        return Result;
    }

    // Register an event so we can receive the outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle = UserCloud->AddOnEnumerateUserFilesCompleteDelegate_Handle(
        FOnEnumerateUserFilesCompleteDelegate::CreateWeakLambda(
            this,
            [UserCloud, CallbackHandle, ResultWk = TWeakPtr<TNetResult<TArray<FCloudFileHeader>>>(Result), UserId](
                bool bCallbackWasSuccessful,
                const FUniqueNetId& CallbackUserId) {
                    // Check if this callback is for us.
                    if (*UserId != CallbackUserId)
                    {
                        // This callback isn't for our call.
                        return;
                    }

                    GetWeakSafe(Result);
                    if (!bCallbackWasSuccessful)
                    {
                        Result.OnResult(
                            false,
                            TEXT("User cloud enumeration failed."));
                        UserCloud->ClearOnEnumerateUserFilesCompleteDelegate_Handle(*CallbackHandle);
                        return;
                    }

                    // Otherwise, convert the results.
                    auto UserCloudFilesPtr = MakeShared<TArray<FCloudFileHeader>>();
                    auto& UserCloudFiles = *UserCloudFilesPtr;
                    UserCloud->GetUserFileList(*UserId, UserCloudFiles);
                    Result.OnReturnResult(true, UserCloudFilesPtr, TEXT(""));

                    // Unregister this callback since we've handled the call we care about.
                    UserCloud->ClearOnEnumerateUserFilesCompleteDelegate_Handle(*CallbackHandle);
            }));

    // Start the enumeration of user files.
    UserCloud->EnumerateUserFiles(*UserId);
    return Result;
}

sp<TNetResult<>> URpUserCloud::WriteStringToFile(const FString& FileName,const FString& FileContents)
{
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    auto Result = MakeThreadPtr(TNetResult<>);
    auto UserCloud = OSS.GetUserCloudInterface();
    if (!UserCloud.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support user cloud."));
        return Result;
    }

    // Convert UTF8 string data to bytes.
    FTCHARToUTF8 Data(*FileContents);
    TArray<uint8> Bytes(reinterpret_cast<const uint8*>(Data.Get()), Data.Length());

    // Register an event so we can receive the outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle =
        UserCloud->AddOnWriteUserFileCompleteDelegate_Handle(FOnWriteUserFileCompleteDelegate::CreateWeakLambda(
            this,
            [UserCloud, CallbackHandle, ResultWk = TWeakPtr<TNetResult<>>(Result), FileName, UserId](
                bool bCallbackWasSuccessful,
                const FUniqueNetId& CallbackUserId,
                const FString& CallbackFileName) {
                    // Check if this callback is for us.
                    if (*UserId != CallbackUserId || FileName != CallbackFileName)
                    {
                        // This callback isn't for our call.
                        return;
                    }

                    GetWeakSafe(Result);
                    Result.OnResult(
                        bCallbackWasSuccessful,
                        bCallbackWasSuccessful ? TEXT("") : TEXT("WriteUserFile call failed."));

                    // Unregister this callback since we've handled the call we care about.
                    UserCloud->ClearOnWriteUserFileCompleteDelegate_Handle(*CallbackHandle);
            }));

    // Start writing the file.
    if (!UserCloud->WriteUserFile(*UserId, FileName, Bytes, false))
    {
        Result->OnResult(false, TEXT("WriteUserFile call failed to start."));
        UserCloud->ClearOnWriteUserFileCompleteDelegate_Handle(*CallbackHandle);
    }

    return Result;
}

sp<TNetResult<>> URpUserCloud::WriteSaveGameToFile(const FString& FileName, USaveGame* SavedGameBaseCast)
{
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    auto Result = MakeThreadPtr(TNetResult<>);
    auto UserCloud = OSS.GetUserCloudInterface();
    if (!UserCloud.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support user cloud."));
        return Result;
    }

    // example of 2nd arg
    // Create the save game, store the float in it, and serialize.
    // UCPlusPlusSaveGame* SaveGame = NewObject<UCPlusPlusSaveGame>();
    // SaveGame->StoredFloat = (float)SaveGameNumber;
    // then cast to it's base class of USaveGame* when passing in.
    // It's the type SaveGameToMemory takes


    TArray<uint8> SaveData;
    if (!UGameplayStatics::SaveGameToMemory(SavedGameBaseCast, SaveData))
    {
        Result->OnResult(false, TEXT("Failed to serialize save game object."));
        return Result;
    }

    // Register an event so we can receive the outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle = 
        UserCloud->AddOnWriteUserFileCompleteDelegate_Handle(FOnWriteUserFileCompleteDelegate::CreateWeakLambda(
            this,
            [UserCloud, CallbackHandle, ResultWk = TWeakPtr<TNetResult<>>(Result), FileName, UserId](
                bool bCallbackWasSuccessful,
                const FUniqueNetId& CallbackUserId,
                const FString& CallbackFileName) {
                    // Check if this callback is for us.
                    if (*UserId != CallbackUserId || FileName != CallbackFileName)
                    {
                        // This callback isn't for our call.
                        return;
                    }

                    GetWeakSafe(Result);
                    Result.OnResult(
                        bCallbackWasSuccessful,
                        bCallbackWasSuccessful ? TEXT("") : TEXT("WriteUserFile call failed."));

                    // Unregister this callback since we've handled the call we care about.
                    UserCloud->ClearOnWriteUserFileCompleteDelegate_Handle(*CallbackHandle);
            }));

    // Start writing the file.
    if (!UserCloud->WriteUserFile(*UserId, FileName, SaveData, false))
    {
        Result->OnResult(false, TEXT("WriteUserFile call failed to start."));
        UserCloud->ClearOnWriteUserFileCompleteDelegate_Handle(*CallbackHandle);
    }
    return Result;
}

sp<TNetResult<FString>> URpUserCloud::ReadStringFromFile(const FString& FileName)
{
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    auto Result = MakeThreadPtr(TNetResult<FString>);
    auto UserCloud = OSS.GetUserCloudInterface();
    if (!UserCloud.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support user cloud."));
        return Result;
    }

    // Register an event so we can receive the outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle =
        UserCloud->AddOnReadUserFileCompleteDelegate_Handle(FOnReadUserFileCompleteDelegate::CreateWeakLambda(
            this,
            [   
                UserCloud,
                CallbackHandle,
                ResultWk = TWeakPtr<TNetResult<FString>>(Result),
                FileName,
                UserId
            ]
            (bool bCallbackWasSuccessful, const FUniqueNetId& CallbackUserId, const FString& CallbackFileName) 
            {
                // Check if this callback is for us.
                if (*UserId != CallbackUserId || FileName != CallbackFileName)
                {
                    // This callback isn't for our call.
                    return;
                }

                GetWeakSafe(Result);
                if (!bCallbackWasSuccessful)
                {
                    Result.OnResult(false, TEXT("ReadUserFile call failed."));
                    UserCloud->ClearOnReadUserFileCompleteDelegate_Handle(*CallbackHandle);
                    return;
                }

                // Get the file contents.
                TArray<uint8> FileContents;
                if (!UserCloud->GetFileContents(*UserId, FileName, FileContents))
                {
                    Result.OnResult(false, TEXT("GetFileContents call failed."));
                    UserCloud->ClearOnReadUserFileCompleteDelegate_Handle(*CallbackHandle);
                    return;
                }

                // Return the result.
                FUTF8ToTCHAR Data(reinterpret_cast<const ANSICHAR*>(FileContents.GetData()), FileContents.Num());
                auto LsFileContents = MakeShared<FString>(Data.Length(), Data.Get());
                Result.OnReturnResult(true, LsFileContents, TEXT(""));

                // Unregister this callback since we've handled the call we care about.
                UserCloud->ClearOnReadUserFileCompleteDelegate_Handle(*CallbackHandle);
            }));

    // Start reading the file.
    if (!UserCloud->ReadUserFile(*UserId, FileName))
    {
        Result->OnResult(false, TEXT("ReadUserFile call failed to start."));
        UserCloud->ClearOnReadUserFileCompleteDelegate_Handle(*CallbackHandle);
    }

    return Result;
}

sp<TNetResult<USaveGame>> URpUserCloud::ReadSaveGameFromFile(const FString& FileName)
{
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    auto Result = MakeThreadPtr(TNetResult<USaveGame>);
    auto UserCloud = OSS.GetUserCloudInterface();
    if (!UserCloud.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support user cloud."));
        return Result;
    }

    // Register an event so we can receive the outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle =
        UserCloud->AddOnReadUserFileCompleteDelegate_Handle(FOnReadUserFileCompleteDelegate::CreateWeakLambda(
            this,
            [   UserCloud,
                CallbackHandle,
                ResultWk = TWeakPtr<TNetResult<USaveGame>>(Result),
                FileName,
                UserId
            ]
            (bool bCallbackWasSuccessful, const FUniqueNetId& CallbackUserId, const FString& CallbackFileName) {
                // Check if this callback is for us.
                if (*UserId != CallbackUserId || FileName != CallbackFileName)
                {
                    // This callback isn't for our call.
                    return;
                }

                GetWeakSafe(Result);
                if (!bCallbackWasSuccessful)
                {
                    Result.OnResult(false, TEXT("ReadUserFile call failed."));
                    UserCloud->ClearOnReadUserFileCompleteDelegate_Handle(*CallbackHandle);
                    return;
                }

                // Get the file contents.
                TArray<uint8> FileContents;
                if (!UserCloud->GetFileContents(*UserId, FileName, FileContents))
                {
                    Result.OnResult(false, TEXT("GetFileContents call failed."));
                    UserCloud->ClearOnReadUserFileCompleteDelegate_Handle(*CallbackHandle);
                    return;
                }

                // Return the result.
                // UCPlusPlusSaveGame* SaveGame = Cast<UCPlusPlusSaveGame>(UGameplayStatics::LoadGameFromMemory(FileContents));
                USaveGame* SaveGame = UGameplayStatics::LoadGameFromMemory(FileContents);
                if (!SaveGame || !IsValid(SaveGame))
                {
                    Result.OnResult(false, TEXT("Unable to deserialize memory to USaveGame."));
                    UserCloud->ClearOnReadUserFileCompleteDelegate_Handle(*CallbackHandle);
                    return;
                }
                Result.OnReturnResult(true, TStrongObjectPtr<USaveGame>(SaveGame), TEXT(""));

                // Unregister this callback since we've handled the call we care about.
                UserCloud->ClearOnReadUserFileCompleteDelegate_Handle(*CallbackHandle);
            }));

    // Start reading the file.
    if (!UserCloud->ReadUserFile(*UserId, FileName))
    {
        Result->OnResult(false, TEXT("ReadUserFile call failed to start."));
        UserCloud->ClearOnReadUserFileCompleteDelegate_Handle(*CallbackHandle);
    }
    return Result;
}


