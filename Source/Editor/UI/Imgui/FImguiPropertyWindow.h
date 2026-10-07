#pragma once
#include "Editor/Core/FEditor.h"

class AActor;
class USceneComponent;
class UStaticMeshComponent;
class UDirectionLightComponent;
class UPointLightComponent;
class USpotLightComponent;
class UProjectileMovementComponent;
class URotationMovementComponent;
class UTextInstanceComponent;
class UBillBoardComp;
class UAnimatedBillboardComp;

// 선택된 액터의 컴포넌트 속성을 편집하는 창.
class FImguiPropertyWindow final {
public:
	FImguiPropertyWindow() = default;
	~FImguiPropertyWindow() = default;

	//복사 생성 금지
	FImguiPropertyWindow(const FImguiPropertyWindow&) = delete;
	//복사 대입 금지
	FImguiPropertyWindow& operator=(const FImguiPropertyWindow&) = delete;

	void Process(FEditor& Editor);

private:
	// 액터 클래스명과 UUID.
	void ShowActorHeader(const AActor& Actor) const;

	// 컴포넌트 계층 트리. 노드를 클릭하면 선택하고, 드래그해서 같은 액터 안의 다른 컴포넌트 밑으로 옮긴다.
	void ShowComponentHierarchy(FEditor& Editor, AActor& Actor);
	void ShowComponentTreeNode(FEditor& Editor, AActor& Actor, USceneComponent& Comp);
	// 씬 컴포넌트가 아닌 컴포넌트. 계층이 없어서 트리 아래에 나열만 하고 드래그는 받지 않는다.
	void ShowActorComponentList(FEditor& Editor, AActor& Actor);
	void ShowActorComponentSections(AActor& Actor);
	void ShowComponentButtons(FEditor& Editor, AActor& Actor);
	// 드롭은 트리 순회 중에 일어나므로 기억만 해두고 순회가 끝난 뒤 처리한다.
	USceneComponent* PendingDragged = nullptr;
	USceneComponent* PendingTarget = nullptr;

	// 컴포넌트마다 접이식 헤더를 만들고 그 안에 상세 속성을 그린다.
	void ShowComponentSections(FEditor& Editor, AActor& Actor);
	void ShowComponentDetails(FEditor& Editor, AActor& Actor, USceneComponent& Comp, bool bIsRoot);

	// 루트는 에디터 기즈모와 동기화되고, 서브는 상대 트랜스폼을 편집한다.
	void ShowTransform(FEditor& Editor, USceneComponent& Comp, bool bIsRoot) const;

	// 컴포넌트 타입별 속성
	void ShowTextSettings(UTextInstanceComponent& TextComp) const;
	void ShowBillboardSettings(UBillBoardComp& BillboardComp) const;
	void ShowAnimatedBillboardSettings(UAnimatedBillboardComp& BillboardComp) const;
	void ShowStaticMeshSettings(AActor& Actor, UStaticMeshComponent& MeshComp, bool bIsRoot) const;
	void ShowDirectionLightSettings(UDirectionLightComponent& LightComp) const;
	void ShowPointLightSettings(UPointLightComponent& LightComp) const;
	void ShowSpotLightSettings(USpotLightComponent& LightComp) const;
	void ShowProjectileMovementSettings(UProjectileMovementComponent& MovComp) const;
	void ShowRotationMovementSettings(URotationMovementComponent& MovComp) const;


	// 머티리얼의 텍스처 미리보기 겸 드롭 타깃.
	void ShowMaterialSlot(UStaticMeshComponent& MeshComp, int Slot = 0) const;
	void ShowPipelineSlot(UStaticMeshComponent& MeshComp, int Slot = 0) const;
	void ShowTextureSlot(UStaticMeshComponent& MeshComp, int Slot = 0) const;
	void ShowStaticMeshSlot(UStaticMeshComponent& MeshComp) const;

	void ShowApplyAllMaterialSlot(UStaticMeshComponent& MeshComp) const;
	void ShowApplyAllPipelineSlot(UStaticMeshComponent& MeshComp) const;
	void ShowApplyAllTextureSlot(UStaticMeshComponent& MeshComp) const;

	// 창 하단의 기즈모 모드/공간 선택.
	void ShowGizmoSettings(FEditor& Editor) const;
};
