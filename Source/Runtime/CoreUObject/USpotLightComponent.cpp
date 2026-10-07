#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/CoreUObject/USpotLightComponent.h"

IMPLEMENT_UCLASS(USpotLightComponent, ULightComponent)

void USpotLightComponent::BuildConstants(FLightConstants& Constants)
{
	// 배열이 가득 차면 더 넣지 않는다.
	if (Constants.NumSpotLights >= MAXLIGHTS)
	{
		return;
	}

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