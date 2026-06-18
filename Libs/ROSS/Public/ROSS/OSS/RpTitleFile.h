// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ROSS/RpConfig.h"
#include "RpTitleFile.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class THEGAME_API URpTitleFile : public URpConfig
{
    GENERATED_BODY()
public:
    URpTitleFile();
protected:
    virtual void BeginPlay() override;
public:
    sp<TNetResult<TArray<FCloudFileHeader>>> QueryFiles();
    sp<TNetResult<FString>> StringFromFile(const FString &FileName);
};
