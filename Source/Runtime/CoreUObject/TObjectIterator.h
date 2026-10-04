#pragma once

#include "Runtime/CoreUObject/FUObjectArray.h"

template<typename TObject>
	requires std::derived_from<TObject, UObject>
class TObjectIterator
{
public:
	TObjectIterator() : Iterator(FUObjectArray::Get().begin())
	{
		SkipInvalid();
	}

	TObjectIterator& operator++()
	{
		if (Iterator != FUObjectArray::Get().end()) 
		{
			++Iterator;
			SkipInvalid();
		}
		return *this;
	}

	TObjectIterator operator++(int)
	{
		TObjectIterator Temp = *this;
		++(*this);
		return Temp;
	}

	explicit operator bool() const 
	{
		return Iterator != FUObjectArray::Get().end();
	}

	TObject* operator*() const
	{
		if (Iterator == FUObjectArray::Get().end())
		{
			return nullptr;
		}

		return static_cast<TObject*>(*Iterator);
	}

	TObject* operator->() const 
	{
		return operator*();
	}

private:
	void SkipInvalid() 
	{
		while (Iterator != FUObjectArray::Get().end()) 
		{
			// 빈 슬롯은 이터레이터가 건너뛰므로 Object는 항상 유효하다.
			UObject* Object = *Iterator;
			if (Object->IsA(TObject::StaticClass()))
			{
				return;
			}
			++Iterator;
		}
	}

	FUObjectArray::TIterator Iterator;
};

