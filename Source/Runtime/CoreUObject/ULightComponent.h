#pragma once
#include "Runtime/CoreUObject/USceneComponent.h"
#include "Runtime/Rendering/ShaderConstants.h"

class UWorld;

class ULightComponent : public USceneComponent
{
	GENERATED_BODY()
	DECLARE_UCLASS(ULightComponent, USceneComponent)

public:
	void Register(UWorld* InWorld) override;
	void Unregister() override;

	virtual void BuildConstants(FLightConstants& Constants);

	void SetIntensity(const float& InIntensity) { Intensity = InIntensity; }
	float GetIntensity() { return Intensity; }

	void SetAmbientIntensity(const float& InAmbientIntensity) { AmbientIntensity = InAmbientIntensity; }
	float GetAmbientIntensity() { return AmbientIntensity; }
	
	void SetFallOffStart(const float& FallStart) { FallOffStart= FallStart; }
	float GetFallOffStart() { return FallOffStart; }
	
	void SetFallOffEnd(const float& FallEnd) { FallOffEnd = FallEnd; }
	float GetFallOffEnd() { return FallOffEnd; }
	
	void SetSpotPower(const float& InSpotPower) { SpotPower = InSpotPower; }
	float GetSpotPower() { return SpotPower; }
	
	void SetLightColor(const FVector& Color) { LightColor = Color; }
	FVector GetLightColor() { return LightColor; }
	
	void SetLightDirection (const FVector& LightDir) { LightDirection = LightDir; }
	FVector GetLightDirection() { return LightDirection; }

protected:
	/*static int32 NumDirection;
	static int32 NumSpot;
	static int32 NumPoint;*/

private:
	float Intensity = 1.0f;
	float AmbientIntensity = 0.2f;

	FVector LightColor{ 1.0f, 1.0f, 1.0f }; 
	FVector LightDirection{ 0.0f, 0.0f, 0.0f }; 

	float FallOffStart = 0.0f;	
	float FallOffEnd = 0.0f;	
	float SpotPower = 0.0f;

};