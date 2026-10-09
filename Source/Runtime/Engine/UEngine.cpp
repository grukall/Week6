#include "UEngine.h"

#include "Runtime/Core/Globals.h"
#include "Runtime/Input/FInputManager.h"
#include "Runtime/Engine/FTimeManager.h"
#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Resource/FResourceLoader.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/Engine/FGameViewportClient.h"
#include "Runtime/Utility/EngineUtil.h"
#include "FArchive.h"
#include "Source/Converter.h"

#include <Windows.h>
#include "FViewportClient.h"
#include "Runtime/Slate/FViewport.h"

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
	while (!WorldContexts.empty())
	{
		DestroyWorld(WorldContexts.back().ContextId);
	}
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

UWorld* UEngine::BuildWorld(EWorldType Type, const FArchive* Source)
{
	UWorld* World = NewObject<UWorld>();
	World->Initialize(Type);

	if (Source)
	{
		// 역직렬화 중 예외가 나면 이 월드는 아직 아무도 소유하지 않으므로 여기서 정리한다.
		try
		{
			World->Deserialize(*Source);
		}
		catch (const std::exception& Error)
		{
			UE_LOG_ERROR("[BuildWorld] 월드 역직렬화에 실패했습니다: %s", Error.what());
			DestroyObject(World);
			return nullptr;
		}
	}

	return World;
}

void UEngine::ActivateWorld(UWorld* World)
{
	if (ULevel* Level = World->GetPersistentLevel())
	{
		Level->Activate();
	}

	// 활성화로 FScene에 컴포넌트가 등록된 뒤에 BVH를 만든다.
	FScene* Scene = World->GetScene();
	Scene->GetSceneBVH().Build(Scene->GetPrimitives());

	// 에디터 월드는 BeginPlay하지 않는다. 플레이하는 월드만 시작한다.
	const EWorldType Type = World->GetWorldType();
	if (Type == EWorldType::PIE || Type == EWorldType::Game)
	{
		World->BeginPlay();
	}
}

void UEngine::ShutdownWorld(UWorld* World)
{
	if (!World)
	{
		return;
	}

	World->EndPlay();
	DestroyObject(World);
}

//새 컨텍스트와 월드를 만든다.Source가 있으면 그 내용으로 채운다(PIE 복제 등).
//CurrentWorld가 비어 있으면 이 월드가 CurrentWorld가 된다. (이미 있으면 바꾸지 않는다)
uint32 UEngine::CreateWorld(EWorldType Type, const FArchive* Source)
{
	UWorld* World = BuildWorld(Type, Source);
	if (!World)
	{
		return InvalidContextId;
	}

	const uint32 ContextId = NextContextId++;
	WorldContexts.push_back(FWorldContext{ Type, World, ContextId });

	// 첫 월드(에디터 월드)만 자동으로 CurrentWorld가 된다. PIE 월드가 생겨도 CurrentWorld는 바뀌지 않는다.
	if (!CurrentWorld)
	{
		CurrentWorld = World;
	}

	// 컨텍스트에 등록된 뒤에 시작한다. BeginPlay 중에 컨텍스트를 조회해도 찾을 수 있도록.
	ActivateWorld(World);
	return ContextId;
}

//같은 컨텍스트의 월드를 새 월드로 바꾼다 (LoadMap, NewMap). 새 월드를 완성한 뒤에 이전 월드를 파괴하므로
//실패하면 false를 반환하고 이전 월드는 그대로 남김
bool UEngine::ReplaceWorld(uint32 ContextId, const FArchive* Source)
{
	FWorldContext* Context = FindWorldContext(ContextId);
	if (!Context)
	{
		UE_LOG_ERROR("[ReplaceWorld] 등록되지 않은 컨텍스트입니다. ContextId=%u", ContextId);
		return false;
	}

	// 새 월드를 먼저 완성한다. 실패하면 이전 월드는 그대로 둔다.
	UWorld* NewWorld = BuildWorld(Context->WorldType, Source);
	if (!NewWorld)
	{
		return false;
	}

	UWorld* OldWorld = Context->World;
	Context->World = NewWorld;

	// 교체된 월드가 현재 월드였다면 따라가야 한다 (파괴된 월드를 가리키지 않도록).
	if (CurrentWorld == OldWorld)
	{
		CurrentWorld = NewWorld;
	}

	ShutdownWorld(OldWorld);
	ActivateWorld(NewWorld);
	return true;
}

//월드를 종료(EndPlay)하고 파괴하고 컨텍스트를 제거한다.
//월드를 가리키는 엔진 밖의 참조(뷰포트의 ContextId, 선택 등)는 호출하는 쪽이 먼저 정리해야 한다.
void UEngine::DestroyWorld(uint32 ContextId)
{
	for (size_t i = 0; i < WorldContexts.size(); ++i)
	{
		if (WorldContexts[i].ContextId != ContextId)
		{
			continue;
		}

		UWorld* World = WorldContexts[i].World;

		// CurrentWorld가 이 월드였다면 비운다. (엔진 종료 때가 아니면 보통 CurrentWorld는 에디터 월드다)
		if (CurrentWorld == World)
		{
			CurrentWorld = nullptr;
		}

		FGameViewportClient* ViewportClient = WorldContexts[i].GameViewportClient;
		if (ViewportClient)
		{
			// 호출하는 쪽이 뷰포트에서 먼저 떼어 두는 것이 원칙이다. (SIE 상태면 연결돼 있지 않아 null이다)
			// 아직 연결돼 있다면 뷰포트가 해제된 Client를 가리키지 않도록 안전장치로 양쪽을 끊는다.
			if (FViewport* Viewport = ViewportClient->GetViewport())
			{
				Viewport->SetViewportClient(nullptr);
			}
			delete(ViewportClient);
		}

		WorldContexts.erase(WorldContexts.begin() + i);
		ShutdownWorld(World);
		return;
	}
}

FWorldContext* UEngine::FindWorldContext(uint32 ContextId)
{
	for (FWorldContext& Context : WorldContexts)
	{
		if (Context.ContextId == ContextId)
		{
			return &Context;
		}
	}

	return nullptr;
}

UWorld* UEngine::GetWorld(uint32 ContextId)
{
	FWorldContext* Context = FindWorldContext(ContextId);
	return Context ? Context->World : nullptr;
}

bool UEngine::LoadMap(UWorld* World, const FString& Path)
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

	return LoadMap(*FoundContext, Path);
}

bool UEngine::LoadMap(FWorldContext& WorldContext, const FString& Path)
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
	//if (Archive.IsNull("Version") || Archive.GetInt32("Version") == 1)
	//{
	//	Archive = Converter::GetStandardArchive(Archive, std::filesystem::path(Path), OutLegacyCamera);
	//}

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
	return ReplaceWorld(WorldContext.ContextId, &WorldArchive);
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
	// 월드 타입은 컨텍스트의 것을 따른다. 빈 월드로 교체한다.
	ReplaceWorld(WorldContext.ContextId, nullptr);
}