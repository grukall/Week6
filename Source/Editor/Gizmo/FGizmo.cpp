#include "FGizmo.h"

#include "Runtime/Core/IntTypes.h"
#include "Runtime/CoreUObject/USceneComponent.h"
#include "Runtime/Engine/FCamera.h"
#include "Runtime/Rendering/FRenderer.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Math/FMatrix.h"
#include "Runtime/Math/FVector4.h"
#include "Editor/Core/FEditor.h"
#include <numbers>

#include "Runtime/Engine/FRayCastingManager.h"

void FGizmo::Initialize()
{
	auto& RenderResources = FRenderResourceLibrary::Get();
	ArrowMesh = RenderResources.GetMesh(FName("#Arrow"));
	CircleMesh = RenderResources.GetMesh(FName("#Circle"));
	SquareArrowMesh = RenderResources.GetMesh(FName("#SquareArrow"));

	Material = RenderResources.GetMaterial(FName("Gizmo"));
}

void FGizmo::Draw(FRenderer& Renderer, const FTransform& Transform, const FCamera& Camera) const
{
	static FMatrix YAxisRotation = FMatrix::MakeRotationZ(std::numbers::pi_v<float> * 0.5f);
	static FMatrix ZAxisRotation = FMatrix::MakeRotationY(std::numbers::pi_v<float> * 0.5f);

	float GizmoScale = CalculateGizmoScale(Transform.GetLocation(), Camera);
	FMatrix Scale = FMatrix::MakeScale(FVector{ GizmoScale, GizmoScale, GizmoScale });
	FMatrix ObjectRotation = GetSpace() == EGizmoSpace::World ? FMatrix::GetIdentity() : Transform.GetRotation().ToMatrixRow();
	FMatrix Translation = FMatrix::MakeTranslation(Transform.GetLocation());
	DrawAxis(Renderer, EGizmoHandle::XAxis, Scale * ObjectRotation * Translation);
	DrawAxis(Renderer, EGizmoHandle::YAxis, Scale * YAxisRotation * ObjectRotation * Translation);
	DrawAxis(Renderer, EGizmoHandle::ZAxis, Scale * ZAxisRotation * ObjectRotation * Translation);
}

EGizmoHandle FGizmo::HitTest(const FTransform& Transform, const FRay& Ray, const FCamera& Camera) const
{
	float GizmoScale = CalculateGizmoScale(Transform.GetLocation(), Camera);

	static FMatrix YAxisRotation = FMatrix::MakeRotationZ(std::numbers::pi_v<float> * 0.5f);
	static FMatrix ZAxisRotation = FMatrix::MakeRotationY(std::numbers::pi_v<float> * 0.5f);

	FMatrix Scale = FMatrix::MakeScale(FVector{ GizmoScale, GizmoScale, GizmoScale });
	FMatrix ObjectRotation = GetSpace() == EGizmoSpace::World ? FMatrix::GetIdentity() : Transform.GetRotation().ToMatrixRow();
	FMatrix Translation = FMatrix::MakeTranslation(Transform.GetLocation());

	TSharedPtr<FMesh> GizmoMesh;
	switch (Mode)
	{
	case EGizmoMode::Translate:
		GizmoMesh = ArrowMesh;
		break;

	case EGizmoMode::Rotate:
		GizmoMesh = CircleMesh;
		break;

	case EGizmoMode::Scale:
		GizmoMesh = SquareArrowMesh;
		break;

	case EGizmoMode::None:
		return EGizmoHandle::None;
	}

	float ClosestDistance = (std::numeric_limits<float>::max)();
	EGizmoHandle ClosestHandle = EGizmoHandle::None;

	// RayIntersectsMesh는 이 값보다 가까운 교차만 인정하고 갱신하므로 반드시 최대값으로 시작한다.
	float HitDistance, Dummyfloat = (std::numeric_limits<float>::max)();
	FVector ImpactPoint;
	if (FRayCastingManager::RayIntersectsMesh(
			Ray,
			*GizmoMesh,
			Scale * ObjectRotation * Translation,
			HitDistance,
			ImpactPoint, Dummyfloat) &&
		HitDistance < ClosestDistance)
	{
		ClosestDistance = HitDistance;
		ClosestHandle = EGizmoHandle::XAxis;
	}
	if (FRayCastingManager::RayIntersectsMesh(
		Ray,
		*GizmoMesh,
		Scale * YAxisRotation * ObjectRotation * Translation,
		HitDistance,
		ImpactPoint, Dummyfloat) &&
		HitDistance < ClosestDistance)
	{
		ClosestDistance = HitDistance;
		ClosestHandle = EGizmoHandle::YAxis;
	}
	if (FRayCastingManager::RayIntersectsMesh(
		Ray,
		*GizmoMesh,
		Scale * ZAxisRotation * ObjectRotation * Translation,
		HitDistance,
		ImpactPoint, Dummyfloat) &&
		HitDistance < ClosestDistance)
	{
		ClosestDistance = HitDistance;
		ClosestHandle = EGizmoHandle::ZAxis;
	}

	return ClosestHandle;
}

void FGizmo::BeginInteraction(const FTransform& Transform, EGizmoHandle Handle, const FVector2& MousePosition, const FCamera& Camera, const FVector2& ViewportSize)
{
	switch (Handle)
	{
	case EGizmoHandle::XAxis:
		InteractionAxisLocal = FVector{ 1.0f, 0.0f, 0.0f };
		break;
	case EGizmoHandle::YAxis:
		InteractionAxisLocal = FVector{ 0.0f, 1.0f, 0.0f };
		break;
	case EGizmoHandle::ZAxis:
		InteractionAxisLocal = FVector{ 0.0f, 0.0f, 1.0f };
		break;
	case EGizmoHandle::None:
		return;
	}
	InteractionLastMouse = MousePosition;

	if (UpdateInteractionAxis(Transform, Camera, ViewportSize))
	{
		ActiveHandle = Handle;
	}
}

bool FGizmo::UpdateInteractionAxis(const FTransform& Transform, const FCamera& Camera, const FVector2& ViewportSize)
{
	// Local이면 현재 회전 기준 축을 쓴다. 다른 컴포넌트가 회전시켜도 축이 따라 돈다.
	InteractionAxisWorld = GetSpace() == EGizmoSpace::World ? InteractionAxisLocal : Transform.GetRotation().RotateVector(InteractionAxisLocal);

	float GizmoScale = CalculateGizmoScale(Transform.GetLocation(), Camera);

	FVector OriginWorld = Transform.GetLocation();
	FVector AxisEndWorld = OriginWorld + InteractionAxisWorld * GizmoScale;

	FVector2 OriginViewport = WorldToViewport(OriginWorld, Camera, ViewportSize);
	FVector2 AxisEndViewport = WorldToViewport(AxisEndWorld, Camera, ViewportSize);

	FVector2 AxisViewport = AxisEndViewport - OriginViewport;
	float AxisViewportLength = AxisViewport.Size();

	InteractionOriginViewport = OriginViewport;

	FVector CenterToCamera = Camera.GetPosition() - OriginWorld;
	InteractionRotationSign = (CenterToCamera.Dot(InteractionAxisWorld) <= 0.0f) ? 1.0f : -1.0f;

	if (AxisViewportLength <= 1e-5f)
	{
		return false;
	}

	InteractionAxisViewport = AxisViewport / AxisViewportLength;
	InteractionWorldUnitsPerPixel = GizmoScale / AxisViewportLength;
	return true;
}

void FGizmo::UpdateInteraction(FEditor& Editor, const FVector2& MousePosition, const FCamera& Camera, const FVector2& ViewportSize)
{
	if (!Editor.ObjectSelected() || ActiveHandle == EGizmoHandle::None)
	{
		return;
	}

	// SelectedTransform은 FEditor::Process에서 매 프레임 실제 Transform과 동기화된다.
	// 시작 시점이 아니라 현재 Transform 기준으로 축을 다시 구하고, 지난 프레임 이후 마우스 이동량만큼만 적용한다.
	const FTransform& OldSelectedTransform = Editor.SelectedTransform;
	if (!UpdateInteractionAxis(OldSelectedTransform, Camera, ViewportSize))
	{
		InteractionLastMouse = MousePosition;
		return;
	}

	FVector2 MouseDelta = MousePosition - InteractionLastMouse;
	float ViewportDistance = MouseDelta.Dot(InteractionAxisViewport);
	float WorldDistance = ViewportDistance * InteractionWorldUnitsPerPixel;
	FTransform NewSelectedTransform = OldSelectedTransform;

	switch (Mode)
	{
	case EGizmoMode::Translate:
		NewSelectedTransform.SetLocation(OldSelectedTransform.GetLocation() + InteractionAxisWorld * WorldDistance);
		break;

	case EGizmoMode::Rotate:
	{
		FVector2 BA = InteractionLastMouse - InteractionOriginViewport;
		FVector2 BC = MousePosition - InteractionOriginViewport;
		float Theta = (std::atan2f(BA.Y, BA.X) - std::atan2f(BC.Y, BC.X)) * InteractionRotationSign * 180.0f / std::numbers::pi_v<float>;
		if (GetSpace() == EGizmoSpace::World)
		{
			FQuaternion Delta = FQuaternion::FromAxisAngle(InteractionAxisWorld, Theta);
			NewSelectedTransform.SetRotation(Delta * OldSelectedTransform.GetRotation());
		}
		else
		{
			FQuaternion Delta = FQuaternion::FromAxisAngle(InteractionAxisLocal, Theta);
			NewSelectedTransform.SetRotation(OldSelectedTransform.GetRotation() * Delta);
		}
		Editor.SelectedEulerDegDisplay = NewSelectedTransform.GetRotation().GetEulerXYZ() * 180.0f / std::numbers::pi_v<float>;
		break;
	}

	case EGizmoMode::Scale:
		NewSelectedTransform.SetScale3D(OldSelectedTransform.GetScale3D() + InteractionAxisLocal * WorldDistance);
		break;

	case EGizmoMode::None:
		return;
	}

	// 기즈모 갱신 후 World Tick이 한 번 더 돌기 때문에, 변화량만 넘기고 FEditor::Process에서 실제 Transform에 적용한다.
	// World면 월드 기준 변화량, Local이면 Old 기준 로컬 변화량이다. 스케일은 항상 로컬 축 기준 차이다.
	if (GetSpace() == EGizmoSpace::World)
	{
		Editor.GapTransform.SetLocation(NewSelectedTransform.GetLocation() - OldSelectedTransform.GetLocation());
		Editor.GapTransform.SetRotation((NewSelectedTransform.GetRotation() * OldSelectedTransform.GetRotation().Conjugate()).Normalized());
	}
	else
	{
		const FQuaternion InverseOldRotation = OldSelectedTransform.GetRotation().Conjugate();
		Editor.GapTransform.SetLocation(InverseOldRotation.RotateVector(NewSelectedTransform.GetLocation() - OldSelectedTransform.GetLocation()));
		Editor.GapTransform.SetRotation((InverseOldRotation * NewSelectedTransform.GetRotation()).Normalized());
	}
	Editor.GapTransform.SetScale3D(NewSelectedTransform.GetScale3D() - OldSelectedTransform.GetScale3D());
	Editor.SelectedTransform = NewSelectedTransform;
	InteractionLastMouse = MousePosition;
}

void FGizmo::EndInteraction()
{
	ActiveHandle = EGizmoHandle::None;
}

void FGizmo::DrawAxis(FRenderer& Renderer, EGizmoHandle Handle, const FMatrix& World) const
{
	constexpr FVector4 Color[3] = {
		{ 0.8f, 0.0f, 0.0f, 1.0f },
		{ 0.0f, 0.8f, 0.0f, 1.0f },
		{ 0.0f, 0.0f, 0.8f, 1.0f },
	};

	constexpr FVector4 ActiveColor { 1.0f, 1.0f, 0.1f, 1.0f };
	constexpr FVector4 HoverColor { 0.7f, 0.7f, 0.0f, 1.0f };

	TSharedPtr<FMesh> GizmoMesh;
	TSharedPtr<FMaterial> GizmoMaterial;
	switch (Mode)
	{
	case EGizmoMode::Translate:
		GizmoMesh = ArrowMesh;
		GizmoMaterial = Material;
		break;
	case EGizmoMode::Rotate:
		GizmoMesh = CircleMesh;
		GizmoMaterial = Material;
		break;
	case EGizmoMode::Scale:
		GizmoMesh = SquareArrowMesh;
		GizmoMaterial = Material;
		break;
	case EGizmoMode::None:
		return;
	}

	FVector DrawColor = Color[static_cast<uint8>(Handle) - 1];
	if (ActiveHandle == Handle)
	{
		DrawColor = ActiveColor;
	}
	else if (HoveredHandle == Handle && ActiveHandle == EGizmoHandle::None)
	{
		DrawColor = HoverColor;
	}

	FObjectConstants Constants{};
	Constants.World = World;
	Constants.Color = DrawColor;
	Constants.DisableShading = 1.0f;
	Renderer.Draw(*GizmoMesh, *GizmoMaterial, Constants);
}

float FGizmo::CalculateGizmoScale(const FVector& GizmoLocation, const FCamera& Camera) const
{
	constexpr float ScalePerDistance = 0.15f;

	// 직교투영은 거리가 화면상 크기에 영향을 주지 않는다.
	// 거리를 곱하면 멀어질수록 기즈모가 커지므로, 뷰 높이를 기준으로 삼는다.
	if (Camera.GetProjection().GetProjectionType() == EProjectionType::Orthographic)
	{

		constexpr float ScalePerViewHeight = 0.15f;
		return Camera.GetProjection().GetOrthographicHeight() * ScalePerViewHeight;
	}

	FVector ToTarget = GizmoLocation - Camera.GetPosition();
	return ToTarget.Size() * ScalePerDistance;
}

FVector2 FGizmo::WorldToViewport(const FVector& WorldPosition, const FCamera& Camera,
	const FVector2& ViewportSize) const
{
	FMatrix VP = Camera.GetViewProjectionMatrix();

	FVector Projected = VP.TransformPointRow(WorldPosition);

	return FVector2{
		(Projected.Y + 1.0f) * 0.5f * ViewportSize.X,
		(1.0f - Projected.Z) * 0.5f * ViewportSize.Y
	};
}

