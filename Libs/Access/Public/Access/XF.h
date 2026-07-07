#pragma once

#include "CoreMinimal.h"
#include "Macros.h"


class XF // Extended Functions
{
public:

    template<typename K, typename  V> 
    static inline TPair<K, V>  MakePair(const K&& Key, const V&& Value) { return TPair<K, V>(Key, Value); }
    template<typename K, typename  V> 
    static inline TPair<K, V*> MakePair(const K&& Key, const V* Value) { return TPair<K, V>(Key, Value); }
    static        FRotator     MakeRotator(float Roll, float Pitch, float Yaw);

    static bool LocationsAreClose(const FVector& Loc1, const FVector& Loc2);
    static bool RotationsAreClose(const FRotator& Rot1, const FRotator& Rot2);

    template<typename F, typename S>
    static constexpr F Min(F Val1, S Val2) { return (Val1 < Val2) ? Val1 : Val2; }
    template<typename F, typename S>
    static constexpr F Max(F Val1, S Val2) { return (Val1 > Val2) ? Val1 : Val2; }

    template<typename F, typename S>
    static constexpr bool Appx(F&& LnFirst, S&& LnSecond);

    static const char* FStringToRValChars(const FString& FsStr);
    static const char* FStringToRValChars(const FName& FsStr);

    static bool BxMapExists(const FString& FsPathToWorld);
    static bool BxGameModeExists(const FString& FsPathToGameMode);
    static bool BxActorExists(const FString& FsPathToActor);
    template<typename T>
    static bool BxUObjectExists(const FString& FsPathToUObject);

    static int32 StringToInt(const FString& FsStr);

    template<typename T>
    static std::tuple<FString, FString> ToUObjectAndAssetPath(const FString& FsFilePath);

    // ------------------------------------------------------------------------------------
    // CombineToString(...) = Join Values of All Sorts Into One Line of Text
    template <typename FF, typename... RR>
    static FString CombineToString(const FF& Frist, const RR&... Rest);
    template <typename... RR>
    static FString CombineToString(const char* Frist, const RR&... Rest);
    inline static FString CombineToString() { return FString(); }
    // ------------------------------------------------------------------------------------
    template<typename F>
    static void ThrowNulls(const F* First);
    template<typename F, typename... R>
    static void ThrowNulls(const F* First, const R*... Rest);
    // ------------------------------------------------------------------------------------
    static FString FindReplace(const FString& FsStr, const FString& FsToFind, const FString& FsToReplace);
    static FString FindFirstMatch(const FString& FsStr, const FString& FsToFind);
private:
    inline static std::string SsTempStr;
};


// ===============================================================================================

template <typename FF>
inline void CombineFString(FString& String, const FF& First)
{
    String += FString::FromInt(First);
}

template <>
inline void CombineFString<char>(FString& String, const char& First)
{
    String += First;
}

inline void CombineFString(FString& String, const char* First)
{
    String += FString(First);
}

inline void CombineFString(FString& String, const TCHAR* First)
{
    String += FString(First);
}


template <>
inline void CombineFString<FString>(FString& String, const FString& First)
{
    String += First;
}

template<>
inline void CombineFString<FUniqueNetIdRepl>(FString& String, const FUniqueNetIdRepl& First)
{
    String += First.ToString();
}

template <>
inline void CombineFString<FText>(FString& String, const FText& First)
{
    String += First.ToString();
}

template <>
inline void CombineFString<FName>(FString& String, const FName& First)
{
    String += First.ToString();
}

// ----------------------------------------------------------------------------------------------
// Nums
template <>
inline void CombineFString<int32>(FString& String, const int32& First)
{
    String += FString::FromInt(First);
}

template <>
inline void CombineFString<float>(FString& String, const float& First)
{
    String += FString::SanitizeFloat(First);
}

template <>
inline void CombineFString<double>(FString& String, const double& First)
{
    String += FString::SanitizeFloat(First);
}

template <>
inline void CombineFString<std::string>(FString& String, const std::string& First)
{
    String += FString(First.c_str());
}
// ----------------------------------------------------------------------------------------------

template <typename FF, typename ... RR>
inline void CombineFString(FString& String, const FF& First, const RR&... Rest)
{
    CombineFString(String, First);
    CombineFString(String, Rest...);
}

template<typename F, typename S>
inline constexpr bool XF::Appx(F&& LnFirst, S&& LnSecond)
{
    return FMath::IsNearlyEqual(LnFirst, LnSecond);
}

template<typename T>
inline bool XF::BxUObjectExists(const FString& FsPathToUObject)
{
    UClass* LoUObjectClass = StaticLoadClass(T::StaticClass(), nullptr, *FsPathToUObject);
    if (!LoUObjectClass) {
        PrintE("UObject NOT found: ", FsPathToUObject);
        return false;
    }
    return true;
}

template<typename T>
inline std::tuple<FString, FString> XF::ToUObjectAndAssetPath(const FString& FsFilePath)
{
    auto LsAssetPath = FsFilePath;
    LsAssetPath = XF::FindReplace(LsAssetPath, TEXT(R"(^(\.?)/Content/)"), TEXT("/Game/"));
    LsAssetPath = XF::FindReplace(LsAssetPath, TEXT(R"(\.uasset$)"), TEXT(""));
    const auto LsName = FPackageName::GetShortName(LsAssetPath);
    if (!LsAssetPath.EndsWith(TEXT("_C")))
    {
        const auto LsAssetName = XF::FindFirstMatch(LsAssetPath, TEXT(R"(/([^/]+)$)"));
        LsAssetPath += TEXT(".") + LsAssetName + TEXT("_C");
    }
    LsAssetPath = "Blueprint'" + LsAssetPath + "'";
    ensure(!LsAssetPath.IsEmpty());
    ensure(!LsName.IsEmpty());
    ensure(XF::BxUObjectExists<T>(LsAssetPath));
    return std::make_tuple(LsName, LsAssetPath);
}

template <typename FF, typename ... RR>
FString XF::CombineToString(const FF& First, const RR&... Rest)
{
    FString String;
    CombineFString(String, First, Rest...);
    return String;
}

template <typename ... RR>
FString XF::CombineToString(const char* First, const RR&... Rest)
{
    FString String;
    CombineFString(String, FString(First), Rest...);
    return String;
}

template <typename F>
void XF::ThrowNulls(const F* First)
{
    if (!First)
        throw FString(TEXT("Failed to deref"));
}

template <typename F, typename ... R>
void XF::ThrowNulls(const F* First, const R*... Rest)
{
    ThrowNulls(First);
    ThrowNulls(Rest...);
}
