// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ROSS/RpConfig.h"
#include "Interfaces/OnlineUserCloudInterface.h"
#include "Kismet/GameplayStatics.h"
#include "RpUserCloud.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class THEGAME_API URpUserCloud : public URpConfig
{
    GENERATED_BODY()
public:
    URpUserCloud();
protected:
    virtual void BeginPlay() override;
public:
    sp<TNetResult<TArray<FCloudFileHeader>>> QueryFiles();
    sp<TNetResult<>> WriteStringToFile(const FString& FileName, const FString& FileContents);
    sp<TNetResult<>> WriteSaveGameToFile(const FString& FileName, USaveGame* SavedGameBaseCast);
    sp<TNetResult<FString>> ReadStringFromFile(const FString &FileName);
    sp<TNetResult<USaveGame>> ReadSaveGameFromFile(const FString &FileName);
};
