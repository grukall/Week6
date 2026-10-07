#include "AAppleBittenActor.h"

#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/CoreUObject/Mesh/UStaticMeshComponent.h"
#include "Runtime/CoreUObject/UPointLightComponent.h"

#include "Runtime/Asset/FAssetRegistry.h"

IMPLEMENT_UCLASS(AAppleBittenActor, AActor)
UCLASS_META(AAppleBittenActor, DisplayName, "Apple Bitten Actor")

AAppleBittenActor::AAppleBittenActor()
{	
	AppleStaticMeshComp = NewObject<UStaticMeshComponent>();
	SetRootComponent(AppleStaticMeshComp);

	FAssetRegistry& Registry = FAssetRegistry::GetInstance();
	AppleStaticMeshComp->SetMesh(Registry.Get<UStaticMesh>("StaticMesh/JungleApple/Apple_Bitten.json"));

	ULightComponent* Light = NewObject<UPointLightComponent>();
	Light->SetAmbientIntensity(0.3f);
	Light->SetIntensity(1.0f);
	Light->SetFallOffStart(0.0f);
	Light->SetFallOffEnd(100.0f);
	Light->SetLightColor(FVector(1.0f, 1.0f, 1.0f));

	bTickEnabled = true;
	AddComponent(Light);
}

void AAppleBittenActor::Tick(float DeltaTime, ELevelTick eTickType)
{
	Super::Tick(DeltaTime, eTickType);
}