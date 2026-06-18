// Fill out your copyright notice in the Description page of Project Settings.


#include "ROSS/OSS/RpTitleFile.h"
#include "Interfaces/OnlineTitleFileInterface.h"

URpTitleFile::URpTitleFile()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URpTitleFile::BeginPlay()
{
	Super::BeginPlay();
}


sp<TNetResult<TArray<FCloudFileHeader>>> URpTitleFile::QueryFiles()
{
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    auto Result = MakeThreadPtr(TNetResult<TArray<FCloudFileHeader>>);
    auto TitleFile = OSS.GetTitleFileInterface();
    if (!TitleFile.IsValid())
    {
        Result->OnResult(
            false,
            TEXT("Online subsystem does not support title file."));
        return Result;
    }

    // Register an event so we can receive the outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle =
        TitleFile->AddOnEnumerateFilesCompleteDelegate_Handle(FOnEnumerateFilesCompleteDelegate::CreateWeakLambda(
            this,
            [TitleFile, CallbackHandle, ResultWk = TWeakPtr<TNetResult<TArray<FCloudFileHeader>>>(Result), UserId](
                bool bCallbackWasSuccessful,
                const FString& CallbackErrorMessage) 
            {
                    GetWeakSafe(Result); // Make sure the result callback is still valid.
                    if (!bCallbackWasSuccessful)
                    {
                        Result.OnResult(false, CallbackErrorMessage);
                        TitleFile->ClearOnEnumerateFilesCompleteDelegate_Handle(*CallbackHandle);
                        return;
                    }
                    // Otherwise, convert the results.
                    auto TitleFileFilesPtr = MakeShared<TArray<FCloudFileHeader>>();
                    TitleFile->GetFileList(*TitleFileFilesPtr);

                    // Return the results.
                    Result.OnReturnResult(true, TitleFileFilesPtr, TEXT(""));

                    // Unregister this callback since we've handled the call we care about.
                    TitleFile->ClearOnEnumerateFilesCompleteDelegate_Handle(*CallbackHandle);
            }));

    // Start the enumeration of title files.
    TitleFile->EnumerateFiles(FPagedQuery(0, -1));
    return Result;
}

sp<TNetResult<FString>> URpTitleFile::StringFromFile(const FString& FileName)
{
    auto& OSS = GetIOnlineSubsytem();
    auto& UserId = GetNetUserID();
    auto Result = MakeThreadPtr(TNetResult<FString>);
    auto TitleFile = OSS.GetTitleFileInterface();
    if (!TitleFile.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support title file."));
        return Result;
    }

    // Register an event so we can receive the outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle = TitleFile->AddOnReadFileCompleteDelegate_Handle(FOnReadFileCompleteDelegate::CreateWeakLambda(
        this,
        [TitleFile,
        CallbackHandle,
        ResultWk = TWeakPtr<TNetResult<FString>>(Result),
        FileName,
        UserId](bool bCallbackWasSuccessful, const FString& CallbackFileName) 
        {
            // Check if this callback is for us.
            if (FileName != CallbackFileName)
            {
                // This callback isn't for our call.
                return;
            }
            GetWeakSafe(Result); // Make sure the result callback is still valid.

            if (!bCallbackWasSuccessful) // If the read failed, retur now.
            {
                Result.OnResult(false, TEXT("ReadFile call failed."));
                TitleFile->ClearOnReadFileCompleteDelegate_Handle(*CallbackHandle);
                return;
            }

            // Get the file contents.
            TArray<uint8> FileContents;
            if (!TitleFile->GetFileContents(FileName, FileContents))
            {
                Result.OnResult(false, TEXT("GetFileContents call failed."));
                TitleFile->ClearOnReadFileCompleteDelegate_Handle(*CallbackHandle);
                return;
            }

            // Return the result.
            FUTF8ToTCHAR Data(reinterpret_cast<const ANSICHAR*>(FileContents.GetData()), FileContents.Num());
            auto LsReturn = MakeShared<FString>(Data.Length(), Data.Get());
            Result.OnReturnResult(true, LsReturn, TEXT(""));

            // Unregister this callback since we've handled the call we care about.
            TitleFile->ClearOnReadFileCompleteDelegate_Handle(*CallbackHandle);
        }));

    // Start reading the file.
    if (!TitleFile->ReadFile(FileName))
    {
        Result->OnResult(false, TEXT("ReadFile call failed to start."));
        TitleFile->ClearOnReadFileCompleteDelegate_Handle(*CallbackHandle);
    }
    return Result;
}


