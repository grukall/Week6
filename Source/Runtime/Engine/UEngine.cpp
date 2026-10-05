#include "UEngine.h"

#include "Runtime/Core/Globals.h"
#include "Runtime/Input/FInputManager.h"
#include "Runtime/Engine/FTimeManager.h"
#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Resource/FResourceLoader.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/Utility/EngineUtil.h"
#include "FArchive.h"
#include "Source/Converter.h"

#include <Windows.h>

IMPLEMENT_ABSTRACT_UCLASS(UEngine, UObject)

void UEngine::Update(float DeltaTime)
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
		Tick(DeltaTime);
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

void UEngine::Exit()
{
#if !defined(_OBJVIEWER)
	for (int i = WorldContexts.size() - 1; i >= 0; --i)
	{
		DestroyObject(WorldContexts[i].World);
	}
	WorldContexts.clear();
#endif

	Renderer.Shutdown();
}

void UEngine::OnWindowResize(UINT Width, UINT Height)
{
	Renderer.OnWindowSize(Width, Height);
}

void UEngine::Render()
{
}

int32 UEngine::CreateWorldContext(EWorldType WorldType, UWorld* World)
{
	WorldContexts.push_back(FWorldContext{ WorldType, World });
	return WorldContexts.size() - 1;
}

void UEngine::SetWorld(FWorldContext& WorldContext, UWorld* New)
{
	UWorld* Old = WorldContext.World;
	if (Old)
	{
		Old->EndPlay();
		DestroyObject(Old);
	}

	New->Initialize(WorldContext.WorldType);
	WorldContext.World = New;

	// 교체된 월드가 현재 월드였다면 따라가야 한다 (파괴된 월드를 가리키지 않도록).
	if (CurrentWorld == Old)
	{
		CurrentWorld = New;
	}

	if (ULevel* Level = New->GetPersistentLevel())
	{ 
		Level->Activate();
	}

	//새로운 월드로 설정되면 BVH를 재구축한다.
	FScene* Scene = New->GetScene();
	Scene->GetSceneBVH().Build(Scene->GetPrimitives());
}

UWorld* UEngine::GetWorld(uint32 WorldContextId)
{
	for (FWorldContext &Context : WorldContexts)
	{
		if (Context.ContextId == WorldContextId)
		{
			return Context.World;
		}
	}

	return nullptr;
}

bool UEngine::LoadMap(UWorld* World, const FString& Path, FCamera* OutLegacyCamera)
{
	if (!World)
	{
		UE_LOG_ERROR("Fail LoadMap becuase World is nullptr, check LoadMap Timing");
		return false;
	}

	FWorldContext* FoundContext = nullptr;
	for (FWorldContext& WorldContext : WorldContexts)
	{
		if (WorldContext.World == World)
		{
			FoundContext = &WorldContext;
		}
	}

	if (!FoundContext)
	{
		UE_LOG_ERROR("FailLoadMap becuase Unregisterd WorldContext");
		return false;
	}

	return LoadMap(*FoundContext, Path, OutLegacyCamera);
}

bool UEngine::LoadMap(FWorldContext& WorldContext, const FString& Path, FCamera* OutLegacyCamera)
{
	std::ifstream file(Path);
	if (!file)
	{
		UE_LOG("[LoadScene] 씬을 파일에서 불러오는데 실패했습니다. 파일을 읽을 수 없습니다.");
		return false;
	}

	std::stringstream buffer;
	buffer << file.rdbuf();

	nlohmann::json JSON = nlohmann::json::parse(buffer.str());
	FArchive Archive{ JSON };

	// TODO: TEMP: 경연 대회용 임시 컨버터 로직
	if (Archive.IsNull("Version") || Archive.GetInt32("Version") == 1)
	{
		Archive = Converter::GetStandardArchive(Archive, std::filesystem::path(Path), OutLegacyCamera);
	}

	int32 Version = Archive.GetInt32("Version");
	if (Version != 3)
	{
		UE_LOG("[LoadScene] 로드하려는 파일의 Scene Schema 버전이 다릅니다. 파일의 버전: %d, 지원하는 버전: %d", Version, 3);
		return false;
	}

	if (Archive.IsNull("map"))
	{
		UE_LOG("[LoadScene] 로드하려는 파일에서 Scene 항목이 없습니다. 파일 형식이 올바르지 않습니다.");
		return false;
	}

	FArchive WorldArchive = Archive.GetArchive("map");

	UWorld* World = NewObject<UWorld>();
	World->Initialize(WorldContext.WorldType);

	World->Deserialize(WorldArchive);
	SetWorld(WorldContext, World);

	return true;
}

bool UEngine::SaveMap(const UWorld& World, const FString& Path) const
{
	std::filesystem::path fsPath(Path);
	std::filesystem::path directory = fsPath.parent_path();

	if (!directory.empty() && !std::filesystem::exists(directory))
		std::filesystem::create_directories(directory);

	FArchive Archive;
	Archive.SetInt32("Version", 3);

	FArchive WorldArchive;

	//TODO : Scene이 아닌 World의 Serialize로 변경 필요
	World.Serialize(WorldArchive);
	Archive.SetArchive("map", WorldArchive);

	std::ofstream file(Path);
	if (!file)
	{
		UE_LOG("[SaveScene] 현재 씬을 파일로 저장하는데 실패했습니다. 파일에 쓸 수 없습니다.");
		return false;
	}

	file << Archive.GetJSON().dump(4);
	return true;
}

void UEngine::NewMap(UWorld* World, EWorldType WorldType)
{
	if (!World)
	{
		UE_LOG_ERROR("Fail NewMap becuase World is nullptr, check New Timing");
		return;
	}

	FWorldContext* FoundContext = nullptr;
	for (FWorldContext& WorldContext : WorldContexts)
	{
		if (WorldContext.World == World)
		{
			FoundContext = &WorldContext;
		}
	}

	if (!FoundContext)
	{
		UE_LOG_ERROR("Fail NewMap becuase Unregisterd WorldContext");
		return;
	}

	NewMap(*FoundContext, WorldType);
}

void UEngine::NewMap(FWorldContext& WorldContext, EWorldType WorldType)
{
	UWorld* World = NewObject<UWorld>();
	SetWorld(WorldContext, World);
}