#include "UObject.h"
#include "Runtime/Engine/FArchive.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Core/FMemory.h"

IMPLEMENT_ROOT_UCLASS(UObject)
UCLASS_META(UObject, DisplayName, "Object")

void UObject::Initialize()
{
}

void UObject::Release()
{
}

void UObject::Serialize(FArchive& Archive) const
{
	Archive.SetInt32("UUID", UUID);
	Archive.SetString("Type", GetClass()->GetUClassName());
}

void UObject::Deserialize(const FArchive& Archive)
{
	UUID = Archive.GetInt32("UUID");
}

//// 서브 오브젝트를 복제하는 함수 (하위 클래스에서 재정의 가능)
void UObject::DuplicateSubObjects()
{

	//if (SubObjectA)
	//	SubObjectA = SubObjectA->Duplicate();

	//if (SubObjectB)
	//	SubObjectB = SubObjectB->Duplicate();
}

 
// 현재 오브젝트를 복제하는 함수
UObject* UObject::Duplicate()
{
	// 새 객체 생성
	UObject* TempObject = NewObject<UObject>();
	TempObject->DuplicateSubObjects();

	return TempObject;
}

void* UObject::operator new(std::size_t Size)
{
	// void* Memory = ::operator new(Size);
	
	void* Memory = FMemory::Malloc(Size);
	
	TotalAllocationBytes += Size;
	++TotalAllocationCount;

	return Memory;
}

void UObject::operator delete(void* Memory, std::size_t Size) noexcept
{
	if (Memory == nullptr) return;

	TotalAllocationBytes -= Size;
	--TotalAllocationCount;

	//::operator delete(Memory);

	FMemory::Free(Memory);
}

void* UObject::operator new(std::size_t Size, std::align_val_t Alignment)
{
	// void* Memory = ::operator new(Size, Alignment);
	void* Memory = FMemory::Malloc(Size, static_cast<size_t>(Alignment));

	TotalAllocationBytes += Size;
	++TotalAllocationCount;

	return Memory;
}

void UObject::operator delete(void* Memory, std::size_t Size, std::align_val_t Alignment) noexcept
{
	if (Memory == nullptr) return;

	TotalAllocationBytes -= Size;
	--TotalAllocationCount;

	//::operator delete(Memory, Alignment);
	FMemory::Free(Memory, static_cast<size_t>(Alignment));
}
