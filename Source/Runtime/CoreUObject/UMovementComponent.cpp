#include "Runtime/CoreUObject/UMovementComponent.h"

IMPLEMENT_UCLASS(UMovementComponent, UActorComponent)

void UMovementComponent::Serialize(FArchive& Archive) const
{
	Super::Serialize(Archive);
	
	Archive.SetVector("Velocity", Velocity);
	Archive.SetBool("bSweep", bSweep);


	//USceneComponent* UpdatedComponent = nullptr;
	//UPrimitiveComponent* UpdatedPrimitive = nullptr;
}

void UMovementComponent::Deserialize(const FArchive& Archive)
{
	Super::Deserialize(Archive);

	Velocity = Archive.GetVector("Velocity");
	bSweep = Archive.GetBool("bSweep");
}

void UMovementComponent::Initialize()
{
	Super::Initialize();

	if (AActor* Owner = GetActorOwner())
	{
		UpdatedComponent = Owner->GetRootComponent();
		bTickEnabled = true;
		//bTickInEditor = true;
	}

}

void UMovementComponent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

bool UMovementComponent::MoveUpdatedComponent(const FVector& Delta, const FQuaternion& newRotation, bool bSweep)
{
	return MoveUpdatedComponentImpl(Delta, newRotation, bSweep);
}

bool UMovementComponent::MoveUpdatedComponentImpl(const FVector& Delta, const FQuaternion& newRotation, bool bSweep)
{
	return true;
}

