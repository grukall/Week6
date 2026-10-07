#include "pch.h"
#include "AInfo.h"
#include "Runtime/CoreUObject/UBillBoardComp.h"

IMPLEMENT_UCLASS(AInfo, AActor)

void AInfo::Initialize()
{
	Super::Initialize();

	BillBoardComponent = NewObject<UBillBoardComp>();
	AddComponent(BillBoardComponent);

	bTickEnabled = false;
}
