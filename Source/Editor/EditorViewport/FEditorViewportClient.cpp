#include "FEditorViewportClient.h"
#include "Runtime/Engine/UWorld.h"
#include "Runtime/Slate/FViewport.h"
#include "Runtime/Engine/UEngine.h"
#include "Editor/Core/FEditor.h"
#include "Runtime/Input/FInputManager.h"

void FEditorViewportClient::SetOrthograpihcView(FEditorViewportClient::EOrthogonalType type)
{
	float distance = 5.0f;
	eOrthogonalType = type;
	ViewportCamera.SetProjectionType(EProjectionType::Orthographic);
	switch (type)
	{
	case EOrthogonalType::ORTHOGRAPHIC_TOP:
		ViewportCamera.SetPosition(FVector(0.0f, 0.0f, distance));
		ViewportCamera.SetRotation(-90.0f, 0.0f);
		break;
	case EOrthogonalType::ORTHOGRAPHIC_BOTTOM:
		ViewportCamera.SetPosition(FVector(0.0f, 0.0f, -distance));
		ViewportCamera.SetRotation(90.0f, 0.0f);
		break;
	case EOrthogonalType::ORTHOGRAPHIC_LEFT:
		ViewportCamera.SetPosition(FVector(0.0f, -distance, 0.0f));
		ViewportCamera.SetRotation(0.0f, 90.0f);
		break;

	case EOrthogonalType::ORTHOGRAPHIC_RIGHT:
		ViewportCamera.SetPosition(FVector(0.0f, distance, 0.0f));
		ViewportCamera.SetRotation(0.0f, -90.0f);
		break;

	case EOrthogonalType::ORTHOGRAPHIC_FRONT:
		ViewportCamera.SetPosition(FVector(distance, 0.0f, 0.0f));
		ViewportCamera.SetRotation(0.0f, 0.0f);
		break;

	case EOrthogonalType::ORTHOGRAPHIC_BACK:
		ViewportCamera.SetPosition(FVector(-distance, 0.0f, 0.0f));
		ViewportCamera.SetRotation(0.0f, 180.0f);
		break;
	}
}

void FEditorViewportClient::AddAssociation(FViewport& _Viewport)
{
	Viewport = &_Viewport;
}

void FEditorViewportClient::RemoveAssociation(FViewport & _Viewport)
{
	Viewport = nullptr;
}

void FEditorViewportClient::ProccessInput(const FViewportInput& Input, float deltaTime)
{
    // 클릭 전에도 마우스가 올라간 뷰포트에서 매 프레임 검사한다.
    FGizmo& Gizmo = Editor->GetGizmo();

    // 선택된 액터가 이 뷰포트가 보는 월드의 것일 때만 기즈모를 검사한다. (다른 월드의 기즈모는 이 뷰포트에 그려지지 않는다)
    if (Editor->IsSelectionInWorld(GetWorld()) && Input.bHovered && !Gizmo.IsInteracting())
    {
        UpdateGizmoHover(Input.LocalMouse, Input.SizePixels);
    }

	UpdateSelection(Input);
	UpdateGizmo(Input);
	UpdateCamera(Input, deltaTime);
}


void FEditorViewportClient::UpdateSelection(const FViewportInput& Input)
{
    if (Input.bPickRequested)
    {
        HandlePicking(Input.LocalMouse, Input.SizePixels);
    }
}

void FEditorViewportClient::UpdateGizmo(const FViewportInput& Input)
{
    FGizmo& Gizmo = Editor->GetGizmo();

    // Hover와 종료는 Process에서 처리하고, 여기서는 진행 중인 드래그만 갱신한다.
    if (Editor->ObjectSelected() && Gizmo.IsInteracting() && Input.bLeftDown)
    {
        Gizmo.UpdateInteraction(*Editor, Input.LocalMouse, ViewportCamera, Input.SizePixels);
        Editor->bChangedByGizmo = true;
    }
}

void FEditorViewportClient::UpdateCamera(const FViewportInput& Input, float DeltaTime)
{
    if (!Input.bFocused)
    {
        CameraController.ResetVelocity();
        return;
    }

    CameraController.CameraRotateSpeed = Editor->State.GetCameraSensitivity();
    CameraController.CameraMoveSpeed = Editor->State.GetCameraSpeed();



    // ORTHOGRAPHIC 화면모드와의 분기
    if (ViewportCamera.GetProjection().GetProjectionType() == EProjectionType::Orthographic)
    {
        CameraController.UpdateMouseInput_ORTHOGRAPHIC(ViewportCamera);
    }
    else
    {
        CameraController.UpdateMouseInput(ViewportCamera);
    }


    // 우클릭 중에는 WASD 가 카메라 비행에 쓰이므로 단축키와 겹치지 않게 나눈다.
    if (FInputManager::Get().IsMousePressed(EMouseButton::Right))
    {
        CameraController.UpdateKeyInput(ViewportCamera, DeltaTime);
        return;
    }
    else
    {
        CameraController.ResetVelocity();
    }


    // 기즈모를 드래그하는 중에는 모드가 바뀌면 안 된다.
    if (!Editor->GetGizmo().IsInteracting())
    {
        UpdateShortcuts();
    }
}

void FEditorViewportClient::UpdateShortcuts() const
{
    FInputManager& Input = FInputManager::Get();
    FGizmo& Gizmo = Editor->GetGizmo();

    // 백틱(`) : 월드/로컬 공간 전환.
    // Translate/Rotate 에서만 의미가 있어 None/Scale 은 제외한다.
    if (Input.IsKeyDown(VK_OEM_3))
    {
        if (Gizmo.Mode != EGizmoMode::None && Gizmo.Mode != EGizmoMode::Scale)
        {
            Gizmo.SetGizmoSpace(
                static_cast<EGizmoSpace>((static_cast<uint8>(Gizmo.GetSpace()) + 1) % 2));
        }
    }

    if (Input.IsKeyDown('Q'))
    {
        Gizmo.Mode = EGizmoMode::None;
    }
    else if (Input.IsKeyDown('W'))
    {
        Gizmo.Mode = EGizmoMode::Translate;
    }
    else if (Input.IsKeyDown('E'))
    {
        Gizmo.Mode = EGizmoMode::Rotate;
    }
    else if (Input.IsKeyDown('R'))
    {
        Gizmo.Mode = EGizmoMode::Scale;
    }
    else if (Input.IsKeyDown(VK_SPACE))
    {
        Gizmo.Mode = static_cast<EGizmoMode>((static_cast<uint8>(Gizmo.Mode) + 1) % 4);
    }

}

void FEditorViewportClient::UpdateGizmoHover(const FVector2& LocalMousePixels, const FVector2& ViewportSizePixels)
{
    FRay Ray = FRayCastingManager::CreateRayFromScreenPosition(
       ViewportCamera, LocalMousePixels, ViewportSizePixels);

    FGizmo& Gizmo = Editor->GetGizmo();
    Gizmo.HoveredHandle = Gizmo.HitTest(Editor->SelectedTransform, Ray, ViewportCamera);
}


void FEditorViewportClient::HandlePicking(const FVector2& LocalMousePixels, const FVector2& ViewportSizePixels)
{
    // 기즈모 핸들 위를 눌렀으면 피킹 대신 조작을 시작한다.
    // 선택된 액터가 이 뷰포트가 보는 월드의 것일 때만 기즈모 핸들을 잡을 수 있다.
    if (Editor->IsSelectionInWorld(GetWorld()))
    {
        // Process에서 현재 뷰포트의 Hover 판정을 먼저 갱신한 상태다.
        FGizmo& Gizmo = Editor->GetGizmo();

        if (Gizmo.HoveredHandle != EGizmoHandle::None)
        {
            Gizmo.BeginInteraction(
                Editor->SelectedTransform,
                Gizmo.HoveredHandle,
                LocalMousePixels,
                ViewportCamera,
                ViewportSizePixels);

            return;
        }
    }

    UPrimitiveComponent* HitComponent = nullptr;
    FVector ImpactPoint;
    bool bHit = false;

    // 클릭한 뷰포트가 보는 월드에서 피킹한다 (PIE 중에는 PIE 월드)
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG("[Picking] No world available for picking.");
        return;
    }

    FScene* PickScene = World->GetScene();

    // 1) 마우스 화면 좌표 획득
    // 2) 화면 좌표 -> 월드 좌표로의 픽 레이(Pick Ray) 계산
    const FRay PickRay = FRayCastingManager::CreateRayFromScreenPosition(
        ViewportCamera, LocalMousePixels, ViewportSizePixels);

    // 벤치마크용 광선 저장 (측정 구간 밖)
    FRayCastingManager::LastPickRay = PickRay;
    FRayCastingManager::bHasLastPickRay = true;

    // 3) 퍼포먼스 측정용 카운터 시작
    FScopeCycleCounter PickCounter;

    // 4) 전체 Picking 횟수 누적
    ++Editor->PickingAttempts;

    // 5) 모든 오브젝트(프리미티브)에 대해 충돌 판정
    if (Editor->bUseBVHPicking && PickScene)
    {
        bHit = PickScene->GetSceneBVH().QueryRay(PickRay, HitComponent, ImpactPoint);
    }
    else
    {
        const TArray<UPrimitiveComponent*>& Components = Editor->GetPrimitiveComponents();
        bHit = FRayCastingManager::RayIntersectsMeshes(
            PickRay, ViewportCamera, Components, HitComponent, ImpactPoint);
    }

    // 6) 퍼포먼스 측정 종료 및 시간 누적
    Editor->LastPickingMs = PickCounter.Finish();
    Editor->AccumulatedPickingMs += Editor->LastPickingMs;

    // 필요 시 'isHit' 결과를 활용해 추가 로직 처리
    // 피킹은 액터 단위로 선택한다. 소유 액터가 없으면 선택할 수 없다.
    if (!bHit || !HitComponent || !HitComponent->GetActorOwner())
    {
        Editor->UnSelectActor();
        return;
    }

    AActor* OwnerActor = HitComponent->GetActorOwner();
    Editor->SelectActor(OwnerActor);

    const char* ActorClass =
        OwnerActor->GetClass() ? OwnerActor->GetClass()->GetDisplayName().c_str() : "Unknown";
    const char* CompClass =
        HitComponent->GetClass() ? HitComponent->GetClass()->GetDisplayName().c_str() : "Unknown";

    UE_LOG("[Picking] Actor: %s (Name: %s), Component: %s (Index: %u)", ActorClass,
        OwnerActor->GetName().ToString().c_str(), CompClass, HitComponent->GetInternalIndex());
}