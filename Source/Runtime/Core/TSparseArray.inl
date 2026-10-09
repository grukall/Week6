
template<typename T>
T& TSparseArray<T>::operator[](uint32 index)
{
	assert(index < mDatas.size());
	assert(mDatas[index].first && "빈 슬롯에 접근했습니다.");
	return mDatas[index].second.Element;
}

template<typename T>
const T& TSparseArray<T>::operator[](uint32 index) const
{
	assert(index < mDatas.size());
	assert(mDatas[index].first && "빈 슬롯에 접근했습니다.");
	return mDatas[index].second.Element;
}

template<typename T>
typename TSparseArray<T>::Iterator TSparseArray<T>::begin()
{
	return TSparseArray<T>::Iterator(this, 0);
}

template<typename T>
typename TSparseArray<T>::Iterator TSparseArray<T>::end()
{
	return TSparseArray<T>::Iterator(this, mDatas.size());
}

template<typename T>
typename TSparseArray<T>::ConstIterator TSparseArray<T>::begin() const
{
	return TSparseArray<T>::ConstIterator(this, 0);
}

template<typename T>
typename TSparseArray<T>::ConstIterator TSparseArray<T>::end() const
{
	return TSparseArray<T>::ConstIterator(this, mDatas.size());
}

template<typename T>
int32 TSparseArray<T>::Num() const
{
	return mNumElements;
}

template<typename T>
int32 TSparseArray<T>::GetMaxIndex() const
{
	return static_cast<int32>(mDatas.size());
}

template<typename T>
int32 TSparseArray<T>::Capacity() const
{
	return static_cast<int32>(mDatas.capacity());
}

template<typename T>
void TSparseArray<T>::Reserve(uint32 capacity)
{
	mDatas.reserve(capacity);
}

template<typename T>
bool TSparseArray<T>::IsEmpty() const
{
	return mNumElements == 0;
}

template<typename T>
bool TSparseArray<T>::IsValidIndex(uint32 index) const
{
	return index < mDatas.size() && mDatas[index].first;
}

template<typename T>
void TSparseArray<T>::Empty(int32 ExpectedNumElements)
{
	// T가 자명한 타입이므로 요소 소멸자 호출은 필요 없다.
	std::vector<std::pair<bool, Slot>>().swap(mDatas); // 메모리 해제
	mDatas.reserve(ExpectedNumElements > 0 ? static_cast<std::size_t>(ExpectedNumElements) : 0);
	mFreeIndex = -1;
	mNumElements = 0;
}

template<typename T>
void TSparseArray<T>::Reset()
{
	mDatas.clear(); // 메모리는 유지
	mFreeIndex = -1;
	mNumElements = 0;
}

template<typename T>
void TSparseArray<T>::RemoveAt(uint32 index)
{
	assert(index < mDatas.size());
	if (mDatas[index].first) // 사용 중인 슬롯일 때만
	{
		// T가 자명한 타입이므로 소멸자 호출 없이 슬롯을 free list에 연결한다.
		mDatas[index].first = false;
		mDatas[index].second.NextFreeIndex = mFreeIndex; // 이전 free list 머리에 연결
		mFreeIndex = static_cast<int32>(index);          // 이 슬롯이 새 머리
		mNumElements--;
	}
}

template<typename T>
TArray<T> TSparseArray<T>::ToTArray() const
{
	TArray<T> result;
	result.reserve(mNumElements);
	for (const auto& [occupied, slot] : mDatas)
	{
		if (occupied)
		{
			result.push_back(slot.Element);
		}
	}
	return result;
}

template<typename T>
void TSparseArray<T>::Iterator::SkipEmpty()
{
	while (Index < Owner->mDatas.size() && !Owner->mDatas[Index].first)
	{
		++Index;
	}
}

template<typename T>
TSparseArray<T>::Iterator::Iterator(TSparseArray* owner, std::size_t index)
	: Owner(owner), Index(index)
{
	SkipEmpty();
}

template<typename T>
T& TSparseArray<T>::Iterator::operator*() const
{
	assert(Index < Owner->mDatas.size());
	return Owner->mDatas[Index].second.Element;
}

template<typename T>
T* TSparseArray<T>::Iterator::operator->() const
{
	assert(Index < Owner->mDatas.size());
	return &Owner->mDatas[Index].second.Element;
}

template<typename T>
typename TSparseArray<T>::Iterator& TSparseArray<T>::Iterator::operator++()
{
	++Index;
	SkipEmpty();
	return *this;
}

template<typename T>
bool TSparseArray<T>::Iterator::operator==(const Iterator& other) const
{
	return Owner == other.Owner && Index == other.Index;
}

template<typename T>
bool TSparseArray<T>::Iterator::operator!=(const Iterator& other) const
{
	return !(*this == other);
}

template<typename T>
void TSparseArray<T>::ConstIterator::SkipEmpty()
{
	while (Index < Owner->mDatas.size() && !Owner->mDatas[Index].first)
	{
		++Index;
	}
}

template<typename T>
TSparseArray<T>::ConstIterator::ConstIterator(const TSparseArray* owner, std::size_t index)
	: Owner(owner), Index(index)
{
	SkipEmpty();
}

template<typename T>
const T& TSparseArray<T>::ConstIterator::operator*() const
{
	assert(Index < Owner->mDatas.size());
	return Owner->mDatas[Index].second.Element;
}

template<typename T>
const T* TSparseArray<T>::ConstIterator::operator->() const
{
	assert(Index < Owner->mDatas.size());
	return &Owner->mDatas[Index].second.Element;
}

template<typename T>
typename TSparseArray<T>::ConstIterator& TSparseArray<T>::ConstIterator::operator++()
{
	++Index;
	SkipEmpty();
	return *this;
}

template<typename T>
bool TSparseArray<T>::ConstIterator::operator==(const ConstIterator& other) const
{
	return Owner == other.Owner && Index == other.Index;
}

template<typename T>
bool TSparseArray<T>::ConstIterator::operator!=(const ConstIterator& other) const
{
	return !(*this == other);
}
