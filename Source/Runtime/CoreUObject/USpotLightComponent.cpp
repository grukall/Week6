#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/CoreUObject/USpotLightComponent.h"

IMPLEMENT_UCLASS(USpotLightComponent, ULightComponent)

void USpotLightComponent::BuildConstants(FLightConstants& Constants)
{
	int32 NumLight = Constants.NumSpotLights++;
	SpotLight Light;
	Light.Intensity = GetIntensity();
	Light.AmbientIntensity = GetAmbientIntensity();
	Light.LightColor = GetLightColor();
	Light.Position = GetGlobalTransform().GetLocation();
	Light.LightDirection = GetLightDirection();
	Light.FallOffStart = GetFallOffStart();
	Light.FallOffEnd = GetFallOffEnd();
	Light.SpotPower = GetSpotPower();

	Constants.SpotLights[NumLight] = Light;
}