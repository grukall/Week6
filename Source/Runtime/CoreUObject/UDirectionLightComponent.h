#pragma once
#include "Runtime/CoreUObject/ULightComponent.h"

class UDirectionLightComponent : public ULightComponent
{
	GENERATED_BODY()
	DECLARE_UCLASS(UDirectionLightComponent, ULightComponent)

public:

	virtual void BuildConstants(FLightConstants& Constants) override
	{
		int32 NumLight = Constants.NumDirLights++;
		DirectionLight DirLight;
		DirLight.Intensity = GetInensity();
		DirLight.AmbientIntensity = GetAmbientInensity();
		DirLight.LightColor = GetLightColor();
		DirLight.Position = GetGlobalTransform().GetLocation();
		DirLight.LightDirection = GetLightDirection();
		
		Constants.DirLights[NumLight] = DirLight;
	}


private:


};