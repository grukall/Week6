#include "AAppleBittenActor.h"

#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/CoreUObject/Mesh/UStaticMeshComponent.h"
#include "Runtime/Asset/FAssetRegistry.h"

IMPLEMENT_UCLASS(AAppleBittenActor, AActor)
UCLASS_META(AAppleBittenActor, DisplayName, "Apple Bitten Actor")

AAppleBittenActor::AAppleBittenActor()
{	
	AppleStaticMeshComp = NewObject<UStaticMeshComponent>();
	SetRootComponent(AppleStaticMeshComp);

	FAssetRegistry& Registry = FAssetRegistry::GetInstance();
	AppleStaticMeshComp->SetMesh(Registry.Get<UStaticMesh>("StaticMesh/JungleApple/Apple_Bitten.json"));
}

void AAppleBittenActor::DuplicateSubObjects()
{
	Super::DuplicateSubObjects();
	RemapComponent(AppleStaticMeshComp);
}

void AAppleBittenActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
