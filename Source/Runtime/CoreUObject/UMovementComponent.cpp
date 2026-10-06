#include "Runtime/CoreUObject/UMovementComponent.h"

IMPLEMENT_UCLASS(UMovementComponent, UActorComponent)

void UMovementComponent::Initialize()
{
	Super::Initialize();
	UpdatedComponent = GetActorOwner()->GetRootComponent();
	bTickEnabled = true;

}

void UMovementComponent::Update(float DeltaTime)
{
	//Super::Update(DeltaTime);
}

bool UMovementComponent::MoveUpdatedComponent(const FVector& Delta, const FQuaternion& newRotation, bool bSweep)
{
	return MoveUpdatedComponentImpl(Delta, newRotation, bSweep);
}

bool UMovementComponent::MoveUpdatedComponentImpl(const FVector& Delta, const FQuaternion& newRotation, bool bSweep)
{
	return true;
}

