#include "pch.h"
#include "UExponentialHeightFogComponent.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include <Runtime/Rendering/ShaderConstants.h>
#include "Runtime/Engine/UWorld.h"
#include "Runtime/Engine/FScene.h"

IMPLEMENT_UCLASS(UExponentialHeightFogComponent, USceneComponent)

void UExponentialHeightFogComponent::Initialize()
{
	//Tick 해야하면 True로
	bTickEnabled = false;
	bTickInEditor = false;
}

void UExponentialHeightFogComponent::Serialize(FArchive& Archive) const
{
	Super::Serialize(Archive);
		
	//직렬화 해야 하는 정보를 여기에 작성 ex) 안개 밀도 등,
}

void UExponentialHeightFogComponent::Deserialize(const FArchive & Archive)
{
	Super::Deserialize(Archive);

	//역직렬화 해야 하는 정보를 여기에 작성
}

void UExponentialHeightFogComponent::Register(UWorld* InWorld)
{
	Super::Register(InWorld);
	InWorld->GetScene()->AddFogComponent(this);
}

void UExponentialHeightFogComponent::Unregister()
{
	if (World)
	{
		if (FScene* Scene = World->GetScene())
		{
			Scene->RemoveFogComponent(this);
		}
	}

	Super::Unregister();
}

void UExponentialHeightFogComponent::BuildConstants(FFogData& Constants) const
{
	Constants.Density = GetDensity();
	Constants.HeightFalloff = GetHeightFalloff();
	Constants.StartDistance = GetStartDistance();
	Constants.CutoffDistance = GetCutoffDistance();
	Constants.MaxOpacity = GetMaxOpacity();
	Constants.InscatteringColor = GetInscatteringColor();
}

void UExponentialHeightFogComponent::BeginPlay()
{
	Super::BeginPlay();

	//시작 시 해야할 일들을 여기에
}

void UExponentialHeightFogComponent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void UExponentialHeightFogComponent::EndPlay()
{
	Super::EndPlay();
}
