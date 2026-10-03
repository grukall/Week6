#include "UEditorEngine.h"

#include "Runtime/Core/Log.h"
#include "Runtime/Core/Globals.h"
#include "Runtime/Core/FMemory.h"
#include "Runtime/Input/FInputManager.h"
#include "Runtime/Engine/FEngineLoop.h"
#include "Runtime/Engine/FTimeManager.h"
#include "Runtime/Engine/FScene.h"
#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Resource/FResourceLoader.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Engine/FArchive.h"
#include "Runtime/CoreUObject/FUObjectArray.h"

#include "Runtime/Utility/EngineUtil.h"

#include "Editor/Application/FEditorApplication.h"
#include "Editor/Application/FObjViewerApplication.h"

#include "ThirdParty/Json/json.hpp"

#include "Converter.h"

#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <Windows.h>

void UEditorEngine::Init()
{
	HWND Window = EngineLoop.GetMainWindowHandle();
	if (!Renderer.Initialize(Window))
	{
		throw EngineUtil::CreateError("FRenderer 초기화에 실패했습니다.");
	}
	FStatsManager::Get().Initialize(Renderer.GetDevice());
	FMemory::Init();

	FRenderResourceLibrary& RenderResources = FRenderResourceLibrary::Get();
	if (!RenderResources.Initialize(Renderer))
	{
		throw EngineUtil::CreateError("FRenderResourceLibrary 초기화에 실패했습니다.");
	}

	UClass::ResolveTypeBitsets();

	FResourceLoader::LoadAssets();

#if defined(_OBJVIEWER)
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* Context = nullptr;
	Renderer.GetDeviceAndContext_ImplDX11(Device, Context);

	TUniquePtr<FObjViewerApplication> ObjViewer = MakeUnique<FObjViewerApplication>(Renderer);
	ObjViewer->Initialize(Window, Device, Context);
	Application = std::move(ObjViewer);

#else
	// 새씬 생성
	SetWorld(NewObject<UWorld>());

	TUniquePtr<FEditorApplication> EditorApp = MakeUnique<FEditorApplication>();
	{
		ID3D11Device* Device = nullptr;
		ID3D11DeviceContext* Context = nullptr;
		Renderer.GetDeviceAndContext_ImplDX11(Device, Context);
		EditorApp->Initialize_ImguiWin32DX11(Window, Device, Context);
	}
	EditorApp->Initialize_Runtime(this, &RenderView);
	Application = std::move(EditorApp);
#endif

}

void UEditorEngine::Tick(float DeltaTime)
{
	FStatsManager::Get().ResetFrame();
	FInputManager::Get().BeginFrame();

	if (Globals::bIsRequestingResize)
	{
		Renderer.OnWindowSize(Globals::ResizeWidth, Globals::ResizeHeight);
		Application->OnWindowSize(Globals::ResizeWidth, Globals::ResizeHeight);
		Globals::bIsRequestingResize = false;
	}

	{
		SCOPE_CYCLE_COUNTER("Game");
		Application->Tick(DeltaTime);
	}

	{
		SCOPE_CYCLE_COUNTER("Draw");
		Renderer.BeginFrame();
		Application->Render();
		Renderer.SwapBuffer();
	}

	SET_CYCLE_COUNTER("Frame", FTimeManager::GetDeltaTime() * 1000.0f);

	FInputManager::Get().EndFrame();

	// 입력 메시지 수신 ~ 프레임 종료까지의 지연. 프레임의 맨 마지막이어야 한다.
	FInputLatencyTimer::Get().Tick();
}

void UEditorEngine::Exit()
{

	Application->Shutdown();

//#if !defined(_OBJVIEWER)
//	SceneManager.Release();
//#endif
	Application.Reset();

	Renderer.Shutdown();

}

const FWorldContext& UEditorEngine::GetWorldContextFromWorld(UWorld* InWorld) const
{ 
	if (InWorld == nullptr)
		return (FWorldContext());

	for (const FWorldContext& WorldContext : WorldContexts)
	{
		if (WorldContext.GetCurrentWorld() && WorldContext.GetCurrentWorld() == InWorld)
		{
			return (WorldContext);
		}
	}
	return (FWorldContext());
}


void UEditorEngine::SaveWorld(const FString& path) const
{
	std::filesystem::path fsPath(path);
	std::filesystem::path directory = fsPath.parent_path();

	if (!directory.empty() && !std::filesystem::exists(directory))
		std::filesystem::create_directories(directory);

	FUObjectArray& ObjectArray = FUObjectArray::Get();
	int32 UUID = ObjectArray.GetNextUUID();

	FArchive Archive;
	Archive.SetInt32("Version", 2);
	Archive.SetInt32("NextUUID", UUID);

	FArchive SceneArchive;
	GWorld->Serialize(SceneArchive);
	Archive.SetArchive("Scene", SceneArchive);

	std::ofstream file(path);
	if (!file)
	{
		UE_LOG("[SaveScene] 현재 씬을 파일로 저장하는데 실패했습니다. 파일에 쓸 수 없습니다.");
		return;
	}

	file << Archive.GetJSON().dump(4);
}

void UEditorEngine::LoadWorld(const FString& path, FCamera* OutCamera)
{

	std::ifstream file(path);
	if (!file)
	{
		UE_LOG("[LoadScene] 씬을 파일에서 불러오는데 실패했습니다. 파일을 읽을 수 없습니다.");
		return;
	}

	std::stringstream buffer;
	buffer << file.rdbuf();

	nlohmann::json JSON = nlohmann::json::parse(buffer.str());
	FArchive Archive{ JSON };

	// TODO: TEMP: 경연 대회용 임시 컨버터 로직
	if (Archive.IsNull("Version") || Archive.GetInt32("Version") == 1)
	{
		Archive = Converter::GetStandardArchive(Archive, std::filesystem::path(path), OutCamera);
	}

	int32 Version = Archive.GetInt32("Version");
	if (Version != 2)
	{
		UE_LOG("[LoadScene] 로드하려는 파일의 Scene Schema 버전이 다릅니다. 파일의 버전: %d, 지원하는 버전: %d", Version, 2);
		return;
	}

	int32 NextUUID = Archive.GetInt32("NextUUID");
	FUObjectArray& ObjectArray = FUObjectArray::Get();
	ObjectArray.SetNextUUID(NextUUID);

	if (Archive.IsNull("Scene"))
	{
		UE_LOG("[LoadScene] 로드하려는 파일에서 Scene 항목이 없습니다. 파일 형식이 올바르지 않습니다.");
		return;
	}

	FArchive SceneArchive = Archive.GetArchive("Scene");

	SetWorld(NewObject<UWorld>());
	GWorld->GetPersistentLevel()->Deserialize(SceneArchive);
}

void UEditorEngine::SetWorld(UWorld* InWorld)
{
	if (InWorld == nullptr)
	{
		return;
	}
	if (GWorld)
	{
		DestroyObject(GWorld);
		GWorld = nullptr;
	}

	InWorld->Initialize();
	GWorld = InWorld;
}

void UEditorEngine::Release()
{
	for (const FWorldContext& WorldContext : WorldContexts)
	{
		UWorld* World = WorldContext.GetCurrentWorld();
		if (World)
		{
			if (World == GWorld)
			{
				GWorld = nullptr;
			}
			DestroyObject(World);
		}
	}

	if (GWorld)
	{
		DestroyObject(GWorld);
		GWorld = nullptr;
	}
}
