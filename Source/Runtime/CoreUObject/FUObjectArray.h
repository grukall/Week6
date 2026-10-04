#pragma once

#include "UObject.h"
#include "Runtime/Core/TSparseArray.h"
#include "Runtime/Core/IntTypes.h"
#include <utility>

class UObject;

// FUObjectArray의 슬롯 하나 (UE의 FUObjectItem에 대응).
// SerialNumber는 슬롯이 재사용될 때마다 새로 발급되어, 이전 객체를 가리키던 약참조를 무효화한다.
struct FUObjectItem
{
	UObject* Object = nullptr;
	uint32 SerialNumber = 0;

	FUObjectItem() = default;
	FUObjectItem(UObject* InObject, uint32 InSerialNumber) : Object(InObject), SerialNumber(InSerialNumber) {}
};

class FUObjectArray final
{
public:
	static FUObjectArray& Get() {
		static FUObjectArray Instance;
		return Instance;
	}

	// 사용 중인 객체의 수
	[[nodiscard]] uint32 GetNumObjects() const { return Objects.Num(); }
	[[nodiscard]] UObject* GetObjectByIndex(uint32 Index) const { return Objects.IsValidIndex(Index) ? Objects[Index].Object : nullptr; }

	// 슬롯의 시리얼 번호. 사용 중이 아닌 슬롯이면 0
	[[nodiscard]] uint32 GetSerialNumber(uint32 Index) const { return Objects.IsValidIndex(Index) ? Objects[Index].SerialNumber : 0u; }

	// 약참조 검증 (O(1)): Index 슬롯이 사용 중이고 시리얼 번호가 일치하면 유효하다.
	// 삭제된 객체의 포인터를 역참조하지 않도록 포인터가 아니라 슬롯 번호와 시리얼 번호로 검사한다.
	[[nodiscard]] bool IsValid(uint32 Index, uint32 SerialNumber) const;

	FUObjectArray(const FUObjectArray&) = delete;
	FUObjectArray& operator=(const FUObjectArray&) = delete;

	FUObjectArray(FUObjectArray&&) = delete;
	FUObjectArray& operator=(FUObjectArray&&) = delete;

	// 사용 중인 슬롯만 순회한다 (빈 슬롯은 TSparseArray 이터레이터가 건너뜀). 요소는 FUObjectItem이다.
	using TIterator = TSparseArray<FUObjectItem>::Iterator;
	TIterator begin() { return Objects.begin(); }
	TIterator end() { return Objects.end(); }

private:
	FUObjectArray() = default;
	~FUObjectArray() = default;

	void AddObject(UObject* Object);
	void RemoveObject(UObject* Object);

	[[nodiscard]] uint32 AcquireSerialNumber() { return NextSerialNumber++; }

	TSparseArray<FUObjectItem> Objects;
	uint32 NextSerialNumber = 1u;

	template <typename TObject, typename ... TArgs>
		requires std::derived_from<TObject, UObject>
	friend TObject* NewObject(TArgs&&... Args);
	friend UObject* NewObject(UClass* ClassType);
	friend void DestroyObject(UObject* Object);

	void DestroyObject(UObject* Object);
};
