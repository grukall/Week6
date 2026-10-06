#pragma once
#include "Runtime/CoreUObject/USceneComponent.h"
#include "Runtime/CoreUObject/UPrimitiveComponent.h"
#include "Runtime/Core/PointerTypes.h"
#include "Runtime/Math/FQuaternion.h"
#include "Runtime/Math/FVector.h"


class UMovementComponent : public USceneComponent
{
	GENERATED_BODY()
	DECLARE_UCLASS(UMovementComponent, USceneComponent)

public:
	virtual void Initialize() override;
	virtual void Update(float DeltaTime) override;
	virtual void SetUpdatedComponent(USceneComponent* InUpdatedComponent) { UpdatedComponent = InUpdatedComponent; }
	USceneComponent* GetUpdatedComponet() { return UpdatedComponent; }

	void SetVelocity(FVector InVel) { Velocity = InVel; }
	FVector GetVelocity() { return Velocity; }

	bool MoveUpdatedComponent(const FVector& Delta, const FQuaternion& newRotation, bool bSweep);

	virtual float GetGravityZ() { return 0.0f; }

protected:
	virtual bool MoveUpdatedComponentImpl(const FVector& Delta, const FQuaternion& newRotation, bool bSweep);

public:
	USceneComponent* UpdatedComponent = nullptr;
	UPrimitiveComponent* UpdatedPrimitive = nullptr;
	FVector Velocity{};

	bool bSweep;

};