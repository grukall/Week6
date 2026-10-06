#pragma once

#include "Runtime/Core/FString.h"
#include "Runtime/Core/IntTypes.h"

#include <cstdio>
#include <functional>
#include <random>

// 128비트 전역 고유 식별자 (UE의 FGuid에 대응)
// A + B + C + D, 4개 32bit 무작위 값으로 고유한 FGuid를 만듬
struct FGuid
{
	uint32 A = 0;
	uint32 B = 0;
	uint32 C = 0;
	uint32 D = 0;

	FGuid() = default;
	FGuid(uint32 InA, uint32 InB, uint32 InC, uint32 InD) : A(InA), B(InB), C(InC), D(InD) {}

	[[nodiscard]] bool IsValid() const { return (A | B | C | D) != 0; }
	void Invalidate() { A = B = C = D = 0; }

	[[nodiscard]] bool operator==(const FGuid& Other) const
	{
		return A == Other.A && B == Other.B && C == Other.C && D == Other.D;
	}
	[[nodiscard]] bool operator!=(const FGuid& Other) const { return !(*this == Other); }

	// 무작위 128비트 값으로 새 Guid를 만든다. (항상 유효한 값)
	[[nodiscard]] static FGuid NewGuid()
	{
		thread_local std::mt19937_64 Engine = []
		{
			std::random_device Device;
			const uint64 Seed = (static_cast<uint64>(Device()) << 32) | Device();
			return std::mt19937_64(Seed);
		}();

		FGuid Result;
		do
		{
			//uint64 랜덤 분리 => uint32 랜덤 2개
			const uint64 High = Engine();
			const uint64 Low = Engine();
			Result = FGuid(
				static_cast<uint32>(High >> 32), static_cast<uint32>(High),
				static_cast<uint32>(Low >> 32), static_cast<uint32>(Low));
		} while (!Result.IsValid());

		return Result;
	}

	// 32자리 16진수 문자열 (예: "0A1B2C3D4E5F60718293A4B5C6D7E8F9")
	[[nodiscard]] FString ToString() const
	{
		char Buffer[33];
		std::snprintf(Buffer, sizeof(Buffer), "%08X%08X%08X%08X", A, B, C, D);
		return FString(Buffer);
	}

	// ToString()이 만든 형식을 읽는다. 형식이 맞지 않으면 false를 반환하고 Out은 변경하지 않는다.
	[[nodiscard]] static bool Parse(const FString& Text, FGuid& Out)
	{
		if (Text.size() != 32)
		{
			return false;
		}

		uint32 Parts[4] = {};
		for (int32 PartIndex = 0; PartIndex < 4; ++PartIndex)
		{
			uint32 Value = 0;
			for (int32 CharIndex = 0; CharIndex < 8; ++CharIndex)
			{
				const char Ch = Text[PartIndex * 8 + CharIndex];
				uint32 Digit;
				if (Ch >= '0' && Ch <= '9') { Digit = static_cast<uint32>(Ch - '0'); }
				else if (Ch >= 'A' && Ch <= 'F') { Digit = static_cast<uint32>(Ch - 'A' + 10); }
				else if (Ch >= 'a' && Ch <= 'f') { Digit = static_cast<uint32>(Ch - 'a' + 10); }
				else { return false; }

				Value = (Value << 4) | Digit;
			}
			Parts[PartIndex] = Value;
		}

		Out = FGuid(Parts[0], Parts[1], Parts[2], Parts[3]);
		return true;
	}

	[[nodiscard]] size_t GetHash() const
	{
		size_t Hash = std::hash<uint32>{}(A);
		Hash ^= std::hash<uint32>{}(B) + 0x9e3779b9 + (Hash << 6) + (Hash >> 2);
		Hash ^= std::hash<uint32>{}(C) + 0x9e3779b9 + (Hash << 6) + (Hash >> 2);
		Hash ^= std::hash<uint32>{}(D) + 0x9e3779b9 + (Hash << 6) + (Hash >> 2);
		return Hash;
	}
};

namespace std
{
	template <>
	struct hash<FGuid>
	{
		size_t operator()(const FGuid& Guid) const noexcept { return Guid.GetHash(); }
	};
}
