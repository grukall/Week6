#include "FEditor.h"
#include "Runtime/Actors/AActor.h"
#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/Engine/FTimeManager.h"
#include "Runtime/Input/FInputManager.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Math/Random.h"
#include "Runtime/Asset/FAssetRegistry.h"
#include <numbers>
#include "Runtime/Engine/FSceneBVH.h"
#include "Editor/Engine/UEditorEngine.h"
#include "Runtime/Engine/UWorld.h"

void FEditor::Initialize(UEditorEngine *Engine) {
  State.ReadFromFile();
  Gizmo.Initialize();
  SelectedActorTextComp = NewObject<UTextInstanceComponent>();
  if (SelectedActorTextComp)
  {
    SelectedActorTextComp->Initialize();
    SelectedActorTextComp->SetInheritRotation(false);
    FAssetRegistry& Registry = FAssetRegistry::GetInstance();
    SelectedActorTextComp->SetMesh(Registry.Get<UStaticMesh>("#Rect"));
    SelectedActorTextComp->SetMaterial(Registry.Get<UMaterial>("Material/SelectedActor_Text.json"));
    SelectedActorTextComp->SetFont(FName("bazziotf"));
  }

  this->EditorEngine = Engine;
}

void FEditor::Shutdown() {
  SaveState();
  State.FlushToFile();
}

FRenderResourceLibrary *FEditor::GetRendererLibrary() {
  return &FRenderResourceLibrary::Get();
}

void FEditor::Process()
{
    if (FInputManager::Get().IsKeyDown(VK_F11))
    {
        bZenMode = !bZenMode;
    }

    // 씬의 액터 업데이트
    if (FInputManager::Get().IsKeyDown(VK_DELETE) && SelectedActor)
    {

        if (SelectedComponent)
        {
            // 선택을 먼저 풀어야 삭제된 컴포넌트의 트랜스폼이 액터 루트에 적용되지 않는다.
            AActor* OwnerActor = SelectedComponent->GetActorOwner();
            UActorComponent* TempComponent = SelectedComponent;
            UnSelectActor();
            if (OwnerActor)
            {
                OwnerActor->DeleteComponent(TempComponent);
                SelectActor(OwnerActor);
                HierarchyVersion++;
            }
        }
        else
        {
            AActor* Target = SelectedActor;
            UnSelectActor();
            Target->Destroy();
        }
    }

    // 활성 뷰포트가 다른 월드를 보게 되면(PIE 시작/종료, 뷰포트 전환) 선택이 그 월드의 액터가 아니므로 해제한다.
    if (SelectedActor && SelectedActor->GetWorld() != GetViewWorld())
    {
        UnSelectActor();
    }


    // 선택된 액터가 속한 월드의 BVH만 갱신한다.
    if (SelectedActor)
    {
        UWorld* World = SelectedActor->GetWorld();

        // 씬 컴포넌트가 아닌 컴포넌트가 선택되면 액터의 루트를 움직인다.
        USceneComponent* Target = SelectedComponent ? SelectedComponent->Cast<USceneComponent>() : nullptr;
        if (!Target)
        {
            Target = SelectedActor->GetRootComponent();
        }

        if (Target && bChangedByGizmo)
        {
            // World면 월드 기준으로, Local이면 현재 회전 기준으로 기즈모 변화량만 적용한다.
            FTransform TargetTransform = Target->GetGlobalTransform();
            const FQuaternion TargetRotation = TargetTransform.GetRotation();
            if (Gizmo.GetSpace() == EGizmoSpace::World)
            {
                TargetTransform.SetLocation(TargetTransform.GetLocation() + GapTransform.GetLocation());
                TargetTransform.SetRotation((GapTransform.GetRotation() * TargetRotation).Normalized());
            }
            else
            {
                TargetTransform.SetLocation(TargetTransform.GetLocation() + TargetRotation.RotateVector(GapTransform.GetLocation()));
                TargetTransform.SetRotation((TargetRotation * GapTransform.GetRotation()).Normalized());
            }
            TargetTransform.SetScale3D(TargetTransform.GetScale3D() + GapTransform.GetScale3D());
            Target->SetRelativeTransformFromGlobal(TargetTransform);
        }

        // 다른 컴포넌트가 바꾼 Transform도 기즈모가 따라가도록 매 프레임 실제 Transform과 동기화한다.
        if (Target)
        {
            SelectedTransform = Target->GetGlobalTransform();
        }
        // Transform이 변경되었을 때만 Refit
        if (bChangedByGizmo && World && World->GetScene())
        {
            RefitActorInBVH(World->GetScene()->GetSceneBVH(), SelectedActor);
        }
        bChangedByGizmo = false;
    }

  SaveState();
  State.Tick(FTimeManager::GetDeltaTime());
}

void FEditor::SaveState() {
  // 저장하는 것은 에디터 카메라다. 뷰포트에 게임 Client가 연결돼 있어도 에디터 상태를 저장한다.
  FEditorViewportClient* EditorClient = GetActiveEditorClient();
  if (!EditorClient) { return; }

  const FCamera *Camera = EditorClient->GetCamera();
  State.SetCameraLocation(Camera->GetPosition());
  State.SetCameraPitch(Camera->GetPitch());
  State.SetCameraYaw(Camera->GetYaw());
  State.SetCameraFOV(Camera->GetProjection().GetFOV());

  State.SetGridCellSize(EditorClient->GetGrid().GetCellSize());
  State.SetGizmoMode(static_cast<uint8>(Gizmo.Mode));
  State.SetGizmoSpace(static_cast<uint8>(Gizmo.GetSpace()));
  // 선택된 액터는 저장하지 않는다. 런타임 식별자는 실행마다 달라지므로 파일 사이에서 의미가 없다.
}

void FEditor::LoadState()
{
    FEditorViewportClient* EditorClient = GetActiveEditorClient();
    if (!EditorClient) { return; }

    FCamera *Camera = EditorClient->GetCamera();
    Camera->SetPosition(State.GetCameraLocation());
    Camera->SetRotation(State.GetCameraPitch(), State.GetCameraYaw());
    Camera->SetFOV(State.GetCameraFOV());

    EditorClient->GetGrid().SetCellSize(State.GetGridCellSize());
    Gizmo.Mode = static_cast<EGizmoMode>(State.GetGizmoMode());
    Gizmo.SetGizmoSpace(static_cast<EGizmoSpace>(State.GetGizmoSpace()));

    //viewmode관련
    VerticalSplitter.Ratio = State.GetSplitter().X;
    HorizonSplitter.Ratio = State.GetSplitter().Y;
    HorizonSplitter2.Ratio = State.GetSplitter().Z;
    
}

UWorld* FEditor::GetEditorWorld() const {
  return EditorEngine ? EditorEngine->GetEditorWorld() : nullptr;
}

UWorld* FEditor::GetViewWorld() const {
  if (Viewports.empty() || ActiveViewportIndex < 0 || ActiveViewportIndex >= static_cast<int32>(Viewports.size())) {
    return nullptr;
  }
  FViewportClient* Client = Viewports[ActiveViewportIndex]->GetClient();
  return Client ? Client->GetWorld() : nullptr;
}

void FEditor::NewMap() {
  if (EditorEngine->IsPlaySessionActive()) {
    UE_LOG_WARN("[NewMap] PIE 실행 중에는 새 씬을 만들 수 없습니다. 먼저 PIE를 종료하세요.");
    return;
  }

  // 이전 월드가 파괴되기 전에 선택을 해제한다.
  UnSelectActor();
  EditorEngine->NewMap(GetEditorWorld(), EWorldType::Editor);
  State.ResetToDefaults();
  LoadState();
}

void FEditor::SaveMap(const FString &Path)
{
  if (EditorEngine->IsPlaySessionActive()) {
    UE_LOG_WARN("[SaveMap] PIE 실행 중에는 씬을 저장할 수 없습니다. 먼저 PIE를 종료하세요.");
    return;
  }

  if (UWorld* World = GetEditorWorld()) {
    EditorEngine->SaveMap(*World, Path);
  }
}

void FEditor::LoadMap(const FString &Path)
{
  if (EditorEngine->IsPlaySessionActive()) {
    UE_LOG_WARN("[LoadMap] PIE 실행 중에는 씬을 불러올 수 없습니다. 먼저 PIE를 종료하세요.");
    return;
  }

  // 이전 월드가 파괴되기 전에 선택을 해제한다.
  UnSelectActor();

  EditorEngine->LoadMap(GetEditorWorld(), Path);
}

void FEditor::AddViewport(UEngine* Engine, FWorldContext& Context)
{
   // 두 배열은 항상 함께 추가한다. (같은 인덱스가 같은 뷰포트)
   EditorViewportClients.push_back(MakeUnique<FEditorViewportClient>(Engine, Context.ContextId, this));
   Viewports.push_back(MakeUnique<FViewport>());

   Viewports.back()->SetViewportClient(EditorViewportClients.back().get());
}
void FEditor::InitMultiViewport(UEngine* Engine, FWorldContext& Context) {
    for (int32 i = 0; i < 4; ++i) {
        AddViewport(Engine, Context);
    }
}
void FEditor::DeleteViewport(int32 IndexOfViewport) {
   // 뷰포트를 먼저 지워 연결을 끊은 뒤 Client를 지운다.
   Viewports.erase(Viewports.begin() + IndexOfViewport);
   EditorViewportClients.erase(EditorViewportClients.begin() + IndexOfViewport);
}

FViewport* FEditor::GetActiveViewport() {
  if (Viewports.empty() || ActiveViewportIndex < 0 || ActiveViewportIndex >= static_cast<int32>(Viewports.size())) {
    return nullptr;
  }
  return Viewports[ActiveViewportIndex].get();
}

FViewportClient* FEditor::GetActiveViewportClient() {
  FViewport* Viewport = GetActiveViewport();
  return Viewport ? Viewport->GetClient() : nullptr;
}

FEditorViewportClient* FEditor::GetEditorClient(int32 Index) {
  if (Index < 0 || Index >= static_cast<int32>(EditorViewportClients.size())) {
    return nullptr;
  }
  return EditorViewportClients[Index].get();
}

FEditorViewportClient* FEditor::GetActiveEditorClient()
{
  return GetEditorClient(ActiveViewportIndex);
}

bool FEditor::IsSelectionInWorld(const UWorld* World) const
{
  return World != nullptr && SelectedActor && SelectedActor->GetWorld() == World;
}

bool FEditor::IsEditorClientAttached(int32 Index) const
{
  if (Index < 0 || Index >= static_cast<int32>(Viewports.size()) || Index >= static_cast<int32>(EditorViewportClients.size())) {
    return false;
  }
  return Viewports[Index]->GetClient() == EditorViewportClients[Index].get();
}

bool FEditor::SelectActor(AActor *Actor) {
    if (SelectedActor) {
        UnSelectActor();
    }

    SelectedActor = Actor;
    if (SelectedActor) {
        USceneComponent* RootComponent = SelectedActor->GetRootComponent();
        SelectedTransform = RootComponent ? RootComponent->GetGlobalTransform() : FTransform{};
        SelectedEulerDegDisplay = SelectedTransform.GetRotation().GetEulerXYZ();
    if (Gizmo.Mode == EGizmoMode::None) {
        Gizmo.Mode = EGizmoMode::Translate;
    }

    if (SelectedActorTextComp) {
        SelectedActorTextComp->SetActorOwner(SelectedActor.Get());
        FTransform RelativeTrans;
        RelativeTrans.SetLocation(FVector{ 0.0f, 0.0f, 1.5f });
        SelectedActorTextComp->SetRelativeTransform(RelativeTrans);
        const FString ActorName = SelectedActor->GetName().ToString();
        SelectedActorTextComp->SetText(L"Name : " + FWString(ActorName.begin(), ActorName.end()));
    }
    }

    return true;
}

bool FEditor::SelectComponent(UActorComponent* InActorComponent)
{
    if (!InActorComponent)
    {
        SelectedComponent = nullptr;
        return false;
    }

    if (InActorComponent->GetActorOwner() != SelectedActor.Get())
    {
        SelectActor(InActorComponent->GetActorOwner());
    }

    SelectedComponent = InActorComponent;

    RefreshSelectedTransform();
    if (Gizmo.Mode == EGizmoMode::None) {
        Gizmo.Mode = EGizmoMode::Translate;
    }

    return true;
}

void FEditor::UnSelectActor() {
    if (SelectedActor)
    {
        USceneComponent* Target = SelectedComponent ? SelectedComponent->Cast<USceneComponent>() : nullptr;
        if (!Target)
        {
            Target = SelectedActor->GetRootComponent();
        }
        if (Target)
        {
            Target->SetRelativeTransformFromGlobal(SelectedTransform);
        }
    }
    SelectedActor = nullptr;
    SelectedComponent = nullptr;
    if (SelectedActorTextComp) {
        SelectedActorTextComp->SetActorOwner(nullptr);
    }
}

void FEditor::RefreshSelectedTransform()
{
    if (!SelectedActor) { return; }

    USceneComponent* Target = SelectedComponent ? SelectedComponent->Cast<USceneComponent>() : nullptr;
    if (!Target)
    {
        Target = SelectedActor->GetRootComponent();
    }
    if (!Target) { return; }

    SelectedTransform = Target->GetGlobalTransform();
    SelectedEulerDegDisplay = SelectedTransform.GetRotation().GetEulerXYZ();
}

const TArray<UPrimitiveComponent *> &FEditor::GetPrimitiveComponents() const {
    static const TArray<UPrimitiveComponent*> Empty;
  UWorld* World = GetViewWorld();
  if (!World || !World->GetScene()) {
    return Empty;
  }
  return World->GetScene()->GetPrimitives();
}

void FEditor::ClearSelectionForGC() {
  SelectedActor = nullptr;
  SelectedComponent = nullptr;
  Gizmo.EndInteraction();
  Gizmo.HoveredHandle = EGizmoHandle::None;
}

void FEditor::SpawnActorToCurrentScene(UClass* Type, int Size)
{
    UWorld* World = GetViewWorld();
    FScene* Scene = World ? World->GetScene() : nullptr;
    if (!Scene)
    {
        return;
    }

    if (Size <= 0) { return; }

    const float Min = State.GetSpawnActorMinLocation();
    const float Max = State.GetSpawnActorMaxLocation();
    if (Min > Max) { return; }

    for (int i = 0; i < Size; ++i)
    {
        FVector Location
        {
            Random::GetFloat(Min, Max, 2),
            Random::GetFloat(Min, Max, 2),
            Random::GetFloat(Min, Max, 2),
        };

        AActor* NewActor = World->SpawnActor(Type);
        if (!NewActor) { return; }


        FTransform CurrentTransform = NewActor->GetTransform();
        CurrentTransform.SetLocation(Location);
        CurrentTransform.SetScale3D(FVector{ 0.5f, 0.5f, 0.5f });
        NewActor->SetTransform(CurrentTransform);

        SelectActor(NewActor);
    }

    FSceneBVH& BVH = Scene->GetSceneBVH();
    if (BVH.ShouldRebuild())
    {
        BVH.Build(Scene->GetPrimitives());
    }
}

void FEditor::ResizeView(FEditorState::SplitViewMode mode)
{
    //viewport를 가지고있는 splitter,window를 업데이트
    ActiveViewportIndex = 0;
    //=== 초기화 ===//
    for (int32 i = 0; i < 4; ++i)
    {
        Leaf[i].EntryIndex = i;
        Leaf[i].bisActive = false;
    }

    HorizonSplitter2.bisActive = false;
    VerticalSplitter.bisActive = false;
    HorizonSplitter.bisActive = false;
    //=== 초기화 ===//

    //===람다함수===//
    auto Connect = [](SSplitter& Splitter, SWindow& LT, SWindow& RB)
        {
            Splitter.SideLT = &LT;
            Splitter.SideRB = &RB;

            Splitter.bisActive = true;
            LT.bisActive = true;
            RB.bisActive = true;
        };

    switch (mode)
    {
        case FEditorState::SplitViewMode::SINGLE:
        Leaf[0].bisActive = true;
        Root = &Leaf[0];
        break;

    case FEditorState::SplitViewMode::HORIZONTAL:
        Leaf[0].bisActive = true;
        Leaf[1].bisActive = true;
        Connect(HorizonSplitter, Leaf[0], Leaf[1]);
        Root = &HorizonSplitter;
        break;

    case FEditorState::SplitViewMode::VERTICAL:
        Leaf[0].bisActive = true;
        Leaf[2].bisActive = true;
        Connect(VerticalSplitter, Leaf[0], Leaf[2]);
        Root = &VerticalSplitter;
        break;

    case FEditorState::SplitViewMode::QUAD:
        Leaf[0].bisActive = true;
        Leaf[1].bisActive = true;
        Leaf[2].bisActive = true;
        Leaf[3].bisActive = true;
        Connect(VerticalSplitter, HorizonSplitter, HorizonSplitter2);
        Connect(HorizonSplitter, Leaf[0], Leaf[1]);
        Connect(HorizonSplitter2, Leaf[2], Leaf[3]);
        Root = &VerticalSplitter;
        break;
    }
}
void FEditor::SetViewLayout(FEditorState::SplitViewMode mode) {
    ResizeView(mode);

    auto SetPerspectiveView = [this](int32 EntryIndex)
    {
        FEditorViewportClient* Client = GetEditorClient(EntryIndex);
        Client->eOrthogonalType = FEditorViewportClient::EOrthogonalType::PERSPECTIVE;
        Client->GetCamera()->SetProjectionType(EProjectionType::Perspective);
    };

    auto SetOrthographicView = [this](int32 EntryIndex, FEditorViewportClient::EOrthogonalType Type)
    {
        FEditorViewportClient* Client = GetEditorClient(EntryIndex);
        Client->SetOrthograpihcView(Type);
    };

    switch (mode)
    {
    case FEditorState::SplitViewMode::SINGLE:
        VerticalSplitter.bisActive = false;
        HorizonSplitter.bisActive = false;
        HorizonSplitter2.bisActive = false;
        SetPerspectiveView(0);
        State.SetSplitMode(FEditorState::SplitViewMode::SINGLE);
        break;

    case FEditorState::SplitViewMode::VERTICAL:
        VerticalSplitter.bisActive = true;
        HorizonSplitter.bisActive = false;
        HorizonSplitter2.bisActive = false;
        SetOrthographicView(0, FEditorViewportClient::EOrthogonalType::ORTHOGRAPHIC_TOP);
        SetPerspectiveView(2);
        State.SetSplitMode(FEditorState::SplitViewMode::VERTICAL);
        break;

    case FEditorState::SplitViewMode::HORIZONTAL:
        VerticalSplitter.bisActive = false;
        HorizonSplitter.bisActive = true;
        HorizonSplitter2.bisActive = false;
        SetOrthographicView(0, FEditorViewportClient::EOrthogonalType::ORTHOGRAPHIC_TOP);
        SetPerspectiveView(1);
        State.SetSplitMode(FEditorState::SplitViewMode::HORIZONTAL);
        break;

    case FEditorState::SplitViewMode::QUAD:
        VerticalSplitter.bisActive = true;
        HorizonSplitter.bisActive = true;
        HorizonSplitter2.bisActive = true;
        SetOrthographicView(0, FEditorViewportClient::EOrthogonalType::ORTHOGRAPHIC_TOP);
        SetPerspectiveView(1);
        SetOrthographicView(2, FEditorViewportClient::EOrthogonalType::ORTHOGRAPHIC_FRONT);
        SetOrthographicView(3, FEditorViewportClient::EOrthogonalType::ORTHOGRAPHIC_RIGHT);
        State.SetSplitMode(FEditorState::SplitViewMode::QUAD);
        break;

    }
}

