#pragma once
#include "Runtime/CoreUObject/UMovementComponent.h"

class URotationMovementComponent : public UMovementComponent
{
	GENERATED_BODY()
	DECLARE_UCLASS(URotationMovementComponent, UMovementComponent)

protected:
	virtual void Update(float DeltaTime) override;
	virtual bool MoveUpdatedComponentImpl(const FVector& Delta, const FQuaternion& newRotation, bool bSweep) override;
	
public:
	void SetRptationRate(const FVector& Rate) { RotationRate = Rate; }

public:
	
	FVector RotationRate;
	FVector PivotTranslation;
	bool bRotationInLocalSpace;

};

