#pragma once

#include "USceneComponent.h"
#include "Runtime/Rendering/ShaderConstants.h"

class UExponentialHeightFogComponent : public USceneComponent
{
	GENERATED_BODY()
	DECLARE_UCLASS(UExponentialHeightFogComponent, USceneComponent)
public:
	virtual void Initialize() override;
	virtual void Serialize(FArchive& Archive) const override;
	virtual void Deserialize(const FArchive& Archive) override;
	virtual void BuildConstants(FFogData& Constants) const;

	void Register(UWorld* InWorld) override;
	void Unregister() override;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay() override;

	void SetDensity(float InDensity) { Density = InDensity; }
	void SetHeightFalloff(float InHeightFalloff) { HeightFalloff = InHeightFalloff; }
	void SetStartDistance(float InStartDistance) { StartDistance = InStartDistance; }
	void SetCutoffDistance(float InCutoffDistance) { CutoffDistance = InCutoffDistance; }
	void SetMaxOpacity(float InMaxOpacity) { MaxOpacity = InMaxOpacity; }
	void  SetInscatteringColor(const FVector4& InInscatteringColor) { InscatteringColor = InInscatteringColor; }

	float GetDensity() const { return Density; }
	float GetHeightFalloff() const { return HeightFalloff; }
	float GetStartDistance() const { return StartDistance; }
	float GetCutoffDistance() const { return CutoffDistance; }
	float GetMaxOpacity() const { return MaxOpacity; }
	const FVector4& GetInscatteringColor() const { return InscatteringColor; }

private:
	

	FMatrix InverseVP;
	float Density = 0.5f;
	float HeightFalloff = 0.1f;
	float StartDistance = 10.0f;
	float CutoffDistance = 1000.0f;
	float MaxOpacity = 0.8f;
	FVector4 InscatteringColor{ 0.0f, 1.0f, 0.0f, 1.0f };
};