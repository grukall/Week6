#pragma once


#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Engine/FRenderView.h"
#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/Core/TArray.h"
#include "UWorld.h"
#include <Windows.h>
#include <Runtime/Core/FString.h>

class FEngineLoop;
class FCamera;

// 엔진의 런타임 계층을 담당하는 클래스
// 엔진 로직, 시스템을 처리하는 부분은 여기서 담당

class UEngine : public UObject
{
	GENERATED_BODY()
	DECLARE_UCLASS(UEngine, UObject)

protected:
	UEngine(){}
	virtual ~UEngine() = default;

public:
	TArray<FWorldContext> WorldContexts;

	//현재 Engine이 focus하고 있는 World(ex: PIE 실행 후 화면 클릭 시, CurrentWorld는 PIE World)
	UWorld* CurrentWorld = nullptr;
	void Update(float DeltaTime);

	virtual void Init(HWND Window);
	virtual void Tick(float DeltaTime) = 0;
	virtual void Exit();

	virtual void OnWindowResize(UINT Width, UINT Height);
	virtual void Render();

	//인덱스 반환
	int32 CreateWorldContext(EWorldType WorldType, UWorld* World);
	void SetWorld(FWorldContext& WorldContext, UWorld *New);

	bool LoadMap(UWorld* World, const FString& Path, FCamera* OutLegacyCamera = nullptr);
	bool LoadMap(FWorldContext& WorldContext, const FString& Path, FCamera* OutLegacyCamera = nullptr);
	bool SaveMap(const UWorld& World, const FString& Path) const;

	void NewMap(UWorld* World, EWorldType WorldType);
	void NewMap(FWorldContext& WorldContext, EWorldType WorldType);

protected:
	FRenderer Renderer;
	FRenderView RenderView{ Renderer };
};