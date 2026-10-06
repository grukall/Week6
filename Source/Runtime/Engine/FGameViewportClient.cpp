#include "FGameViewportClient.h"

void FGameViewportClient::AddAssociation(FViewport& Viewport)
{
	FViewportClient::AddAssociation(Viewport);
}

void FGameViewportClient::RemoveAssociation(FViewport & Viewport)
{
	FViewportClient::RemoveAssociation(Viewport);
}

bool FGameViewportClient::GetViewInfo(FCamera& OutCamera)
{
	OutCamera = TempCamera;
	return true;
}
