#include "FUObjectArray.h"

#include <cassert>

void FUObjectArray::AddObject(UObject* Object)
{
	Object->InternalIndex = Objects.Emplace(Object, AcquireSerialNumber());
}

void FUObjectArray::RemoveObject(UObject* Object)
{
	const uint32 Index = Object->InternalIndex;
	Objects.RemoveAt(Index);
}

void FUObjectArray::DestroyObject(UObject* Object) {
	if (Object == nullptr) return;

	Object->Release();
	RemoveObject(Object);
	delete Object; // 오버라이드해서 통계 구현 필요
}

bool FUObjectArray::IsValid(uint32 Index, uint32 SerialNumber) const
{
	// 슬롯이 비어 있거나 다른 객체로 재사용되었다면 시리얼 번호가 달라서 무효가 된다.
	return SerialNumber != 0 && Objects.IsValidIndex(Index) && Objects[Index].SerialNumber == SerialNumber;
}
