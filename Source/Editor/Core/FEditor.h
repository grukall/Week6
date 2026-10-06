    #pragma once

#include "Editor/EditorViewport/FEditorViewportClient.h"
#include "Editor/Gizmo/FGizmo.h"
#include "Editor/Core/FEditorState.h"
#include "Runtime/Core/IntTypes.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/CoreUObject/TWeakObjectPtr.h"
#include "Runtime/Actors/AActor.h"
#include "Runtime/Rendering/ShaderConstants.h"
#include "Runtime/CoreUObject/UTextInstanceComponent.h"
#include "Runtime/UI/SSplitter.h"
#include "Runtime/Slate/FViewport.h"
#include "Runtime/Engine/UWorld.h"

class UEditorEngine;
class UEngine;

enum class EEditorPrimitiveType : uint8 {
  Cube,
  Cylinder,
  Sphere,
  Billboard,
  Spotlight,
};

// Viewport와 Client는 서로의 주소를 가리키므로 엔트리의 주소가 바뀌면 안 된다.
// 복사/이동을 막고 FEditor는 TUniquePtr로만 들고 있는다.
struct FEditorViewportEntry {

    // Viewport보다 먼저 선언해야 소멸 순서(역순)상 Viewport가 먼저 정리되어
    // Viewport의 소멸자가 살아있는 Client를 상대로 연결을 해제할 수 있다.
    FEditorViewportClient Client;
    FViewport Viewport;

    explicit FEditorViewportEntry(UEngine* Engine, FWorldContext &Context) : Client(Engine, Context.ContextId)
    {
        Viewport.SetViewportClient(&Client);
    }

    FEditorViewportEntry(const FEditorViewportEntry&) = delete;
    FEditorViewportEntry& operator=(const FEditorViewportEntry&) = delete;
};

class FEditor {
public:
  FTransform SelectedTransform;
  FVector SelectedEulerDegDisplay;

  // TODO: 이건 Scene에 들어가야함. 아마 아래와 같은 컴포넌트가 부착된 액터로 들어가야할 것
  // https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UDirectionalLightComponent
  FLightConstants GlobalLight;

  FEditorState State;

  // 피킹 경로 선택 및 측정. 검증이 끝나면 제거한다.
  bool bUseBVHPicking = true;
  bool bHideUI = false;
  // F11. bHideUI가 숨기는 창에 더해 툴바까지 숨긴다.
  bool bZenMode = false;
  // 컴포넌트 추가/삭제/재부착처럼 계층이 바뀔 때 올린다. 아웃라이너가 캐시 갱신에 쓴다.
  uint32 HierarchyVersion = 0;
  bool bShowBenchmark = true;
  double LastPickingMs = 0.0;
  double AccumulatedPickingMs = 0.0;
  int32 PickingAttempts = 0;

  void ResetPickingStats()
  {
    LastPickingMs = 0.0;
    AccumulatedPickingMs = 0.0;
    PickingAttempts = 0;
  }

public:
  void Initialize(UEditorEngine* Engine);
  void Shutdown();

  void Process();

  // 월드의 소유자는 UEditorEngine이다. FEditor는 접근만 위임한다.
  // 에디터 월드: 저장/로드/New의 대상. PIE 중에도 항상 에디터 월드를 가리킨다.
  [[nodiscard]] UWorld* GetEditorWorld() const;
  // 활성 뷰포트가 보는 월드: 아웃라이너, 선택, 피킹, 기즈모, 스폰의 대상. PIE 중에는 PIE 월드가 된다.
  [[nodiscard]] UWorld* GetViewWorld() const;

  void NewMap();
  void SaveMap(const FString &Path);
  void LoadMap(const FString &Path);

  void AddViewport(UEngine* Engine, FWorldContext& Context);
  void InitMultiViewport(UEngine* Engine, FWorldContext& Context);
  void ResizeView(FEditorState::SplitViewMode mode);
  void DeleteViewport(int32 IndexOfViewport);
  FEditorViewportEntry* GetActiveViewport();
  // 활성 뷰포트의 Client만 필요한 곳(카메라, 뷰모드 등)에서 쓴다.
  FEditorViewportClient* GetActiveViewportClient();

  void UpdateCamera();
  void RefreshSelectedTransform();
  bool SelectComponent(UActorComponent* InActorComponent);
  bool SelectActor(AActor *Actor);
  void UnSelectActor();
  AActor *GetSelectedActor() const { return SelectedActor.Get(); }
  [[nodiscard]] bool ActorSelected() const { return SelectedActor.IsValid(); }
  [[nodiscard]] bool ObjectSelected() const { return SelectedActor.IsValid(); }

  UActorComponent* GetSelectedComponent() const { return SelectedComponent.Get(); }
  [[nodiscard]] bool ComponentSelected() const { return SelectedComponent.IsValid(); }

  [[nodiscard]] TArray<TUniquePtr<FEditorViewportEntry>> &GetViewports() {return  Entries;}
  void SpawnActorToCurrentScene(UClass* Type, int Count = 1);
  // 피킹 등에서 현재 씬의 렌더링 대상 컴포넌트가 필요할 때 사용
  [[nodiscard]] const TArray<UPrimitiveComponent*>& GetPrimitiveComponents() const;
  FGizmo &GetGizmo() { return Gizmo; }
  FRenderResourceLibrary *GetRendererLibrary();

  void ClearSelectionForGC();

  void SaveState();
  void LoadState();
  void SetViewLayout(FEditorState::SplitViewMode mode);
  UTextInstanceComponent* GetTextcomp() { return SelectedActorTextComp; }
  UEditorEngine* GetEditorEngine() const { return EditorEngine; }
  
 //Viewport관련
  int32 ActiveViewportIndex = 0;
  SWindow* Root=nullptr;
  SWindow Leaf[4];
  SSplitterH HorizonSplitter; //세로선
  SSplitterH HorizonSplitter2; //세로선
  SSplitterV VerticalSplitter; // 가로선

private:
  UEditorEngine* EditorEngine = nullptr;
  TArray<TUniquePtr<FEditorViewportEntry>> Entries;
  FGizmo Gizmo;
  TWeakObjectPtr<AActor> SelectedActor;
  TWeakObjectPtr<UActorComponent> SelectedComponent;
  TWeakObjectPtr<UTextInstanceComponent> SelectedActorTextComp;
};
