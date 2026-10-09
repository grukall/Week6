#include "pch.h"
#include "UExponentialHeightFogComponent.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include <Runtime/Rendering/ShaderConstants.h>
#include "Runtime/Engine/UWorld.h"
#include "Runtime/Engine/FScene.h"
#include "Runtime/Engine/FArchive.h"

IMPLEMENT_UCLASS(UExponentialHeightFogComponent, USceneComponent)

void UExponentialHeightFogComponent::Initialize()
{
	Super::Initialize();
	//Tick 해야하면 True로
	bTickEnabled = false;
	bTickInEditor = false;
}

void UExponentialHeightFogComponent::Serialize(FArchive& Archive) const
{
	Super::Serialize(Archive);

	Archive.SetFloat("Density", Density);
	Archive.SetFloat("HeightFalloff", HeightFalloff);
	Archive.SetFloat("StartDistance", StartDistance);
	Archive.SetFloat("CutoffDistance", CutoffDistance);
	Archive.SetFloat("MaxOpacity", MaxOpacity);
	Archive.SetVector4("InscatteringColor", InscatteringColor);
}

void UExponentialHeightFogComponent::Deserialize(const FArchive & Archive)
{
	Super::Deserialize(Archive);

	// 안개 값을 저장하기 전에 만든 씬에는 키가 없다. 없는 값은 기본값을 유지한다.
	if (!Archive.IsNull("Density")) { Density = Archive.GetFloat("Density"); }
	if (!Archive.IsNull("HeightFalloff")) { HeightFalloff = Archive.GetFloat("HeightFalloff"); }
	if (!Archive.IsNull("StartDistance")) { StartDistance = Archive.GetFloat("StartDistance"); }
	if (!Archive.IsNull("CutoffDistance")) { CutoffDistance = Archive.GetFloat("CutoffDistance"); }
	if (!Archive.IsNull("MaxOpacity")) { MaxOpacity = Archive.GetFloat("MaxOpacity"); }
	if (!Archive.IsNull("InscatteringColor")) { InscatteringColor = Archive.GetVector4("InscatteringColor"); }
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
