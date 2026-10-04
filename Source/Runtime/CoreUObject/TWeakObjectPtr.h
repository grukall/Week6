#pragma once

#include "FUObjectArray.h"
#include <cstddef>
#include <concepts>

class UObject;

// 객체 해제 시 자동으로 널포인터를 반환하는 약한 참조 포인터
// 슬롯 번호(Index)와 UUID로 O(1) 검증한다. (삭제된 객체의 포인터는 역참조하지 않는다)
//template<typename U = T>
//	requires std::derived_from<U, UObject> 이거 여기에 선언하면 다른곳에서 순환오류 날수도있음
template<typename T>
class TWeakObjectPtr
{
private:
	mutable T* RawPtr = nullptr;
	mutable uint32 ObjectIndex = 0;
	mutable uint32 ObjectUUID = 0;

public:
	TWeakObjectPtr() = default;

	template<typename U = T>
		requires std::derived_from<U, UObject>
	TWeakObjectPtr(T* InPtr)
		: RawPtr(InPtr)
	{
		Capture(InPtr);
	}

	TWeakObjectPtr(std::nullptr_t)
		: RawPtr(nullptr), ObjectIndex(0), ObjectUUID(0)
	{
	}

	template<typename U = T>
		requires std::derived_from<U, UObject>
	TWeakObjectPtr& operator=(T* InPtr)
	{
		RawPtr = InPtr;
		Capture(InPtr);
		return *this;
	}

	TWeakObjectPtr& operator=(std::nullptr_t)
	{
		Reset();
		return *this;
	}

	T* Get() const
	{
		if (RawPtr)
		{
			// 생성자 실행 중 등록되어 식별자가 비어있던 경우 갱신
			if (ObjectUUID == 0)
			{
				Capture(RawPtr);
			}

			if (FUObjectArray::Get().IsValid(ObjectIndex, ObjectUUID))
			{
				return RawPtr;
			}
			// 삭제된 객체인 경우 포인터 초기화
			Reset();
		}
		return nullptr;
	}

	T* operator->() const { return Get(); }
	operator T*() const { return Get(); }
	explicit operator bool() const { return Get() != nullptr; }

	bool operator==(const TWeakObjectPtr& Other) const { return Get() == Other.Get(); }
	bool operator!=(const TWeakObjectPtr& Other) const { return Get() != Other.Get(); }
	bool operator==(const T* Other) const { return Get() == Other; }
	bool operator!=(const T* Other) const { return Get() != Other; }

	bool IsValid() const { return Get() != nullptr; }
	void Reset() const { RawPtr = nullptr; ObjectIndex = 0; ObjectUUID = 0; }

private:
	void Capture(T* InPtr) const
	{
		ObjectIndex = InPtr ? InPtr->GetInternalIndex() : 0;
		ObjectUUID = InPtr ? InPtr->GetUUID() : 0;
	}
};
