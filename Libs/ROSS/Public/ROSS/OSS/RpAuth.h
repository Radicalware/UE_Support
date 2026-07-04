// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Access/General.h"
#include "ROSS/RpConfig.h"
#include "RpAuth.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class THEGAME_API URpAuth : public URpConfig
{
    GENERATED_BODY()
protected:

public:
    URpAuth();
    virtual void BeginPlay() override;

    bool BxAuthReady() const;
    bool BxLoggedIn() const;
    bool BxCanLinkCrossPlatformAccount() const;
    void ChecThrowkDedicatedServer(UWorld* FoWorldPtr = nullptr);

    sp<TNetResult<void, FOnLoginCompleteDelegate>> ExAutoLogin(UWorld* FoWorldPtr, FOnLoginCompleteDelegate& FoDelegate);
    sp<TNetResult<>> ExLogout();
};

