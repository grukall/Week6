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
	Light->SetAmbientInensity(0.3f);
	Light->SetInensity(1.0f);
	Light->SetFallOffStart(0.0f);
	Light->SetFallOffEnd(100.0f);
	Light->SetLightColor(FVector(1.0f, 1.0f, 1.0f));

	bTickEnabled = true;
	AddComponent(Light);
}

void AAppleBittenActor::Update(float DeltaTime)
{
	Super::Update(DeltaTime);
}
