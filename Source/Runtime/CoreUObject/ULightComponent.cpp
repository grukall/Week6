#include "Runtime/CoreUObject/ULightComponent.h"
#include "Runtime/Engine/UScene.h"

IMPLEMENT_UCLASS(ULightComponent, USceneComponent)

void ULightComponent::Register(UScene& InScene)
{
	Super::Register(InScene);
	InScene.AddLightComponent(this);
}

void ULightComponent::BuildConstants(FLightConstants& Constants)
{
}