#include "Runtime/CoreUObject/ULightComponent.h"
#include "Runtime/Engine/UWorld.h"

IMPLEMENT_UCLASS(ULightComponent, USceneComponent)

void ULightComponent::Register(UWorld* InWorld)
{
	Super::Register(InWorld);
	InWorld->GetScene()->AddLightComponent(this);
}

void ULightComponent::BuildConstants(FLightConstants& Constants)
{
}