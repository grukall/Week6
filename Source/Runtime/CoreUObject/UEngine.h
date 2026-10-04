#pragma once

#include "UObject.h"
#include "Runtime/CoreUObject/UWorld.h"
#include "Runtime/Engine/FEngineLoop.h"



struct FWorldContext
{
	EWorldType WorldType = None;

	UWorld* World() const
	{
		return ThisCurrentWorld;
	}

	void    SetCurrentWorld(UWorld* World)
	{
		ThisCurrentWorld = World;
	}

	UWorld* GetCurrentWorld() const
	{
		return (ThisCurrentWorld);
	}

	void    SetCurrentWorldType(EWorldType InWorldType)
	{
		WorldType = InWorldType;
	}

private:
	UWorld* ThisCurrentWorld = nullptr;
};

class UEngine : public UObject
{
	GENERATED_BODY()
	DECLARE_UCLASS_NO_COPY(UEngine, UObject)

public:
	virtual void Init(FEngineLoop& InEngineLoop) {}
	virtual void Tick(float DeltaTime) {}
	virtual void Exit() {}

	const FWorldContext* GetWorldContextFromWorld(UWorld* InWorld) const;
	void AddWorld(UWorld* InWorld, EWorldType InWorldType);

protected:
	UEngine() = default;
	TArray<FWorldContext> WorldContexts;
};