#pragma once
#include "Runtime/CoreUObject/ULightComponent.h"

class USpotLightComponent : public ULightComponent
{
	GENERATED_BODY()
	DECLARE_UCLASS(USpotLightComponent, ULightComponent)

public:

	virtual void BuildConstants(FLightConstants& Constants) override
	{
		int32 NumLight = Constants.NumSpotLights++;
		SpotLight Light;
		Light.Intensity = GetInensity();
		Light.AmbientIntensity = GetAmbientInensity();
		Light.LightColor = GetLightColor();
		Light.Position = GetGlobalTransform().GetLocation();
		Light.LightDirection = GetLightDirection();
		Light.FallOffStart = GetFallOffStart();
		Light.FallOffEnd = GetFallOffEnd();
		Light.SpotPower = GetSpotPower();

		Constants.SpotLights[NumLight] = Light;
	}


private:


};
