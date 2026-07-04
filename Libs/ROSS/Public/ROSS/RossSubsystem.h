#pragma once

#include "Subsystems/GameInstanceSubsystem.h"
#include "RossSubsystem.generated.h"


UCLASS()
class THEGAME_API URossSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
};