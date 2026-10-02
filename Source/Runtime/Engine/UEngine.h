#pragma once


#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Engine/FRenderView.h"
#include "Runtime/Engine/USceneManager.h"
#include "Runtime/CoreUObject/UObject.h"
#include <Windows.h>

class FEngineLoop;

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
	virtual void Init(HWND Window);
	void Tick(float DeltaTime);
	virtual void Exit();

	virtual void OnWindowResize(UINT Width, UINT Height);
	virtual void Update(float DeltaTime);
	virtual void Render();


protected:
	FRenderer Renderer;
	FRenderView RenderView{ Renderer };
	USceneManager SceneManager;
};