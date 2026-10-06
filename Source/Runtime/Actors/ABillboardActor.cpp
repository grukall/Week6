#include "ABillboardActor.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/CoreUObject/UBillboardComp.h"
#include "Runtime/CoreUObject/UPointLightComponent.h"
#include "Runtime/CoreUObject/UMovementComponent.h"
#include "Runtime/CoreUObject/UProjectileMovementComponent.h"
#include "Runtime/Asset/FAssetRegistry.h"
#include "Runtime/Math/FVector.h"

IMPLEMENT_UCLASS(ABillboardActor, AActor)
UCLASS_META(ABillboardActor, DisplayName, "Billboard Actor")

ABillboardActor::ABillboardActor()
{
	// 기본 큐브 컴포넌트 장착
	UBillBoardComp* Object = NewObject<UBillBoardComp>();
	SetRootComponent(Object);

	FAssetRegistry& Registry = FAssetRegistry::GetInstance();
	Object->SetTexture(Registry.Get<UTexture>("Texture/Space.json"));

	/*ULightComponent* Light = NewObject<UPointLightComponent>();
	Light->SetAmbientInensity(1.0f);
	Light->SetInensity(5.0f);
	Light->SetFallOffStart(0.0f);
	Light->SetFallOffEnd(1000.0f);
	Light->SetLightColor(FVector(1.0f, 1.0f, 1.0f));

	AddComponent(Light);*/

	UMovementComponent* Move = NewObject<UProjectileMovementComponent>();
	Move->SetVelocity(FVector(1.0f, 0.0f, 0.0f));
	bTickEnabled = true;
	AddComponent(Move);
}

void ABillboardActor::Initialize()
{
	Super::Initialize();
	bTickEnabled = true;
}

UBillBoardComp* ABillboardActor::GetBillboardComponent() const
{
	return RootComponent ? RootComponent->Cast<UBillBoardComp>() : nullptr;
}
