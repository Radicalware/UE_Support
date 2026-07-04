#include "ROSS/RossSubsystem.h"
#ifdef BxROSS
#include "Engine/Engine.h"

void URossSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    PrintStart();
    Super::Initialize(Collection);
    Print("RossSubsystem Initialized");
}

void URossSubsystem::Deinitialize()
{

}
#endif

