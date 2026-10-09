#pragma once
#include "Runtime/CoreUObject/UActorComponent.h"
#include "Runtime/CoreUObject/UPrimitiveComponent.h"
#include "Runtime/Core/PointerTypes.h"
#include "Runtime/Math/FQuaternion.h"
#include "Runtime/Engine/FArchive.h"
#include "Runtime/Math/FVector.h"


class UMovementComponent : public UActorComponent
{
	GENERATED_BODY()
	DECLARE_UCLASS(UMovementComponent, UActorComponent)

public:
	virtual void Deserialize(const FArchive& Archive) override;
	virtual void Serialize(FArchive& Archive) const override;
	virtual void Initialize() override;
	virtual void Tick(float DeltaTime) override;
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