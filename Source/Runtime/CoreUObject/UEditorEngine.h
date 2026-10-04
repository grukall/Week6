#pragma once

#include "UObject.h"
#include "Runtime/Core/PointerTypes.h"
#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Engine/FRenderView.h"
#include "Editor/Application/IApplication.h"
#include "Runtime/CoreUObject/UWorld.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Engine/FScene.h"

class FEngineLoop;
class FCamera;

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

// 엔진의 런타임 계층을 담당하는 클래스
// 엔진 로직, 시스템을 처리하는 부분은 여기서 담당
class UEditorEngine// : public UObject 할거 다하고 이거 수정
{
	//GENERATED_BODY()
	//DECLARE_UCLASS(UEditorEngine, UObject)

private:
	FRenderer Renderer;
	FRenderView RenderView{ Renderer };
	FEngineLoop& EngineLoop;
	TUniquePtr<IApplication> Application;
	TArray<FWorldContext> WorldContexts;

public:
	UEditorEngine(FEngineLoop& InEngineLoop)
		: EngineLoop{ InEngineLoop }
	{
	}

	void Init();
	void Tick(float DeltaTime);
	void Exit();
	const FWorldContext* GetWorldContextFromWorld(UWorld* InWorld) const;
	FWorldContext& GetEditorWorldContext(bool bEnsureIsGWorld = false);

	void SaveWorld(const FString& path) const;
	void LoadWorld(const FString& path, FCamera* OutCamera = nullptr);
	void SetWorld(UWorld* InWorld, EWorldType InWorldType);
	void AddWorld(UWorld* InWorld, EWorldType InWorldType);
	void Release();

	void StartPIE();
	void EndPIE();
};
