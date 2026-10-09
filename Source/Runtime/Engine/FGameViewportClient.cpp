#include "FGameViewportClient.h"

void FGameViewportClient::AddAssociation(FViewport& Viewport)
{
	FViewportClient::AddAssociation(Viewport);
}

void FGameViewportClient::RemoveAssociation(FViewport & Viewport)
{
	FViewportClient::RemoveAssociation(Viewport);
}

void FGameViewportClient::ProccessInput(const FViewportInput& Input, float deltaTime)
{
	//TODO : 게임 입력을 여기서 각 LocalPlayer의 PlayerController에게 전달
}
