#pragma once
#include "Runtime/CoreUObject/ULightComponent.h"

class UDirectionLightComponent : public ULightComponent
{
	GENERATED_BODY()
	DECLARE_UCLASS(UDirectionLightComponent, ULightComponent)

public:

	virtual void BuildConstants(FLightConstants& Constants) override;


private:


};