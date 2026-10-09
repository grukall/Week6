#include "pch.h"
#include "FViewport.h"
#include "Runtime/Engine/FViewportClient.h"
#include "Runtime/UI/SWindow.h"

FViewport::~FViewport()
{
	SetViewportClient(nullptr);
}

void FViewport::SetViewportClient(FViewportClient* NewClient)
{
	if (Client == NewClient)
	{
		return;
	}

	if (Client)
	{
		Client->RemoveAssociation(*this);
	}

	Client = NewClient;

	if (Client)
	{
		Client->AddAssociation(*this);
	}
}

//뷰포트가 화면의 어느 영역을 차지하고 있는지 UV로 기록
void FViewport::SetRegion(const FRect& Rect, const FVector2& ClientSize)
{
	TopLeftUV = FVector2{ Rect.Left / ClientSize.X, Rect.Top / ClientSize.Y };
	LengthUV = FVector2{ Rect.GetWidth() / ClientSize.X, Rect.GetHeight() / ClientSize.Y };
}
