#include "pch.h"
#include "AExponentialHeightFog.h"
#include "Runtime/CoreUObject/UExponentialHeightFogComponent.h"
#include "Runtime/CoreUObject/UBillBoardComp.h"
#include "Runtime/Asset/FAssetRegistry.h"

IMPLEMENT_UCLASS(AExponentialHeightFog, AInfo)

void AExponentialHeightFog::Initialize()
{
	Super::Initialize();

	FAssetRegistry& Registry = FAssetRegistry::GetInstance();
	BillBoardComponent->SetTexture(Registry.Get<UTexture>("Texture/ExponentialHeightFog_64x.json"));

	ExponentialHeightFogComponent = NewObject<UExponentialHeightFogComponent>();
	AddComponent(ExponentialHeightFogComponent);
}
