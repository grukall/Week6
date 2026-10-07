#pragma once

#include "Runtime/Actors/AActor.h"

class UBillBoardComp;

//Actor이지만 물리적 표현이 필요없는 액터들은 이 클래스를 상속받아서 샤용한다.
class AInfo : public AActor
{
	DECLARE_UCLASS(AInfo, AActor)
	GENERATED_BODY()

public:
	virtual void Initialize() override;

protected:
	UBillBoardComp* BillBoardComponent;
};