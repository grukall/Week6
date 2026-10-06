#pragma once
#include "Runtime/CoreUObject/UMovementComponent.h"

class UProjectileMovementComponent : public UMovementComponent
{
	GENERATED_BODY()
	DECLARE_UCLASS(UProjectileMovementComponent, UMovementComponent)

protected:
	virtual void Update(float DeltaTime) override;
	virtual bool MoveUpdatedComponentImpl(const FVector& Delta, const FQuaternion& newRotation, bool bSweep) override;
	virtual float GetGravityZ() override { return ProjectileGravityScale; }
public:
	bool ShouldApplyGravity() { return ProjectileGravityScale != 0; }

	virtual FVector ComputeHomingAcceleration(const FVector& InVelocity, float DeltaTime) const;

public:
	float ProjectileGravityScale = 0;
	float HomingAccelerationMagnitude = 0;

	bool bIsHomingProjectile = false;
	
	USceneComponent* HomingTargetComponent = nullptr;
};
