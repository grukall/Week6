#pragma once

#include "AActor.h"

class UStaticMeshComponent;

class AAppleNormalActor : public AActor
{
	DECLARE_UCLASS(AAppleNormalActor, AActor)
	GENERATED_BODY()

public:
	explicit AAppleNormalActor();
	void DuplicateSubObjects() override;

	virtual void Tick(float DeltaTime) override;

private:
	UStaticMeshComponent* AppleStaticMeshComp;
};
