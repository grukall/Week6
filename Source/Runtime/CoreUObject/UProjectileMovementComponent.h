#pragma once
#include "Runtime/CoreUObject/UMovementComponent.h"

class UProjectileMovementComponent : public UMovementComponent
{
	GENERATED_BODY()
	DECLARE_UCLASS(UProjectileMovementComponent, UMovementComponent)

protected:
	virtual void Update(float DeltaTime) override;
	virtual bool MoveUpdatedComponentImpl(const FVector& Delta, const FQuaternion& newRotation, bool bSweep) override;

};
