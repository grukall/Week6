#pragma once
#include "Runtime/CoreUObject/ULightComponent.h"

class UPointLightComponent : public ULightComponent
{
	GENERATED_BODY()
	DECLARE_UCLASS(UPointLightComponent, ULightComponent)

public:
	virtual void BuildConstants(FLightConstants& Constants) override;

private:


};
