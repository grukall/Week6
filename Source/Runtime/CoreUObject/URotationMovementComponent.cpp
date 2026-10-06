#include "Runtime/CoreUObject/URotationMovementComponent.h"

IMPLEMENT_UCLASS(URotationMovementComponent, UMovementComponent)

void URotationMovementComponent::Update(float DeltaTime)
{
	Super::Update(DeltaTime);
	FVector CurrentLocation = UpdatedComponent->GetGlobalTransform().GetLocation();
	FQuaternion CurrentQuat = UpdatedComponent->GetGlobalTransform().GetRotation();

	

	MoveUpdatedComponent(FVector::ZeroVector, CurrentQuat, false);
}

bool URotationMovementComponent::MoveUpdatedComponentImpl(const FVector& Delta, const FQuaternion& newRotation, bool bSweep)
{
	if (!UpdatedComponent)
	{
		return false;
	}

	const FVector NewLocation = UpdatedComponent->GetGlobalTransform().GetLocation() + Delta;
	UpdatedComponent->SetRelativeLocation(NewLocation);

	return true;
}
