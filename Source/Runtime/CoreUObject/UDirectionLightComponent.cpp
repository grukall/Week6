#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/CoreUObject/UDirectionLightComponent.h"

IMPLEMENT_UCLASS(UDirectionLightComponent, ULightComponent)

void UDirectionLightComponent::BuildConstants(FLightConstants& Constants) 
{
	// 배열이 가득 차면 더 넣지 않는다.
	if (Constants.NumDirLights >= MAXLIGHTS)
	{
		return;
	}

	int32 NumLight = Constants.NumDirLights++;
	DirectionLight DirLight;
	DirLight.Intensity = GetIntensity();
	DirLight.AmbientIntensity = GetAmbientIntensity();
	DirLight.LightColor = GetLightColor();
	DirLight.Position = GetGlobalTransform().GetLocation();
	DirLight.LightDirection = GetLightDirection();

	Constants.DirLights[NumLight] = DirLight;
}