#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/CoreUObject/UDirectionLightComponent.h"

IMPLEMENT_UCLASS(UDirectionLightComponent, ULightComponent)

void UDirectionLightComponent::BuildConstants(FLightConstants& Constants) 
{
	int32 NumLight = Constants.NumDirLights++;
	DirectionLight DirLight;
	DirLight.Intensity = GetIntensity();
	DirLight.AmbientIntensity = GetAmbientIntensity();
	DirLight.LightColor = GetLightColor();
	DirLight.Position = GetGlobalTransform().GetLocation();
	DirLight.LightDirection = GetLightDirection();

	Constants.DirLights[NumLight] = DirLight;
}