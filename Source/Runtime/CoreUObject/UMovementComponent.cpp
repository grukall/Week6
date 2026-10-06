#include "Runtime/CoreUObject/UMovementComponent.h"

IMPLEMENT_UCLASS(UMovementComponent, UActorComponent)

void UMovementComponent::Initialize()
{
	Super::Initialize();

	if (AActor* Owner = GetActorOwner())
	{
		UpdatedComponent = Owner->GetRootComponent();
		bTickEnabled = true;
		bTickInEditor = true;
	}

}

void UMovementComponent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

bool UMovementComponent::MoveUpdatedComponent(const FVector& Delta, const FQuaternion& newRotation, bool bSweep)
{
	return MoveUpdatedComponentImpl(Delta, newRotation, bSweep);
}

bool UMovementComponent::MoveUpdatedComponentImpl(const FVector& Delta, const FQuaternion& newRotation, bool bSweep)
{
	return true;
}

