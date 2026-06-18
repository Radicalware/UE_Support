// Fill out your copyright notice in the Description page of Project Settings.


#include "ROSS/OSS/RpEcommerce.h"
#include "OnlineSubsystemUtils.h"


URpEcommerce::URpEcommerce()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URpEcommerce::BeginPlay()
{
	Super::BeginPlay();
}

sp<TNetResult<TArray<FUniqueOfferId>>> URpEcommerce::QueryOffers() const
{
    auto Result = MakeThreadPtr(TNetResult<TArray<FUniqueOfferId>>);
    auto& OSS = The.GetIOnlineSubsytem();
    auto UserId = The.GetNetUserID();

    // Get the store interface, if the online subsystem supports it.
    auto StoreV2 = OSS.GetStoreV2Interface();
    if (!StoreV2.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support e-commerce store."));
        return Result;
    }

    // Query for all offers.
    StoreV2->QueryOffersByFilter(
        *UserId,
        FOnlineStoreFilter(),
        FOnQueryOnlineStoreOffersComplete::CreateWeakLambda(
            this, [ResultWk = TWeakPtr<TNetResult<TArray<FUniqueOfferId>>>(Result)]
            (bool bWasSuccessful, const TArray<FUniqueOfferId>& FvOfferIDs, const FString& Error) 
            {
                auto OffersPtr = MakeShared<TArray<FUniqueOfferId>, ESPMode::ThreadSafe>(FvOfferIDs);
                GetWeakSafe(Result); // Make sure the result callback is still valid.
                Result.OnReturnResult(bWasSuccessful, OffersPtr, Error);
            }));
    return Result;
}

TArray<FOnlineStoreOfferRef> URpEcommerce::GetEcommerceCachedOffers() const
{
    auto& OSS = The.GetIOnlineSubsytem();
    auto& Identity = The.GetIdentity();
    auto& UserId = The.GetNetUserID();

    // Get the store interface, if the online subsystem supports it.
    auto StoreV2 = OSS.GetStoreV2Interface();
    if (!StoreV2.IsValid())
    {
        return TArray<FOnlineStoreOfferRef>();
    }

    // Get all of the cached offers and return them.
    TArray<FOnlineStoreOfferRef> Offers;
    StoreV2->GetOffers(Offers);
    return Offers;
}

sp<TNetResult<>> URpEcommerce::StartPurchase(
    const TMap<FString, int32>& OfferIdsToQuantities)
{
    auto Result = MakeThreadPtr(TNetResult<>);
    auto& OSS = The.GetIOnlineSubsytem();
    auto& UserId = The.GetNetUserID();

    // Get the purchase interface, if the online subsystem supports it.
    auto Purchase = OSS.GetPurchaseInterface();
    if (!Purchase.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support e-commerce purchasing."));
        return Result;
    }

    // Create our checkout request.
    FPurchaseCheckoutRequest CheckoutRequest;
    for (const auto& KV : OfferIdsToQuantities){
        CheckoutRequest.AddPurchaseOffer(TEXT(""), KV.Key, KV.Value);
    }

    // Start the purchase.
    Purchase->Checkout(
        *UserId,
        CheckoutRequest,
        FOnPurchaseReceiptlessCheckoutComplete::CreateWeakLambda(
            this, [ResultWk = TWeakPtr<TNetResult<>>(Result)]
            (const FOnlineError& Error) 
            {
                GetWeakSafe(Result);
                Result.OnResult(Error.bSucceeded, Error.ToLogString());
            }));
    return Result;
}

sp<TNetResult<>> URpEcommerce::QueryEntitlements()
{
    auto Result = MakeThreadPtr(TNetResult<>);
    auto& OSS = The.GetIOnlineSubsytem();
    auto& UserId = The.GetNetUserID();

    // Get the entitlements interface, if the online subsystem supports it.
    auto Entitlements = OSS.GetEntitlementsInterface();
    if (!Entitlements.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support e-commerce entitlements."));
        return Result;
    }

    // Register an event so we can receive the query outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle = Entitlements->AddOnQueryEntitlementsCompleteDelegate_Handle(
        FOnQueryEntitlementsCompleteDelegate::CreateWeakLambda(
            this,
            [UserId, Entitlements, CallbackHandle, ResultWk = TWeakPtr<TNetResult<>>(Result)]
            (
                bool bCallbackWasSuccessful,
                const FUniqueNetId& CallbackUserId,
                const FString&,
                const FString& CallbackError) 
            {
                    // Check if this callback is for us.
                    if (CallbackUserId != *UserId)
                    {
                        // This callback isn't for our call.
                        return;
                    }

                    GetWeakSafe(Result);
                    Result.OnResult(bCallbackWasSuccessful, CallbackError);

                    // Unregister this callback since we've handled the call we care about.
                    Entitlements->ClearOnQueryEntitlementsCompleteDelegate_Handle(*CallbackHandle);
            }));

    // Attempt to query all of the entitlements.
    if (!Entitlements->QueryEntitlements(*UserId, TEXT(""), FPagedQuery()))
    {
        // The call failed to start; unregister callback handler.
        Entitlements->ClearOnQueryEntitlementsCompleteDelegate_Handle(*CallbackHandle);
        Result->OnResult(false, TEXT("QueryEntitlements call failed to start."));
    }
    return Result;
}

TArray<TSharedRef<FOnlineEntitlement>> URpEcommerce::GetCachedEntitlements() const
{
    auto Result = MakeThreadPtr(TNetResult<>);
    auto& OSS = The.GetIOnlineSubsytem();
    auto& UserId = The.GetNetUserID();

    auto Entitlements = OSS.GetEntitlementsInterface();
    if (!Entitlements.IsValid())
    {
        return TArray<TSharedRef<FOnlineEntitlement>>();
    }

    // Get all of the cached entitlements and return them.
    TArray<TSharedRef<FOnlineEntitlement>> EntitlementsArray;
    Entitlements->GetAllEntitlements(*UserId, TEXT(""), EntitlementsArray);
    return EntitlementsArray;
}

sp<TNetResult<>> URpEcommerce::QueryReceipts()
{
    auto Result = MakeThreadPtr(TNetResult<>);
    auto& OSS = The.GetIOnlineSubsytem();
    auto& UserId = The.GetNetUserID();

    // Get the purchase interface, if the online subsystem supports it.
    auto Purchase = OSS.GetPurchaseInterface();
    if (!Purchase.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support e-commerce purchasing."));
        return Result;
    }

    // Query for all receipts.
    Purchase->QueryReceipts(
        *UserId,
        false,
        FOnQueryReceiptsComplete::CreateWeakLambda(
            this,[ResultWk = TWeakPtr<TNetResult<>>(Result)]
            (const FOnlineError& Error) 
            {
                GetWeakSafe(Result);
                Result.OnResult(Error.bSucceeded, Error.ToLogString());
            }));
    return Result;
}

TArray<FPurchaseReceipt> URpEcommerce::GetCachedReceipts() const
{
    auto Result = MakeThreadPtr(TNetResult<>);
    auto& OSS = The.GetIOnlineSubsytem();
    auto& UserId = The.GetNetUserID();

    // Get the purchase interface, if the online subsystem supports it.
    auto Purchase = OSS.GetPurchaseInterface();
    if (!Purchase.IsValid())
    {
        return TArray<FPurchaseReceipt>();
    }

    // Get all of the cached receipts and return them.
    TArray<FPurchaseReceipt> Receipts;
    Purchase->GetReceipts(*UserId, Receipts);
    return Receipts;
}

