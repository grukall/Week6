#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/CoreUObject/UPointLightComponent.h"

IMPLEMENT_UCLASS(UPointLightComponent, ULightComponent)

void UPointLightComponent::BuildConstants(FLightConstants& Constants)
{
	int32 NumLight = Constants.NumPointLights++;
	PointLight Light;
	Light.Intensity = GetIntensity();
	Light.AmbientIntensity = GetAmbientIntensity();
	Light.LightColor = GetLightColor();
	Light.Position = GetGlobalTransform().GetLocation();
	Light.FallOffStart = GetFallOffStart();
	Light.FallOffEnd = GetFallOffEnd();

	Constants.PointLights[NumLight] = Light;
}
