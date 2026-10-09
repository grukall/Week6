#include "Runtime/CoreUObject/URotationMovementComponent.h"

IMPLEMENT_UCLASS(URotationMovementComponent, UMovementComponent)


void URotationMovementComponent::Serialize(FArchive& Archive) const
{
    Super::Serialize(Archive);

    Archive.SetVector("RotationRate", RotationRate);
    Archive.SetVector("PivotTranslation", PivotTranslation);
    Archive.SetBool("bRotationInLocalSpace", bRotationInLocalSpace);
}

void URotationMovementComponent::Deserialize(const FArchive& Archive)
{
    Super::Deserialize(Archive);

    RotationRate = Archive.GetVector("RotationRate");
	PivotTranslation = Archive.GetVector("PivotTranslation");
	bRotationInLocalSpace = Archive.GetBool("bRotationInLocalSpace");
}

void URotationMovementComponent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	FVector CurrentLocation = UpdatedComponent->GetGlobalTransform().GetLocation();
	FVector DeltaDgree = RotationRate* DeltaTime;
	FQuaternion Quat = FQuaternion::FromEulerXYZDeg(DeltaDgree);

	MoveUpdatedComponent(FVector::ZeroVector, Quat, false);
}

bool URotationMovementComponent::MoveUpdatedComponentImpl(const FVector& Delta, const FQuaternion& newRotation, bool bSweep)
{
	if (!UpdatedComponent)
	{
		return false;
	}

	FQuaternion CurrentQuat = UpdatedComponent->GetGlobalTransform().GetRotation();
	UpdatedComponent->SetRelativeRotation(CurrentQuat * newRotation);

	return true;
}

