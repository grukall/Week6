#include "pch.h"
#include "FViewportClient.h"
#include "Runtime/Engine/UEngine.h"

UWorld* FViewportClient::GetWorld()
{
     return Engine->GetWorld(ContextId);
}
