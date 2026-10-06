#pragma once

#include "Runtime/Math/FVector2.h"

// FViewportClient.h와 서로 include하면 순환되므로 전방 선언만 사용한다.
class FViewportClient;
struct FRect;

//뷰포트(화면)이 어디에 놓이는지 정하는 계층
//bFocused : 이 뷰포트가 Focus 되어 있는지
//bHovered : 이 뷰포트가 호버 되어있는지
//TopLeft, LengthUV : 왼쪽 위 뷰포트가 시작되는 UV부터 길이 UV = 뷰포트 위치와 크기를 표현하는 정보
//Client : 뷰포트와 연결된 FViewportClient, 이 뷰포트가 표현하고 있는 World과 Camera 등
class FViewport
{
private:

	bool bFocused = false;
	bool bHovered = false;

	//가리키고 있는 Client (소유하지 않는다)
	FViewportClient* Client = nullptr;
public:
	FViewport() = default;
	virtual ~FViewport();

	// Client가 이 뷰포트를 가리키는 포인터를 갖고 있으므로 복사하면 연결이 어긋난다.
	FViewport(const FViewport&) = delete;
	FViewport& operator=(const FViewport&) = delete;

	FVector2 TopLeftUV = { 0.0f, 0.0f };
	FVector2 LengthUV = { 1.0f, 1.0f };

	[[nodiscard]] bool IsFocused() const { return bFocused; }
	[[nodiscard]] bool IsHovered() const { return bHovered; }
	[[nodiscard]] FViewportClient* GetClient() const { return Client; }

	void UpdateFocusedAndHovered(bool bInFocused, bool bInHovered)
	{
		bFocused = bInFocused;
		bHovered = bInHovered;
	}

	// 이전 Client는 RemoveAssociation, 새 Client는 AddAssociation으로 서로 연결/해제한다.
	void SetViewportClient(FViewportClient* NewClient);

	// 뷰포트가 화면의 어느 영역을 차지하는지 UV로 기록한다.
	void SetRegion(const FRect& Rect, const FVector2& ClientSize);
};
