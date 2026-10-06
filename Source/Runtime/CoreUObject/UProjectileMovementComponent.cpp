#include "Runtime/CoreUObject/UProjectileMovementComponent.h"

IMPLEMENT_UCLASS(UProjectileMovementComponent, UMovementComponent)

void UProjectileMovementComponent::Update(float DeltaTime)
{
	Super::Update(DeltaTime);
	FQuaternion CurrentQuat = UpdatedComponent->GetGlobalTransform().GetRotation();

	MoveUpdatedComponent(Velocity * DeltaTime, CurrentQuat, false);
}

bool UProjectileMovementComponent::MoveUpdatedComponentImpl(const FVector& Delta, const FQuaternion& newRotation, bool bSweep)
{
	if (!UpdatedComponent) 
	{
		return false;
	}
	const FVector NewLocation = UpdatedComponent->GetGlobalTransform().GetLocation() + Delta;
	UpdatedComponent->SetRelativeLocation(NewLocation);

	return true;
}