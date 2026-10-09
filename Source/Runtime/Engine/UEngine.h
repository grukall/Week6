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
class FArchive;

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

	uint32 CreateWorld(EWorldType Type, const FArchive* Source = nullptr);
	bool ReplaceWorld(uint32 ContextId, const FArchive* Source = nullptr);
	void DestroyWorld(uint32 ContextId);

	FWorldContext* FindWorldContext(uint32 ContextId);
	UWorld* GetWorld(uint32 ContextId);

	bool LoadMap(UWorld* World, const FString& Path);
	bool LoadMap(FWorldContext& WorldContext, const FString& Path);
	bool SaveMap(const UWorld& World, const FString& Path) const;

	void NewMap(UWorld* World, EWorldType WorldType);
	void NewMap(FWorldContext& WorldContext, EWorldType WorldType);

protected:
	FRenderer Renderer;
	FRenderView RenderView{ Renderer };

private:
	// 월드 하나를 만드는 단계를 한 가지씩 나눈 내부 함수
	// BuildWorld   : NewObject -> Initialize -> (Source가 있으면) Deserialize. 가동 전 상태. 실패하면 nullptr
	// ActivateWorld: 레벨 Activate(FScene 등록) -> BVH 빌드 -> 월드 타입이 PIE/Game이면 BeginPlay
	// ShutdownWorld: EndPlay -> DestroyObject
	UWorld* BuildWorld(EWorldType Type, const FArchive* Source);
	void ActivateWorld(UWorld* World);
	void ShutdownWorld(UWorld* World);

	uint32 NextContextId = 0;
};