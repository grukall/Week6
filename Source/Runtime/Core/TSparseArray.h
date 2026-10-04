#pragma once

#include <cassert>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

#include "TArray.h"
#include "IntTypes.h"

// 빈 슬롯을 남겨 두어 요소의 인덱스가 수명 동안 바뀌지 않는 배열 (UE의 TSparseArray에 대응).
// 삭제된 슬롯은 free list(LIFO)로 연결되어 다음 Add/Emplace가 재사용한다.
//
// T는 자명하게 복사 가능한(trivially copyable) 타입이어야 한다.
// 슬롯 재할당(vector 확장)이 요소를 메모리째 옮기고, 삭제 시 소멸자를 호출하지 않기 때문이다.
template<typename T>
class TSparseArray
{
	static_assert(std::is_trivially_copyable_v<T>,
		"TSparseArray는 자명하게 복사 가능한 타입만 담을 수 있습니다. (UObject*, 구조체 등)");

public:
	class Iterator;
	class ConstIterator;

	TSparseArray() = default;
	~TSparseArray() = default;

	TSparseArray(const TSparseArray&) = default;
	TSparseArray& operator=(const TSparseArray&) = default;

	// 이동 후 원본이 "요소가 남아 있는 빈 벡터"가 되지 않도록 카운터를 초기화한다.
	TSparseArray(TSparseArray&& other) noexcept
		: mDatas(std::move(other.mDatas)), mFreeIndex(other.mFreeIndex), mNumElements(other.mNumElements)
	{
		other.mDatas.clear();
		other.mFreeIndex = -1;
		other.mNumElements = 0;
	}

	TSparseArray& operator=(TSparseArray&& other) noexcept
	{
		if (this != &other)
		{
			mDatas = std::move(other.mDatas);
			mFreeIndex = other.mFreeIndex;
			mNumElements = other.mNumElements;

			other.mDatas.clear();
			other.mFreeIndex = -1;
			other.mNumElements = 0;
		}
		return *this;
	}

	// 사용 중인 슬롯만 접근해야 한다. (디버그 빌드에서 확인)
	T& operator[](uint32 index);
	const T& operator[](uint32 index) const;

	Iterator begin();
	Iterator end();

	ConstIterator begin() const;
	ConstIterator end() const;

	uint32 Add(const T& data) { return Emplace(data); }

	// 가장 최근에 비워진 슬롯(없으면 새 슬롯)에 T를 제자리 생성하고 인덱스를 반환한다.
	template<typename... TArgs>
	uint32 Emplace(TArgs&&... Args)
	{
		uint32 Index;
		if (mFreeIndex != -1)
		{
			Index = static_cast<uint32>(mFreeIndex);
			mFreeIndex = mDatas[Index].second.NextFreeIndex;
		}
		else
		{
			mDatas.emplace_back();
			Index = static_cast<uint32>(mDatas.size() - 1);
		}

		mDatas[Index].first = true;

		// placement new: new(주소) T(인자...) - 메모리 할당 없이 해당 주소에 T를 생성한다.
		// ::new를 쓰는 이유: T가 클래스 범위 operator new를 가지면 일반 new(주소)가 가려지기 때문
		::new (static_cast<void*>(&mDatas[Index].second.Element)) T(std::forward<TArgs>(Args)...);
		++mNumElements;
		return Index;
	}

	// 사용 중인 요소의 개수
	int32 Num() const;
	// 빈 슬롯을 포함한 슬롯의 개수 (유효한 인덱스의 상한)
	int32 GetMaxIndex() const;
	// 재할당 없이 담을 수 있는 슬롯 수
	int32 Capacity() const;
	void Reserve(uint32 capacity);

	bool IsEmpty() const;
	// 범위 안이고 사용 중인 슬롯인지
	bool IsValidIndex(uint32 index) const;

	// 모든 요소를 지우고 메모리를 해제한다. ExpectedNumElements만큼은 미리 확보한다.
	void Empty(int32 ExpectedNumElements = 0);
	// 모든 요소를 지우되 할당된 메모리는 유지한다.
	void Reset();
	void RemoveAt(uint32 index);

	TArray<T> ToTArray() const;

	class Iterator
	{
		TSparseArray* Owner;
		std::size_t Index;

		void SkipEmpty();

	public:
		Iterator(TSparseArray* owner, std::size_t index);

		T& operator*() const;
		T* operator->() const;

		Iterator& operator++();
		bool operator==(const Iterator& other) const;
		bool operator!=(const Iterator& other) const;
	};

	class ConstIterator
	{
		const TSparseArray* Owner;
		std::size_t Index;

		void SkipEmpty();

	public:
		ConstIterator(const TSparseArray* owner, std::size_t index);

		const T& operator*() const;
		const T* operator->() const;

		ConstIterator& operator++();
		bool operator==(const ConstIterator& other) const;
		bool operator!=(const ConstIterator& other) const;
	};

	// 사용 중이면 Element, 비어 있으면 다음 빈 슬롯 번호(NextFreeIndex)를 담는다.
	// 사용자 정의 기본 생성자: T에 기본 멤버 초기화 값이 있어도 union의 기본 생성자가 삭제되지 않게 한다.
	union Slot
	{
		T Element;
		int32 NextFreeIndex;

		Slot() {}
	};

private:
	//TODO : union으로 아낀 메모리를 pair<bool, Slot>으로 다시 쓰고 있음, bool을 TbitArray로 분리하여 관리하기
	std::vector<std::pair<bool, Slot>> mDatas;
	int32 mFreeIndex = -1;
	int32 mNumElements = 0;
};

#include "TSparseArray.inl"
