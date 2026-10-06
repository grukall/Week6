#pragma once


//하나의 T만 저장하는 자료구조, 새로운 자료구조가 오면 덮어씌운다.
template<typename T>
struct TOptional
{
	T Saved;
	bool bIsSet = false;

	bool IsSet() const { return bIsSet; }
	void Set(const T& _New)
	{
		Saved = _New;
		bIsSet = true;
	}

	T *Get()
	{
		if (!bIsSet)
			return nullptr;

		return &Saved;
	}

	void Reset() { bIsSet = false; }
};