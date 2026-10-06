

#include "UEditorEngine.h"
#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Engine/FTimeManager.h"
#include <Windows.h>
#include <Runtime/Core/PointerTypes.h>
#include "Runtime/Core/Globals.h"
#include "Runtime/Slate/FViewport.h"
#include "Runtime/Engine/FGameViewportClient.h"

IMPLEMENT_UCLASS(UEditorEngine, UEngine)

void UEditorEngine::Init(HWND Window)
{
	Super::Init(Window);

#if defined(_OBJVIEWER)
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* Context = nullptr;
	Renderer.GetDeviceAndContext_ImplDX11(Device, Context);

	ObjViewer = MakeUnique<FObjViewerApplication>(Renderer);
	ObjViewer->Initialize(Window, Device, Context);
#else
	// 새 빈 에디터 월드 생성 (첫 월드이므로 CurrentWorld가 된다)
    EditorContextId = CreateWorld(EWorldType::Editor);

	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* Context = nullptr;
	Renderer.GetDeviceAndContext_ImplDX11(Device, Context);
	ImguiManager.Initialize_ImplWin32DX11(Window, Device, Context);

	Editor.Initialize(this);

    //뷰포트를 4개 확정으로 생성하네, 이래도 되는건가
    Editor.InitMultiViewport(this, *FindWorldContext(EditorContextId));
	Editor.LoadState();
	Editor.SetViewLayout(Editor.State.GetSplitMode());

	// TEMP: 당분간 기본값으로 활성화
	EditorViewportWindow.Toggle(FImguiStatsWindow::EStatsWindow::Unit);
	EditorViewportWindow.Toggle(FImguiStatsWindow::EStatsWindow::FPS);
#endif

}

void UEditorEngine::Exit()
{
#ifdef _OBJVIEWER
	ObjViewer->Shutdown();
	ObjViewer.Reset();
#else
	Editor.Shutdown();
#endif
	Super::Exit();
}

void UEditorEngine::OnWindowResize(UINT Width, UINT Height)
{
	Super::OnWindowResize(Width, Height);
#ifdef _OBJVIEWER
    ObjViewer->OnWindowSize(Width, Height);
#else
	// 뷰포트 종횡비 갱신
	TArray<TUniquePtr<FViewport>>& Viewports = Editor.GetViewports();
	for (int32 i = 0; i < static_cast<int32>(Viewports.size()); ++i) {
		FEditorViewportClient* EditorClient = Editor.GetEditorClient(i);
		if (!EditorClient) continue;

		const FVector2 SizePixels =
			Viewports[i]->LengthUV *
			FVector2{ static_cast<float>(Width), static_cast<float>(Height) };

		EditorClient->GetCamera().SetAspectRatio(SizePixels.X / SizePixels.Y);
	}
#endif
}

void UEditorEngine::Tick(float DeltaTime)
{
    if (bRequestEndPlay)
    {
        StartQueuedEndSessionRequest();
    }
    if (PlaySessionRequest.IsSet())
    {
        StartQueuedPlaySessionRequest();
    }

#ifdef _OBJVIEWER
	ObjViewer->Update(DeltaTime);
#else
	ImguiManager.NewFrame();
    ToolBar.Process(Editor, ConsoleWindow, ControlPanelWindow, PropertyWindow);
    EditorViewportWindow.Process(Editor, DeltaTime);
    WorldOutliner.Process(Editor);
    ControlPanelWindow.Process(Editor);
    PropertyWindow.Process(Editor);
    ConsoleWindow.Process(Editor, [this](const char* Command) {ExecuteCommand(Command); });
    ContentsDrawer.Process(Editor);

    //WorldContext Tick
    for (size_t i = 0; i < WorldContexts.size(); ++i)
    {
        FWorldContext& WorldContext = WorldContexts[i];
        UWorld* World = WorldContext.World;
        if (!World) continue;

        if (WorldContext.WorldType == EWorldType::Editor || WorldContext.WorldType == EWorldType::EditorPreview)
        {
            World->Tick(FTimeManager::GetDeltaTime(), ELevelTick::LEVELTICK_ViewportsOnly);
        }

        if (WorldContext.WorldType == EWorldType::Game || WorldContext.WorldType == EWorldType::PIE)
        {
            World->Tick(FTimeManager::GetDeltaTime(), ELevelTick::LEVELTICK_All);
        }
    }

    Editor.Process();
#endif
}

void UEditorEngine::Render()
{
	Super::Render();

#ifdef _OBJVIEWER
    ObjViewer->Render();
#else
    TArray<TUniquePtr<FViewport>>& Viewports = Editor.GetViewports();

    // 렌더 준비
    RenderView.PrepareRender();

    {
        //컬링 준비 시간 기록?
        // 
        //이동한 오브젝트는 월드 AABB 재계산
        for (FWorldContext& WorldContext : WorldContexts)
        {
            WorldContext.World->GetScene()->UpdateDirtyBounds();
        }
    }

    //Active인 ViewportClient만 렌더링
    for (SWindow& Leaf : Editor.Leaf)
    {
        if (!Leaf.bisActive) continue;
        FViewport& Viewport = *Viewports[Leaf.EntryIndex];

        // 지금 이 뷰포트에 연결된 Client가 무엇이든(에디터/게임) 그 Client의 시점과 월드로 그린다.
        FViewportClient* ViewportClient = Viewport.GetClient();
        if (!ViewportClient) continue;

        FCamera Camera;
        if (!ViewportClient->GetViewInfo(Camera)) continue;

        // 뷰모드, 쇼플래그, 그리드는 에디터 Client의 설정을 쓴다.
        // TODO 클라이언트 별로 SceneView를 만들어 돌려주는 함수를 구현하면 게임 Client의 설정도 반영할 수 있다.
        FEditorViewportClient* EditorViewport = Editor.GetEditorClient(Leaf.EntryIndex);
        if (!EditorViewport) continue;

        // 뷰포트 렌더링 명세 구성
        FSceneView sceneview{
            .Camera = Camera,
            .ViewProj = Camera.GetViewProjectionMatrix(),
            .TopLeftUV = Viewport.TopLeftUV,
            .LengthUV = Viewport.LengthUV,
            .ViewMode = EditorViewport->ViewMode,
            .ShowFlags = EditorViewport->ShowFlags,
            .LightConstants = Editor.GlobalLight
        };

        // 에디터 렌더링 컨텍스트 구성
        // 게임 Client가 연결된 뷰포트(Play)에는 하이라이트, 기즈모, 이름표, 그리드, 비주얼라이저 같은 에디터 장식을 그리지 않는다.
        const bool bEditorTools = Editor.IsEditorClientAttached(Leaf.EntryIndex);

        FEditorRenderContext EditorCtx;
        EditorCtx.SelectedActor = bEditorTools ? Editor.GetSelectedActor() : nullptr;
        EditorCtx.SelectedTransform = Editor.SelectedTransform;
        EditorCtx.Gizmo = (bEditorTools && Editor.ObjectSelected()) ? &Editor.GetGizmo() : nullptr;
        EditorCtx.TextComp = (bEditorTools && Editor.ObjectSelected()) ? Editor.GetTextcomp() : nullptr;
        EditorCtx.Grid = bEditorTools ? &EditorViewport->GetGrid() : nullptr;
        EditorCtx.VisualizerRegistry = bEditorTools ? &VisualizerRegistry : nullptr;

        if (EditorCtx.SelectedActor) {
            if (USceneComponent* RootComp = EditorCtx.SelectedActor->GetRootComponent()) {
                EditorCtx.SelectedPrimitive = RootComp->Cast<UPrimitiveComponent>();
            }
        }

        // 뷰포트 렌더링 일괄 수행: 이 뷰포트가 보는 월드를 그린다 (PIE 중에는 PIE 월드)
        if (UWorld* ViewWorld = ViewportClient->GetWorld())
        {
            RenderView.RenderView(sceneview, *ViewWorld->GetScene(), EditorCtx);
        }

    }

    //기즈모 그리기
    if (Editor.ObjectSelected())
    {
        for (const SWindow& Leaf : Editor.Leaf)
        {
            if (!Leaf.bisActive)
                continue;

            // 게임 Client가 연결된 뷰포트(Play)에는 기즈모를 그리지 않는다.
            if (!Editor.IsEditorClientAttached(Leaf.EntryIndex))
                continue;

            const FViewport& Viewport = *Viewports[Leaf.EntryIndex];

            FViewportClient* ViewportClient = Viewport.GetClient();
            if (!ViewportClient)
                continue;

            FCamera Camera;
            if (!ViewportClient->GetViewInfo(Camera))
                continue;

            const FEditorViewportClient* EditorViewport = Editor.GetEditorClient(Leaf.EntryIndex);
            if (!EditorViewport)
                continue;

            FSceneView SceneView{
      .Camera = Camera,
      .ViewProj = Camera.GetViewProjectionMatrix(),
      .TopLeftUV = Viewport.TopLeftUV,
      .LengthUV = Viewport.LengthUV,
      .ViewMode = EditorViewport->ViewMode,
      .ShowFlags = EditorViewport->ShowFlags,
      .LightConstants = Editor.GlobalLight
            };

            RenderView.RenderOverlayPass(Camera, SceneView, Editor.SelectedTransform, Editor.GetGizmo(), Editor.GetTextcomp());
            // 마지막으로 그린 뷰의 렌더 모드가 남지 않도록 설정

            RenderView.SetRenderMode(EditorViewport->ViewMode);
            RenderView.RenderGizmo(
                Editor.SelectedTransform,
                Camera,
                Viewport.TopLeftUV,
                Viewport.LengthUV,
                Editor.GetGizmo());
        }
    }

    ImguiManager.RenderUI();
#endif
}

void UEditorEngine::ExecuteCommand(const char* Command)
{
    if (!Command) return;

    FString lowerCmd = Command;
    unsigned int NumberArg = 0;

    std::transform(lowerCmd.begin(), lowerCmd.end(), lowerCmd.begin(), ::tolower);

    if (lowerCmd.compare("stat memory") == 0) {
        UE_LOG("Stat Memory Command is executed!");
        EditorViewportWindow.Toggle(FImguiStatsWindow::EStatsWindow::Memory);
    }

    else if (lowerCmd.compare("stat fps") == 0) {
        UE_LOG("Stat FPS Command is executed!");
        EditorViewportWindow.Toggle(FImguiStatsWindow::EStatsWindow::FPS);
    }

    else if (lowerCmd.compare("stat unit") == 0) {
        UE_LOG("Stat unit Command is executed!");
        EditorViewportWindow.Toggle(FImguiStatsWindow::EStatsWindow::Unit);
    }

    else if (lowerCmd.compare("stat none") == 0) {
        UE_LOG("Stat Window is closed!");
        EditorViewportWindow.SetClose();
    }

    else if (lowerCmd.compare("stat cull") == 0)
    {
        UE_LOG("Stat Cull Command is executed!");
        //EditorViewportWindow.Toggle(FImguiStatsWindow::EStatsWindow::Cull);
    }

    else if (lowerCmd.compare("cull") == 0)
    {
        //컬링 토글
        Globals::bEnableFrustumCulling = !Globals::bEnableFrustumCulling;
        UE_LOG("Culling : %s", Globals::bEnableFrustumCulling ? "ON" : "OFF");
    }

    else {
        UE_LOG("Unknown command: '%s'\n", Command);
        return;
    }
}

//가장 마지막에 요청받은 PlaySession에 대한 World를 생성하고, 타깃 클라이언트의 월드를 PIE로 설정한다.
void UEditorEngine::StartQueuedPlaySessionRequest()
{
    StartQueuedPlaySessionRequestInternal();

    //성공 여부와 상관 없이 Request는 비운다.
    PlaySessionRequest.Reset();
}

void UEditorEngine::StartQueuedEndSessionRequest()
{
    if (!bRequestEndPlay) return;
    bRequestEndPlay = false;

    FPlaySession *PlayingSession = PlaySession.Get();
    if (!PlayingSession) return;

    // 월드를 파괴하기 전에 PIE 월드를 가리키는 참조를 정리한다.
    // 1) 선택: 선택된 액터가 PIE 월드의 것일 수 있으므로, 액터가 파괴되기 전에 해제한다.
    Editor.UnSelectActor();

    // 2) 뷰포트: PIE 월드를 보던 뷰포트를 원래 월드로 되돌린다.
    // 뷰포트에 에디터 Client(Play 상태에서는 게임 Client가 연결될 수 있음)의 월드를 원래대로 돌려 놓는다.
    if (FEditorViewportClient* TargetClient = Editor.GetEditorClient(PlayingSession->ViewEntryIndex))
    {
        TargetClient->SetContextId(PlayingSession->PrevViewContextId);

        // 뷰포트에 에디터 Client를 다시 연결한다. Play 상태에서는 게임 Client가 연결돼 있는데, 곧 파괴되므로
        // 먼저 떼어 놓지 않으면 뷰포트가 해제된 Client를 가리키게 된다. 이미 에디터 Client면 아무 일도 하지 않는다.
        TArray<TUniquePtr<FViewport>>& Viewports = Editor.GetViewports();
        if (PlayingSession->ViewEntryIndex < Viewports.size())
        {
            Viewports[PlayingSession->ViewEntryIndex]->SetViewportClient(TargetClient);
        }
    }

    DestroyWorld(PlayingSession->PIEContextId);
    PlaySession.Reset();
}

//PIE, SIE 전환 시 호출
void UEditorEngine::SwitchPlaySessionMode(FViewport *TargetViewport, EPlaySessionType SessionType)
{
    FPlaySession* Session = PlaySession.Get();
    if (!Session) return;

    //GameViewportClient를 가져와 교체한다.
    if (SessionType == EPlaySessionType::PIE)
    {
        FWorldContext* Context = FindWorldContext(Session->PIEContextId);
        if (!Context || !Context->GameViewportClient)
        {
            UE_LOG_ERROR("PIE 컨텍스트 또는 GameViewportClient를 찾을 수 없습니다.");
            return;
        }

        TargetViewport->SetViewportClient(Context->GameViewportClient);
    }
    else if (SessionType == EPlaySessionType::SIE)
    {
        FEditorViewportClient* EditorViewportClient = Editor.GetEditorClient(Session->ViewEntryIndex);
        TargetViewport->SetViewportClient(EditorViewportClient);
    }

    Session->SessionType = SessionType;
}

void UEditorEngine::StartQueuedPlaySessionRequestInternal()
{
    const FRequestPlaySessionParams* Params = PlaySessionRequest.Get();

    //이미 PIE가 실행 중이면 새로 시작하지 않는다.
    if (PlaySession.IsSet())
    {
        UE_LOG_ERROR("이미 PIE가 실행 중입니다. 먼저 종료해야 합니다.");
        return;
    }

    //1. 현재 포커스된 뷰포트의 월드 가져오기
    TArray<TUniquePtr<FViewport>>& Viewports = Editor.GetViewports();
    FEditorViewportClient* TargetClient = nullptr;
    uint32 TargetEntryIndex = 0;
    for (size_t i = 0; i < Viewports.size(); ++i)
    {
        if (Viewports[i]->IsFocused())
        {
            TargetClient = Editor.GetEditorClient(static_cast<int32>(i));
            TargetEntryIndex = static_cast<uint32>(i);
            break;
        }
    }

    //Focus된 Viewport가 없으면
    // 표시되고 있는 viewport중 첫 번째 Viewport가 보고있는 World를 복제한다.
    if (!TargetClient)
    {
        for (int i = 0; i < 4; ++i)
        {
            if (Editor.Leaf[i].bisActive)
            {
                TargetEntryIndex = Editor.Leaf[i].EntryIndex;
                TargetClient = Editor.GetEditorClient(static_cast<int32>(TargetEntryIndex));
                break;
            }
        }
    }

    if (!TargetClient)
    {
        UE_LOG_ERROR("활성화 되어 있는 Viewport 및 Leaf가 없다. PIE 생성 실패, 코드 확인 필요");
        return;
    }

    //2. 월드 복제하기: 타깃 뷰포트가 보는 월드를 Serialize해서 복제 원본으로 쓴다.
    UWorld* TargetWorld = TargetClient->GetWorld();
    if (!TargetWorld)
    {
        UE_LOG_ERROR("타깃 뷰포트가 보는 월드가 없어 PIE를 시작할 수 없습니다.");
        return;
    }
    FArchive TempArchive;
    TargetWorld->Serialize(TempArchive);

    //3. PIE 월드 생성 (Initialize -> Deserialize -> 컨텍스트 등록 -> Activate -> BeginPlay)
    const uint32 PIEContextId = CreateWorld(Params->WorldType, &TempArchive);
    if (PIEContextId == InvalidContextId)
    {
        UE_LOG_ERROR("PIE 월드 생성에 실패했습니다.");
        return;
    }

    // 선택된 액터는 에디터 월드의 것이므로, 뷰포트가 PIE 월드를 보기 전에 선택을 해제한다.
    Editor.UnSelectActor();
    
    //4. PIE용 GameViewportClient를 생성한다.
    // ContextId는 배열 인덱스가 아니므로 반드시 조회 함수로 찾는다.
    FWorldContext* PIEContext = FindWorldContext(PIEContextId);
    if (!PIEContext)
    {
        UE_LOG_ERROR("생성한 PIE 컨텍스트를 찾을 수 없습니다.");
        DestroyWorld(PIEContextId);
        return;
    }

    FGameViewportClient* GameViewportClient = new FGameViewportClient(this, PIEContextId, TargetClient->GetCamera());
    PIEContext->GameViewportClient = GameViewportClient;

    EPlaySessionType SessionType = Params->SessionType;
    const uint32 PrevContextId = TargetClient->GetContextId();
 
    //4. 클라이언트들의 ContextId를 PIE용으로 교체한다.
    TargetClient->SetContextId(PIEContextId);
    GameViewportClient->SetContextId(PIEContextId);

    //5. 현재 Session 상태를 저장한다.
    PlaySession.Set(FPlaySession(PIEContextId, TargetEntryIndex, PrevContextId, Params->SessionType));

    //6. PIE인 경우, SessionMode를 PIE로 전환한다.
    if (SessionType == EPlaySessionType::PIE)
    {
        //현재 Focus된 viewport의 클라이언트를 GameViewportClient로 바꾼다.
        FViewport* TargetViewport = TargetClient->GetViewport();
        SwitchPlaySessionMode(TargetViewport, EPlaySessionType::PIE);
    }
}
