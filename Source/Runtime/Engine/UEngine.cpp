#include "UEngine.h"

#include "Runtime/Core/Globals.h"
#include "Runtime/Input/FInputManager.h"
#include "Runtime/Engine/FTimeManager.h"
#include "Runtime/Engine/USceneManager.h"
#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Resource/FResourceLoader.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/Utility/EngineUtil.h"

#include <Windows.h>

IMPLEMENT_UCLASS(UEngine, UObject)

void UEngine::Init(HWND Window)
{
	if (!Renderer.Initialize(Window))
	{
		throw EngineUtil::CreateError("FRenderer 초기화에 실패했습니다.");
	}
	FStatsManager::Get().Initialize(Renderer.GetDevice());
	FRenderResourceLibrary& RenderResources = FRenderResourceLibrary::Get();
	if (!RenderResources.Initialize(Renderer))
	{
		throw EngineUtil::CreateError("FRenderResourceLibrary 초기화에 실패했습니다.");
	}

	UClass::ResolveTypeBitsets();

	FResourceLoader::LoadAssets();
}
void UEngine::Tick(float DeltaTime)
{
	FStatsManager::Get().ResetFrame();
	FInputManager::Get().BeginFrame();

	if (Globals::bIsRequestingResize)
	{
		OnWindowResize(Globals::ResizeWidth, Globals::ResizeHeight);
		Globals::bIsRequestingResize = false;
	}

	{
		SCOPE_CYCLE_COUNTER("Game");
		Update(DeltaTime);
	}

	{
		SCOPE_CYCLE_COUNTER("Draw");
		Renderer.BeginFrame();
		Render();
		Renderer.SwapBuffer();
	}

	SET_CYCLE_COUNTER("Frame", FTimeManager::GetDeltaTime() * 1000.0f);

	FInputManager::Get().EndFrame();

	// 입력 메시지 수신 ~ 프레임 종료까지의 지연. 프레임의 맨 마지막이어야 한다.
	FInputLatencyTimer::Get().Tick();
}
void UEngine::Exit()
{
#if !defined(_OBJVIEWER)
	SceneManager.Release();
#endif

	Renderer.Shutdown();
}

void UEngine::OnWindowResize(UINT Width, UINT Height)
{
	Renderer.OnWindowSize(Width, Height);
}

void UEngine::Update(float DeltaTime)
{
}

void UEngine::Render()
{
}
