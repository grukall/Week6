#include "UClass.h"
#include "UActorComponent.h"
#include "ThirdParty/Json/json.hpp"
#include "UObjectGlobals.h" 
#include "Runtime/Engine/FArchive.h"
#include "Runtime/CoreUObject/UWorld.h"


IMPLEMENT_UCLASS(UActorComponent, UObject)

void UActorComponent::Initialize()
{
    Super::Initialize();
    World = nullptr;
    bHasBegunPlay = false;
    bTickEnabled = false;
}
void UActorComponent::Release()
{
    if (bHasBegunPlay) { EndPlay(); }
    if (World) { Unregister(); }

    ActorOwner = nullptr;
    World = nullptr;

    Super::Release();
}

void UActorComponent::DuplicateSubObjects()
{
    Super::DuplicateSubObjects();

    World = nullptr;
    BatchIndex = -1;
    bHasBegunPlay = false;
}

void UActorComponent::Register(UWorld& InWorld)
{
    if (World == &InWorld) { return; }
    if (World) { Unregister(); }

    World = &InWorld;
}

UWorld* UActorComponent::GetWorld() const
{
    return (World);
}

ULevel* UActorComponent::GetLevel() const
{
    if (!World)
    {
        return nullptr;
    }
    return World->GetPersistentLevel();
}

void UActorComponent::BeginPlay()
{
    if (!World || bHasBegunPlay) { return; }
    bHasBegunPlay = true;
}

void UActorComponent::EndPlay()
{
    if (!bHasBegunPlay) { return; }
    bHasBegunPlay = false;
}

void UActorComponent::Unregister()
{
    if (bHasBegunPlay) { EndPlay(); }
    World = nullptr;
}

bool UActorComponent::IsRegistered() const
{
    return World != nullptr; 
}

void UActorComponent::Serialize(FArchive& Archive) const
{
    Super::Serialize(Archive);
}

void UActorComponent::Deserialize(const FArchive& Archive)
{
    Super::Deserialize(Archive);
}



