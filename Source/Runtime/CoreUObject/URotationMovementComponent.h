#pragma once
#include "Runtime/CoreUObject/UMovementComponent.h"

class URotationMovementComponent : public UMovementComponent
{
	GENERATED_BODY()
	DECLARE_UCLASS(URotationMovementComponent, UMovementComponent)

protected:
	virtual void Tick(float DeltaTime) override;
	virtual bool MoveUpdatedComponentImpl(const FVector& Delta, const FQuaternion& newRotation, bool bSweep) override;
	
public:
	void SetRotationRate(const FVector& Rate) { RotationRate = Rate; }
	FVector GetRotationRate() { return RotationRate; }

public:
	
	FVector RotationRate;
	FVector PivotTranslation;
	bool bRotationInLocalSpace;

};

