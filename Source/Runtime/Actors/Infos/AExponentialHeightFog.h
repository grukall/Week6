#pragma once

#include "AInfo.h"

class UExponentialHeightFogComponent;

class AExponentialHeightFog : public AInfo
{
	DECLARE_UCLASS(AExponentialHeightFog, AInfo)
	GENERATED_BODY()

public:
	virtual void Initialize() override;

protected:

	//TODO : 액터 initialize에서 생성된 컴포넌트는 Actor에 의해 연결/해제 되므로 포인터로 갖고 있어도 문제없다.
	// 하지만 다른 포인터는 직렬화되지 않으므로 사용에 주의해야 한다.
	UExponentialHeightFogComponent* ExponentialHeightFogComponent;
};