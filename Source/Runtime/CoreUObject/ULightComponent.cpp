#include "Runtime/CoreUObject/ULightComponent.h"
#include "Runtime/Engine/UWorld.h"

IMPLEMENT_UCLASS(ULightComponent, USceneComponent)

void ULightComponent::Register(UWorld* InWorld)
{
	Super::Register(InWorld);
	InWorld->GetScene()->AddLightComponent(this);
}

void ULightComponent::Unregister()
{
    if (World)
    {
        if (FScene* Scene = World->GetScene())
        {
            Scene->RemoveLightComponent(this);
        }
    }

    Super::Unregister();
}

void ULightComponent::BuildConstants(FLightConstants& Constants)
{
}