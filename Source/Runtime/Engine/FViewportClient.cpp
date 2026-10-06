#include "pch.h"
#include "FViewportClient.h"
#include "Runtime/Engine/UEngine.h"

UWorld* FViewportClient::GetWorld() const
{
     return Engine ? Engine->GetWorld(ContextId) : nullptr;
}
