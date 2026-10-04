#include "UEngine.h"


IMPLEMENT_UCLASS_NO_COPY(UEngine, UObject)


const FWorldContext* UEngine::GetWorldContextFromWorld(UWorld* InWorld) const
{
	if (InWorld == nullptr)
		return (nullptr);

	for (const FWorldContext& WorldContext : WorldContexts)
	{
		if (WorldContext.GetCurrentWorld() && WorldContext.GetCurrentWorld() == InWorld)
		{
			return (&WorldContext);
		}
	}
	return (nullptr);
}

void UEngine::AddWorld(UWorld* InWorld, EWorldType InWorldType)
{
	FWorldContext WorldContext;
	WorldContext.SetCurrentWorld(InWorld);
	WorldContext.SetCurrentWorldType(InWorldType);
	WorldContexts.push_back(WorldContext);
}

