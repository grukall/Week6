#include "Runtime/CoreUObject/UProjectileMovementComponent.h"

IMPLEMENT_UCLASS(UProjectileMovementComponent, UMovementComponent)

void UProjectileMovementComponent::Update(float DeltaTime)
{
	Super::Update(DeltaTime);
	FQuaternion CurrentQuat = UpdatedComponent->GetGlobalTransform().GetRotation();

	if (ShouldApplyGravity())
	{
		float X = Velocity.X;
		float Y = Velocity.Y;
		float Z = Velocity.Z - ProjectileGravityScale * DeltaTime;

		Velocity = { X, Y, Z };
	}

	if (bIsHomingProjectile)
	{
		Velocity += ComputeHomingAcceleration(Velocity, DeltaTime);
	}
	const float MaxSpeed = 500.0f;
	const float Speed = Velocity.Size();
	
	if (Speed > MaxSpeed || Speed < -MaxSpeed)
	{
		Velocity.Normalize();
		Velocity *= MaxSpeed;
	}

	FVector Location = UpdatedComponent->GetGlobalTransform().GetLocation();
	FVector TargetLocation = HomingTargetComponent->GetRelativeLocation();

	FVector Direction = TargetLocation - Location;

	if (Direction.Size() < 1.0f )
	{
		SetVelocity(FVector(0, 0, 0));
	}


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

FVector UProjectileMovementComponent::ComputeHomingAcceleration(const FVector& InVelocity, float DeltaTime) const
{
	if (!HomingTargetComponent)
	{
		return FVector::ZeroVector;
	}

	FVector Direction;

	FVector Location = UpdatedComponent->GetGlobalTransform().GetLocation();
	FVector TargetLocation = HomingTargetComponent->GetRelativeLocation();
	
	Direction = TargetLocation - Location;
	
	Direction.Normalize();

	return Direction * HomingAccelerationMagnitude * DeltaTime;
}