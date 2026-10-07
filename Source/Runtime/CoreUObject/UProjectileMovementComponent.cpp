#include "Runtime/CoreUObject/UProjectileMovementComponent.h"

IMPLEMENT_UCLASS(UProjectileMovementComponent, UMovementComponent)

void UProjectileMovementComponent::Serialize(FArchive& Archive) const
{
	Super::Serialize(Archive);

	Archive.SetFloat("ProjectileGravityScale", ProjectileGravityScale);
	Archive.SetFloat("HomingAccelerationMagnitude", HomingAccelerationMagnitude);
	Archive.SetBool("bIsHomingProjectile", bIsHomingProjectile);

	if (HomingTargetComponent)
	{
		Archive.SetString("HomingTarget", HomingTargetComponent->GetName().ToString());
		const AActor* ParentHomingTarget = HomingTargetComponent->GetActorOwner();
		if (ParentHomingTarget)
		{
			Archive.SetString("ParentHomingTarget", ParentHomingTarget->GetGuid().ToString());
			return;
		}
	}
	Archive.SetNull("HomingTarget");
}

void UProjectileMovementComponent::Deserialize(const FArchive& Archive)
{
	Super::Deserialize(Archive);

	ProjectileGravityScale = Archive.GetFloat("ProjectileGravityScale");
	HomingAccelerationMagnitude = Archive.GetFloat("HomingAccelerationMagnitude");
	bIsHomingProjectile = Archive.GetBool("bIsHomingProjectile");
}

void UProjectileMovementComponent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!UpdatedComponent)
	{
		return;
	}
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

	if (bIsHomingProjectile && HomingTargetComponent)
	{
		FVector Location = UpdatedComponent->GetGlobalTransform().GetLocation();
		FVector TargetLocation = HomingTargetComponent->GetGlobalTransform().GetLocation();

		FVector Direction = TargetLocation - Location;

		if (Direction.Size() < 1.0f)
		{
			SetVelocity(FVector(0, 0, 0));
		}
	}

	MoveUpdatedComponent(Velocity * DeltaTime, CurrentQuat, false);
}

bool UProjectileMovementComponent::MoveUpdatedComponentImpl(const FVector& Delta, const FQuaternion& newRotation, bool bSweep)
{
	if (!UpdatedComponent) 
	{
		return false;
	}
	FTransform NewTransform = UpdatedComponent->GetGlobalTransform();
	NewTransform.SetLocation(NewTransform.GetLocation() + Delta);
	UpdatedComponent->SetRelativeTransformFromGlobal(NewTransform);

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
	FVector TargetLocation = HomingTargetComponent->GetGlobalTransform().GetLocation();
	
	Direction = TargetLocation - Location;
	
	Direction.Normalize();

	return Direction * HomingAccelerationMagnitude * DeltaTime;
}