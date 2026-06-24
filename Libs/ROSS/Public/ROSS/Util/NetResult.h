#pragma once

#include "CoreMinimal.h"
#include "Access/XF.h" // Assuming PrintW or related helpers live here
#include "Access/Macros.h"
#include "UObject/StrongObjectPtr.h"
#include "Templates/SharedPointer.h"
#include "Interfaces/OnlineSessionDelegates.h"
#include "Delegates/DelegateSignatureImpl.inl"

#include <type_traits>
#include <concepts>

// Usage
//      SetValueCallback >> TNetResult<R, C>
//          R = Return FValueDelegate
//          void(bool, const TNetResultPtr<R>)
// 
//      SetVoidCallback  >> TNetResult<void, C>
//          void  = FVoidDelegate
//          void(bool, const FString&)
// OR
//      SetCallbacks(void(void) Success, void(void) Fail)

// Treat cv/ref variations as the same; works when R is a UObject-derived type, not a pointer
template<typename T>
concept CIsUObject = std::is_base_of_v<UObject, std::remove_cvref_t<T>>;

template<typename R>
using TNetResultPtr = std::conditional_t<CIsUObject<R>, TStrongObjectPtr<R>, TSharedPtr<R>>;

// Base now templated on delegate type C, defaulting to FVVDelegate
template<typename C = FVVDelegate>
class FNetResultBase
{
protected:
    bool bDidCallback = false;
    bool MbWasSuccessful = false;

    C MoCallback;

protected:
    INL void ThrowDuplicateCallback(bool bSuccess, const FString& ErrorMessage)
    {
        if (bDidCallback) {
            throw BBB("Ran Callback Already!!");
        }
        bDidCallback = true;
        MbWasSuccessful = bSuccess;
        if (!MbWasSuccessful)
            PrintW(ErrorMessage);
    }

    INL void Reset()
    {
        bDidCallback = false;
        MoCallback.Unbind();
    }

    template<typename... CA>
    INL void ExecuteCallback(CA&&... Args)
    {
        MoCallback.Execute(Forward<CA>(Args)...);
        // bDidCallback = false; // you should NOT be calling OnResult multiple times
        MoCallback.Unbind();
    }

public:
    INL bool HasCompleted() const { return bDidCallback; }
    INL bool BxSuccessful() const { return MbWasSuccessful; }
    INL void SetExternalCallback(const C& FoCallback) { MoCallback = FoCallback; bDidCallback = false; }
};

template<typename R = void, typename C = FVVDelegate>
class TNetResult;

// Void specialization retains second parameter
template<typename C>
class TNetResult<void, C> : public FNetResultBase<C>
{
public:
    INL void Reset()
    {
        FNetResultBase<C>::Reset();
    }

    template<typename... CA>
    INL void OnResult(bool FbWasSuccessful, const FString& ErrorMessage, CA&&... Args)
    {
        The.ThrowDuplicateCallback(FbWasSuccessful, ErrorMessage);
        The.ExecuteCallback(Forward<CA>(Args)...);
    }
};

// Generic (non-void) version
template<typename R, typename C>
class TNetResult : public FNetResultBase<C>
{
public:
    using TResultPtr = TNetResultPtr<R>;
    TResultPtr MoResultPtr;
public:
    TNetResult() = default;

    INL bool BxHasResult() const { return MoResultPtr.IsValid(); }

    INL R& Get() const
    {
        if (!MoResultPtr.IsValid()) {
            ThrowIt("Result Not Valid");
        }
        return *MoResultPtr.Get();
    }

    INL void Reset()
    {
        MoResultPtr = nullptr;
        FNetResultBase<C>::Reset();
    }

    template<typename... CA>
    INL void OnResult(bool FbWasSuccessful, const FString& FsError = FString(), CA&&... Args)
    {;
        The.ThrowDuplicateCallback(FbWasSuccessful, FsError);
        The.ExecuteCallback(Forward<CA>(Args)...);
    }

    template<typename... CA>
    INL void OnReturnResult(bool FbWasSuccessful, TResultPtr FtResult, const FString& FsError = FString(), CA&&... Args)
    {
        The.ThrowDuplicateCallback(FbWasSuccessful, FsError);
        if (FtResult.IsValid())
            MoResultPtr = MoveTemp(FtResult);

        The.ExecuteCallback(Forward<CA>(Args)...);
    }

    // Convenience overload: raw pointer for UObject types
    template<typename... CA>
    INL void OnReturnResult(bool FbWasSuccessful, R* RawPtr, const FString& FsError = FString(), CA&&... Args) requires CIsUObject<R>
    {
        TResultPtr TempPtr;
        if (RawPtr)
        {
            TempPtr = TResultPtr(RawPtr);
        }
        OnReturnResult(FbWasSuccessful, MoveTemp(TempPtr), FsError, Forward<CA>(Args)...);
    }

    template<typename... CA>
    INL void OnReturnResult(bool FbWasSuccessful, std::nullptr_t /*EmptyPtr*/, const FString& FsError = FString(), CA&&... Args)
    {
        TResultPtr NullPtr{};
        OnReturnResult(FbWasSuccessful, MoveTemp(NullPtr), FsError, Forward<CA>(Args)...);
    }
};

