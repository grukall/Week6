#include "FUObjectArray.h"

#include <cassert>

void FUObjectArray::AddObject(UObject* Object)
{
	Object->InternalIndex = Objects.Add(Object);
	Object->UUID = AcquireUUID();
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

bool FUObjectArray::IsValid(uint32 Index, uint32 UUID) const
{
	return UUID != 0 && Objects.IsValidIndex(Index) && Objects[Index]->UUID == UUID;
}
