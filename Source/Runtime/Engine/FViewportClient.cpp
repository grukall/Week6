#include "pch.h"
#include "FViewportClient.h"
#include "Runtime/Engine/UEngine.h"

UWorld* FViewportClient::GetWorld() const
{
     return Engine ? Engine->GetWorld(ContextId) : nullptr;
}

void FViewportClient::AddAssociation(FViewport& _Viewport)
{
    Viewport = &_Viewport;
}

void FViewportClient::RemoveAssociation(FViewport & _Viewport)
{
    Viewport = nullptr;
}
