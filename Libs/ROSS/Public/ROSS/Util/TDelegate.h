#pragma once

#include "Access/General.h"
#include <any>

class FTDelegate
{
    std::any MoDelegate;
public:
    template<typename T>
    INL void SetDelegate(const T& FoDelegate){ MoDelegate = FoDelegate; }
    
    template<typename T = FVVDelegate>
    INL T& GetDelegate();

    template<typename T = FVVDelegate>
    INL void Unbind();

    template<typename T = FVVDelegate>
    INL bool BxBound();
};

template<typename T>
T& FTDelegate::GetDelegate()
{
    if (!MoDelegate.has_value())
        throw BBB("No delegate is set.");

    try{
        return std::any_cast<T&>(MoDelegate);
    }
    catch (const std::bad_any_cast& e){
        throw BBB(FString("Bad delegate type cast >> ") + FString(e.what()));
    }
    catch(...){
        throw BBB("Unknown error during delegate type cast.");
    }
}

template<typename T>
void FTDelegate::Unbind()
{
    if (!MoDelegate.has_value())
        return;

    try {
        auto& LoDelegate = std::any_cast<T&>(MoDelegate);
        try {
            LoDelegate.Unbind();
        }
        catch (...) {
            BBB("error during delegate unbind.");
        }
    }
    catch (const std::bad_any_cast& e) {
        throw BBB(FString("Bad delegate type cast >> ") + FString(e.what()));
    }
    catch (...) {
        throw BBB("Unknown error during delegate type cast.");
    }
}

template<typename T>
bool FTDelegate::BxBound()
{
    if (!MoDelegate.has_value())
        return false;

    try {
        auto& LoDelegate = std::any_cast<T&>(MoDelegate);
        return LoDelegate.IsBound();
    }
    catch (const std::bad_any_cast& e) {
        throw BBB(FString("Bad delegate type cast >> ") + FString(e.what()));
    }
    catch (...) {
        throw BBB("Unknown error during delegate type cast.");
    }
}

