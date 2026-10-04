#pragma once
#include "Runtime/CoreUObject/ULightComponent.h"

class USpotLightComponent : public ULightComponent
{
	GENERATED_BODY()
	DECLARE_UCLASS(USpotLightComponent, ULightComponent)

public:

	virtual void BuildConstants(FLightConstants& Constants) override
	{
		int32 NumLight = Constants.NumPointLights++;
		PointLight Light;
		Light.Intensity = GetInensity();
		Light.AmbientIntensity = GetAmbientInensity();
		Light.LightColor = GetLightColor();
		Light.Position = GetGlobalTransform().GetLocation();
		Light.FallOffStart = GetFallOffStart();
		Light.FallOffEnd = GetFallOffEnd();

		Constants.PointLights[NumLight] = Light;
	}


private:


};
