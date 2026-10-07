#pragma once
#include "Runtime/Math/FVector2.h"

// 이 프레임의 뷰포트 입력 상태.
struct FViewportInput
{
	// 뷰포트 좌상단 기준 마우스 좌표(픽셀)
	FVector2 LocalMouse{};
	// 뷰포트 크기(픽셀)
	FVector2 SizePixels{};

	bool bHovered = false;
	bool bFocused = false;
	bool bPickRequested = false;
	bool bLeftDown = false;
	bool bLeftReleased = false;
};