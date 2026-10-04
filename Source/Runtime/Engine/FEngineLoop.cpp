#include "FEngineLoop.h"

#include "Runtime/Core/Globals.h"
#include "Runtime/Engine/FTimeManager.h"
#include "Runtime/Engine/UEngine.h"

#include <format>
#include <Windows.h>
#include <Runtime/Core/FMemory.h>

void FEngineLoop::Init(HINSTANCE Instance)
{
	// 윈도우 객체 초기화
	WindowsApplication = MakeUnique<FWindowsApplication>(Instance);
	WindowsApplication->CreateMainWindow({
		.Instance = Instance,
	    .ClassName = Globals::EngineWindowClass,
	    .WindowName = Globals::EngineName,
		.Width = Globals::WindowWidth,
		.Height = Globals::WindowHeight,
	});

	//Memory Allocator 초기화
	FMemory::Init();
}

void FEngineLoop::Tick()
{
	while (!Globals::bIsRequestingExit)
	{
		// 윈도우 종료 메시지 체크
		if (WindowsApplication->CheckExitMessage())
		{
			Globals::bIsRequestingExit = true;
			continue;
		}

		// 시간 업데이트
		FTimeManager::Tick();
		Engine->Update(FTimeManager::GetDeltaTime());

	}
}

void FEngineLoop::Exit()
{
	// 엔진 종료
	Engine->Exit();
	DestroyObject(Engine);

	// 윈도우 객체 종료
	WindowsApplication->Quit();
}

HWND FEngineLoop::GetMainWindowHandle() const
{
	if (!WindowsApplication)
	{
		return nullptr;
	}

	const FWindow* MainWindow = WindowsApplication->GetMainWindow();

	if (!MainWindow)
	{
		return nullptr;
	}

	return MainWindow->GetHandle();
}
