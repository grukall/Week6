

#include "UEditorEngine.h"
#include "Editor/Application/FEditorApplication.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Engine/USceneManager.h"
#include "Runtime/Engine/UScene.h"
#include "Runtime/Rendering/FRenderer.h"
#include <Windows.h>
#include <Runtime/Core/PointerTypes.h>

IMPLEMENT_UCLASS(UEditorEngine, UEngine)

void UEditorEngine::Init(HWND Window)
{
	Super::Init(Window);

#if defined(_OBJVIEWER)
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* Context = nullptr;
	Renderer.GetDeviceAndContext_ImplDX11(Device, Context);

	TUniquePtr<FObjViewerApplication> ObjViewer = MakeUnique<FObjViewerApplication>(Renderer);
	ObjViewer->Initialize(Window, Device, Context);
	Application = std::move(ObjViewer);

#else
	// 새씬 생성
	SceneManager.SetScene(NewObject<UScene>());
	TUniquePtr<FEditorApplication> EditorApp = MakeUnique<FEditorApplication>();
	{
		ID3D11Device* Device = nullptr;
		ID3D11DeviceContext* Context = nullptr;
		Renderer.GetDeviceAndContext_ImplDX11(Device, Context);
		EditorApp->Initialize_ImguiWin32DX11(Window, Device, Context);
	}
	EditorApp->Initialize_Runtime(&SceneManager, &RenderView);
	Application = std::move(EditorApp);
#endif

}

void UEditorEngine::Exit()
{
	Application->Shutdown();
	Application.Reset();

	Super::Exit();
}

void UEditorEngine::OnWindowResize(UINT Width, UINT Height)
{
	Super::OnWindowResize(Width, Height);
	Application->OnWindowSize(Width, Height);
}

void UEditorEngine::Update(float DeltaTime)
{
	Super::Update(DeltaTime);
	Application->Update(DeltaTime);
}

void UEditorEngine::Render()
{
	Super::Render();
	Application->Render();
}
