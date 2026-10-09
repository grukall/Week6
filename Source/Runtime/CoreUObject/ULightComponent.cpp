#include "Runtime/CoreUObject/ULightComponent.h"
#include "Runtime/Engine/UWorld.h"

IMPLEMENT_UCLASS(ULightComponent, USceneComponent)

void ULightComponent::Serialize(FArchive& Archive) const
{
	Super::Serialize(Archive);

	Archive.SetFloat("Intensity", Intensity);
	Archive.SetFloat("AmbientIntensity", AmbientIntensity);
	Archive.SetVector("LightColor", LightColor);
	Archive.SetVector("LightDirection", LightDirection);
	Archive.SetFloat("FallOffStart", FallOffStart);
	Archive.SetFloat("FallOffEnd", FallOffEnd);
	Archive.SetFloat("SpotPower", SpotPower);
}

void ULightComponent::Deserialize(const FArchive& Archive)
{
	Super::Deserialize(Archive);

	Intensity = Archive.GetFloat("Intensity");
	AmbientIntensity = Archive.GetFloat("AmbientIntensity");
	LightColor = Archive.GetVector("LightColor");
	LightDirection = Archive.GetVector("LightDirection");
	FallOffStart = Archive.GetFloat("FallOffStart");
	FallOffEnd = Archive.GetFloat("FallOffEnd");
	SpotPower = Archive.GetFloat("SpotPower");
}

void ULightComponent::Register(UWorld* InWorld)
{
	Super::Register(InWorld);
	InWorld->GetScene()->AddLightComponent(this);
}

void ULightComponent::Unregister()
{
    if (World)
    {
        if (FScene* Scene = World->GetScene())
        {
            Scene->RemoveLightComponent(this);
        }
    }

    Super::Unregister();
}

void ULightComponent::BuildConstants(FLightConstants& Constants)
{
}