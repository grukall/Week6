#pragma once

#include "USceneComponent.h"

class UExponentialHeightFogComponent : public USceneComponent
{
	GENERATED_BODY()
	DECLARE_UCLASS(UExponentialHeightFogComponent, USceneComponent)
public:
	virtual void Initialize() override;
	virtual void Serialize(FArchive& Archive) const override;
	virtual void Deserialize(const FArchive& Archive) override;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay() override;
};