#include "pch.h"
#include "UActorComponent.h"
#include "Runtime/Engine/UWorld.h"

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

void UActorComponent::Register(UWorld* InWorld)
{
    if (World == InWorld) { return; }
    if (World) { Unregister(); }

    World = InWorld;
}

void UActorComponent::Unregister()
{
    if (bHasBegunPlay) { EndPlay(); }
    World = nullptr;
}
