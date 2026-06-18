// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ROSS/RpConfig.h"
#include "Interfaces/OnlineStoreInterfaceV2.h"
#include "Interfaces/OnlineEntitlementsInterface.h"
#include "Interfaces/OnlinePurchaseInterface.h"
#include "RpEcommerce.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class THEGAME_API URpEcommerce : public URpConfig
{
    GENERATED_BODY()
public:
    URpEcommerce();
protected:
    virtual void BeginPlay() override;
public:
    sp<TNetResult<TArray<FUniqueOfferId>>> QueryOffers() const;
    TArray<FOnlineStoreOfferRef> GetEcommerceCachedOffers() const;
    sp<TNetResult<>> StartPurchase(const TMap<FString, int32> &OfferIdsToQuantities);
    sp<TNetResult<>> QueryEntitlements();
    TArray<TSharedRef<FOnlineEntitlement>> GetCachedEntitlements() const;
    sp<TNetResult<>> QueryReceipts();
    TArray<FPurchaseReceipt> GetCachedReceipts() const;
};
