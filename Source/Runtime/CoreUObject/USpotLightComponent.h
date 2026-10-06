#pragma once
#include "Runtime/CoreUObject/ULightComponent.h"

class USpotLightComponent : public ULightComponent
{
	GENERATED_BODY()
	DECLARE_UCLASS(USpotLightComponent, ULightComponent)

public:

	virtual void BuildConstants(FLightConstants& Constants) override;


private:


};
