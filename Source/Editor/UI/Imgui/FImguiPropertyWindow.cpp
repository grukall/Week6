#include "FImguiPropertyWindow.h"
#include "Runtime/CoreUObject/USceneComponent.h"
#include "Runtime/CoreUObject/UPrimitiveComponent.h"
#include "Runtime/CoreUObject/UDirectionLightComponent.h"
#include "Runtime/CoreUObject/UPointLightComponent.h"
#include "Runtime/CoreUObject/USpotLightComponent.h"
#include "Runtime/CoreUObject/UProjectileMovementComponent.h"
#include "Runtime/CoreUObject/URotationMovementComponent.h"
#include "Runtime/CoreUObject/UTextInstanceComponent.h"
#include "Runtime/CoreUObject/UBillBoardComp.h"
#include "Runtime/CoreUObject/UAnimatedBillboardComp.h"
#include "Runtime/CoreUObject/Mesh/UStaticMeshComponent.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/Actors/AActor.h"
#include "ThirdParty/Imgui/imgui.h"
#include "ThirdParty/Imgui/imgui_internal.h"
#include "ThirdParty/Imgui/imgui_impl_dx11.h"
#include "ThirdParty/Imgui/imgui_impl_win32.h"
#include "ThirdParty/Imgui/imgui_stdlib.h"
#include <string>
#include <algorithm>
#include "FImguiDragDrop.h"
#include "Editor/Core/EditorConstant.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Rendering/FMaterial.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Asset/FAssetRegistry.h"
#include "Runtime/Asset/UFont.h"


namespace
{
	constexpr float SlotSize = 64.0f;

	TArray<USceneComponent*> GetTreeChildren(const AActor& Actor, USceneComponent& Comp)
	{
		TArray<USceneComponent*> TreeChildren;
		for (USceneComponent* Child : Comp.GetChildren())
		{
			if (Child && Child->GetSceneOwner() == &Comp && Child->GetActorOwner() == &Actor)
			{
				TreeChildren.push_back(Child);
			}
		}

		if (&Comp == Actor.GetRootComponent())
		{
			for (USceneComponent* Other : Actor.GetAttachedComponents())
			{
				if (Other && Other != &Comp && !Other->GetSceneOwner())
				{
					TreeChildren.push_back(Other);
				}
			}
		}
		return TreeChildren;
	}
}


void FImguiPropertyWindow::Process(FEditor& Editor)
{
	if (Editor.bHideUI || Editor.bZenMode)
	{
		return;
	}

	ImGui::Begin("Jungle Property Window");

	if (AActor* SelectedActor = Editor.GetSelectedActor())
	{
		ShowActorHeader(*SelectedActor);
		ImGui::Separator();

		ShowComponentHierarchy(Editor, *SelectedActor);
		ImGui::Separator();

		ShowComponentSections(Editor, *SelectedActor);
		ShowActorComponentSections(*SelectedActor);
	}
	else
	{
		ImGui::TextDisabled("No selection");
	}

	ShowGizmoSettings(Editor);

	ImGui::End();
}

void FImguiPropertyWindow::ShowActorHeader(const AActor& Actor) const
{
	const char* ActorClassName = Actor.GetClass() ? Actor.GetClass()->GetDisplayName().c_str() : "None";
	ImGui::Text("Actor Class: %s", ActorClassName);
	ImGui::Text("Actor Name: %s", Actor.GetName().ToString().c_str());
	ImGui::Text("Actor Guid: %s", Actor.GetGuid().ToString().c_str());
}

void FImguiPropertyWindow::ShowComponentHierarchy(FEditor& Editor, AActor& Actor)
{
	ImGui::TextDisabled("Components Hierarchy");

	PendingDragged = nullptr;
	PendingTarget = nullptr;

	if (USceneComponent* RootComp = Actor.GetRootComponent())
	{
		ShowComponentTreeNode(Editor, Actor, *RootComp);
	}
	else
	{
		ImGui::TextDisabled("No RootComponent");
	}

	ShowActorComponentList(Editor, Actor);

	if (PendingDragged && PendingTarget
		&& PendingDragged != Actor.GetRootComponent()
		&& PendingDragged->GetActorOwner() == &Actor
		&& PendingTarget->GetActorOwner() == &Actor
		&& PendingDragged->GetSceneOwner() != PendingTarget)
	{
		PendingDragged->SetupAttachment(PendingTarget, true);
		PendingDragged->MarkActorTransformDirty();
		Editor.RefreshSelectedTransform();
		++Editor.HierarchyVersion;
	}

	ShowComponentButtons(Editor, Actor);
}

void FImguiPropertyWindow::ShowComponentTreeNode(FEditor& Editor, AActor& Actor, USceneComponent& Comp)
{
	const bool bIsRoot = (&Comp == Actor.GetRootComponent());
	const TArray<USceneComponent*> TreeChildren = GetTreeChildren(Actor, Comp);

	ImGuiTreeNodeFlags NodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;
	if (&Comp == Editor.GetSelectedComponent())
	{
		NodeFlags |= ImGuiTreeNodeFlags_Selected;
	}
	if (TreeChildren.empty())
	{
		NodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}

	const FString CompName = Comp.GetName().ToString();
	const char* CompClassName = Comp.GetClass() ? Comp.GetClass()->GetDisplayName().c_str() : "Component";
	const bool bNodeOpen = ImGui::TreeNodeEx(&Comp, NodeFlags, "%s%s (%s)", bIsRoot ? "[Root] " : "", CompName.c_str(), CompClassName);

	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
	{
		Editor.SelectComponent(&Comp);
	}

	if (!bIsRoot && ImGui::BeginDragDropSource())
	{
		USceneComponent* Dragged = &Comp;
		ImGui::SetDragDropPayload(ComponentDragPayloadType, &Dragged, sizeof(Dragged));
		ImGui::TextUnformatted(CompName.c_str());
		ImGui::EndDragDropSource();
	}

	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(ComponentDragPayloadType))
		{
			PendingDragged = *static_cast<USceneComponent* const*>(Payload->Data);
			PendingTarget = &Comp;
		}
		ImGui::EndDragDropTarget();
	}

	if (bNodeOpen && !TreeChildren.empty())
	{
		for (USceneComponent* Child : TreeChildren)
		{
			ShowComponentTreeNode(Editor, Actor, *Child);
		}

		ImGui::TreePop();
	}
}

void FImguiPropertyWindow::ShowActorComponentList(FEditor& Editor, AActor& Actor)
{
	for (UActorComponent* Comp : Actor.GetOwnedComponents())
	{
		if (!Comp || Comp->IsA<USceneComponent>())
		{
			continue;
		}

		ImGuiTreeNodeFlags NodeFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (Comp == Editor.GetSelectedComponent())
		{
			NodeFlags |= ImGuiTreeNodeFlags_Selected;
		}

		const FString CompName = Comp->GetName().ToString();
		const char* CompClassName = Comp->GetClass() ? Comp->GetClass()->GetDisplayName().c_str() : "Component";
		ImGui::TreeNodeEx(Comp, NodeFlags, "%s (%s)", CompName.c_str(), CompClassName);

		if (ImGui::IsItemClicked())
		{
			Editor.SelectComponent(Comp);
		}
	}
}

void FImguiPropertyWindow::ShowComponentButtons(FEditor& Editor, AActor& Actor)
{
	if (ImGui::Button("Add Component"))
	{
		ImGui::OpenPopup("AddComponentPopup");
	}

	UActorComponent* SelectedComponent = Editor.GetSelectedComponent();
	const bool bCanDelete = SelectedComponent
		&& SelectedComponent->GetActorOwner() == &Actor
		&& SelectedComponent != Actor.GetRootComponent();

	ImGui::SameLine();
	ImGui::BeginDisabled(!bCanDelete);
	const bool bDeleteClicked = ImGui::Button("Delete Component");
	ImGui::EndDisabled();

	if (ImGui::BeginPopup("AddComponentPopup"))
	{
		for (UClass* Item : EditorConstant::SpawnableComponents)
		{
			if (!ImGui::Selectable(Item->GetUClassName().c_str()))
			{
				continue;
			}

			UObject* Object = NewObject(Item);
			UActorComponent* NewComponent = Object ? Object->Cast<UActorComponent>() : nullptr;
			if (NewComponent)
			{
				Actor.AddComponent(NewComponent);
				++Editor.HierarchyVersion;
			}
			else if (Object)
			{
				DestroyObject(Object);
			}
		}
		ImGui::EndPopup();
	}

	if (bDeleteClicked && bCanDelete)
	{
		// 선택을 먼저 풀어야 삭제된 컴포넌트의 트랜스폼이 액터 루트에 적용되지 않는다.
		Editor.UnSelectActor();
		Actor.DeleteComponent(SelectedComponent);
		Editor.SelectActor(&Actor);
		++Editor.HierarchyVersion;
	}
}

void FImguiPropertyWindow::ShowComponentSections(FEditor& Editor, AActor& Actor)
{
	USceneComponent* RootComp = Actor.GetRootComponent();

	for (USceneComponent* Comp : Actor.GetAttachedComponents())
	{
		if (!Comp)
		{
			continue;
		}

		const bool bIsRoot = (Comp == RootComp);
		const char* CompTypeName = Comp->GetClass() ? Comp->GetClass()->GetDisplayName().c_str() : "Component";

		// ### 뒤쪽이 실제 ID 라서, 앞의 표시 이름이 바뀌어도 접힘 상태가 유지된다.
		std::string SectionTitle = (bIsRoot ? "[Root] " : "[Sub] ") + std::string(CompTypeName)
			+ " (Index: " + std::to_string(Comp->GetInternalIndex()) + ")###CompHeader_" + std::to_string(Comp->GetInternalIndex());

		if (!ImGui::CollapsingHeader(SectionTitle.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			continue;
		}

		// 컴포넌트마다 위젯 ID 를 분리해야 같은 라벨끼리 충돌하지 않는다.
		ImGui::PushID(Comp);
		ShowComponentDetails(Editor, Actor, *Comp, bIsRoot);
		ImGui::PopID();

		ImGui::Spacing();
	}
}

void FImguiPropertyWindow::ShowActorComponentSections(AActor& Actor)
{
	for (UActorComponent* Comp : Actor.GetOwnedComponents())
	{
		if (!Comp || Comp->IsA<USceneComponent>())
		{
			continue;
		}

		const char* CompTypeName = Comp->GetClass() ? Comp->GetClass()->GetDisplayName().c_str() : "Component";
		std::string SectionTitle = "[Comp] " + std::string(CompTypeName)
			+ " (Index: " + std::to_string(Comp->GetInternalIndex()) + ")###CompHeader_" + std::to_string(Comp->GetInternalIndex());

		if (!ImGui::CollapsingHeader(SectionTitle.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			continue;
		}

		ImGui::Text("Name: %s", Comp->GetName().ToString().c_str());
		ImGui::TextDisabled("No editable properties");
		ImGui::Spacing();
	}
}

void FImguiPropertyWindow::ShowComponentDetails(FEditor& Editor, AActor& Actor,
	USceneComponent& Comp, bool bIsRoot)
{
	ShowTransform(Editor, Comp, bIsRoot);

	if (Comp.IsA<UTextInstanceComponent>())
	{
		ShowTextSettings(static_cast<UTextInstanceComponent&>(Comp));
	}
	else if (Comp.IsA<UAnimatedBillboardComp>())
	{
		auto& BillboardComp = static_cast<UAnimatedBillboardComp&>(Comp);
		ShowBillboardSettings(BillboardComp);
		ShowAnimatedBillboardSettings(BillboardComp);
	}
	else if (Comp.IsA<UBillBoardComp>())
	{
		ShowBillboardSettings(static_cast<UBillBoardComp&>(Comp));
	}
	else if (Comp.IsA<UDirectionLightComponent>())
	{
		ShowDirectionLightSettings(static_cast<UDirectionLightComponent&>(Comp));
	}
	else if (Comp.IsA<UPointLightComponent>())
	{
		ShowPointLightSettings(static_cast<UPointLightComponent&>(Comp));
	}
	else if (Comp.IsA<USpotLightComponent>())
	{
		ShowSpotLightSettings(static_cast<USpotLightComponent&>(Comp));
	}

	else if (Comp.IsA<UProjectileMovementComponent>())
	{
		//ShowProjectileMovementSettings(static_cast<UProjectileMovementComponent&>(Comp));
	}

	else if (Comp.IsA<URotationMovementComponent>())
	{
		//ShowRotationMovementSettings(static_cast<URotationMovementComponent&>(Comp));
	}

	else if (Comp.IsA<UStaticMeshComponent>())
	{
		ShowStaticMeshSettings(Actor, static_cast<UStaticMeshComponent&>(Comp), bIsRoot);
	}

	
}

void FImguiPropertyWindow::ShowTransform(FEditor& Editor, USceneComponent& Comp, bool bIsRoot) const
{
	ImGui::TextDisabled("Transform");

	const bool bHasParent = !bIsRoot || Comp.GetSceneOwner();
	const char* LocationLabel = bHasParent ? "Rel Location###Location" : "Location###Location";
	const char* RotationLabel = bHasParent ? "Rel Rotation (deg)###Rotation" : "Rotation (deg)###Rotation";
	const char* ScaleLabel = bHasParent ? "Rel Scale###Scale" : "Scale###Scale";

	// 서브 컴포넌트 상대 트랜스폼 편집
	FTransform RelTransform = Comp.GetRelativeTransform();
	FVector RelLocation = RelTransform.GetLocation();
	bool bTransformChanged = false;
	if (ImGui::DragFloat3(LocationLabel, &RelLocation.X, 0.01f))
	{
		bTransformChanged = true;
		RelTransform.SetLocation(RelLocation);
	}

	FVector RelEuler = RelTransform.GetRotation().ToEulerXYZDeg();
	if (ImGui::DragFloat3(RotationLabel, &RelEuler.X, 0.5f))
	{
		bTransformChanged = true;
		RelTransform.SetRotation(FQuaternion::FromEulerXYZDeg(RelEuler));
	}
	FVector RelScale = RelTransform.GetScale3D();
	if (ImGui::DragFloat3(ScaleLabel, &RelScale.X, 0.01f))
	{
		bTransformChanged = true;
		RelTransform.SetScale3D(RelScale);
	}

	if (!bTransformChanged)
	{
		return;
	}

	Comp.SetRelativeTransform(RelTransform);
	Editor.RefreshSelectedTransform();
}

void FImguiPropertyWindow::ShowTextSettings(UTextInstanceComponent& TextComp) const
{
	ImGui::Separator();
	ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Text Settings");


	ImGui::TextDisabled("Font");
	const UFont* Font = TextComp.GetFont();
	const FString FontLabel = Font ? Font->GetID().ToString() : "No Font";
	const float FullWidth = ImGui::GetContentRegionAvail().x;
	ImGui::Button(FontLabel.c_str(), ImVec2(FullWidth, SlotSize));

	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(ContentDragPayloadType))
		{
			const auto* Dropped = static_cast<const FContentDragPayload*>(Payload->Data);
			if (Dropped && Dropped->Ptr)
			{
				if (UFont* NewFont = Dropped->Ptr->Cast<UFont>())
				{
					TextComp.SetFont(NewFont);
				}
			}
		}
		ImGui::EndDragDropTarget();
	}

	static char utfBuffer[512]{};
	WideCharToMultiByte(CP_UTF8, 0, TextComp.GetText().c_str(), -1, &utfBuffer[0], sizeof(utfBuffer), NULL, NULL);

	if (ImGui::InputText("Text Content", &utfBuffer[0], sizeof(utfBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
	{
		FString Buffer{ &utfBuffer[0] };
		uint32 convertResult = MultiByteToWideChar(CP_UTF8, 0, Buffer.c_str(), Buffer.length(), NULL, 0);
		FWString newText(convertResult, 0);
		MultiByteToWideChar(CP_UTF8, 0, Buffer.c_str(), Buffer.length(), newText.data(), convertResult);
		TextComp.SetText(newText);
	}

	ImGui::TextDisabled("Text Bounds");
	ImGui::Text("Width: %.2f", TextComp.GetWidth());
	ImGui::Text("Height: %.2f", TextComp.GetHeight());
}

void FImguiPropertyWindow::ShowBillboardSettings(UBillBoardComp& BillboardComp) const
{
	ImGui::Separator();
	ImGui::TextColored(ImVec4(0.8f, 0.6f, 1.0f, 1.0f), "Billboard Settings");

	UTexture* TextureAsset = BillboardComp.GetTexture();
	FTexture* Texture = TextureAsset ? TextureAsset->Get() : nullptr;
	const float FullWidth = ImGui::GetContentRegionAvail().x;

	ImGui::TextDisabled("Texture");
	if (Texture && Texture->GetSRV())
	{
		ImGui::Image(reinterpret_cast<ImTextureID>(Texture->GetSRV()), ImVec2(FullWidth, SlotSize));
	}
	else
	{
		ImGui::Button("No\nTexture", ImVec2(FullWidth, SlotSize));
	}

	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(ContentDragPayloadType))
		{
			const auto* Dropped = static_cast<const FContentDragPayload*>(Payload->Data);
			if (Dropped && Dropped->Ptr)
			{
				if (UTexture* NewTexture = Dropped->Ptr->Cast<UTexture>())
				{
					BillboardComp.SetTexture(NewTexture);
				}
			}
		}
		ImGui::EndDragDropTarget();
	}

	FVector2 UVScale = BillboardComp.GetUVScale();
	if (ImGui::DragFloat2("UV Scale", &UVScale.X, 0.01f))
	{
		BillboardComp.SetUVScale(UVScale);
	}

	FVector2 UVOffset = BillboardComp.GetUVOffset();
	if (ImGui::DragFloat2("UV Offset", &UVOffset.X, 0.01f))
	{
		BillboardComp.SetUVOffset(UVOffset);
	}
}

void FImguiPropertyWindow::ShowAnimatedBillboardSettings(UAnimatedBillboardComp& BillboardComp) const
{
	ImGui::Separator();
	ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.25f, 1.0f), "Animation Settings");

	int GridX = BillboardComp.GetGridX();
	int GridY = BillboardComp.GetGridY();
	int TotalFrames = BillboardComp.GetTotalFrames();
	bool bSpriteSheetChanged = ImGui::DragInt("Grid X", &GridX, 1.0f, 1, 256);
	bSpriteSheetChanged |= ImGui::DragInt("Grid Y", &GridY, 1.0f, 1, 256);
	bSpriteSheetChanged |= ImGui::DragInt("Total Frames", &TotalFrames, 1.0f, 1, 65536);
	if (bSpriteSheetChanged)
	{
		GridX = std::max(GridX, 1);
		GridY = std::max(GridY, 1);
		TotalFrames = std::clamp(TotalFrames, 1, GridX * GridY);
		BillboardComp.SetSpriteSheet(GridX, GridY, BillboardComp.GetFrameRate(), TotalFrames);
	}

	float FrameRate = BillboardComp.GetFrameRate();
	if (ImGui::DragFloat("Frame Rate", &FrameRate, 0.1f, 0.1f, 240.0f))
	{
		BillboardComp.SetFrameRate(std::max(FrameRate, 0.1f));
	}

	int CurrentFrame = BillboardComp.GetCurrentFrame();
	if (ImGui::SliderInt("Current Frame", &CurrentFrame, 0, std::max(0, BillboardComp.GetTotalFrames() - 1)))
	{
		BillboardComp.SetCurrentFrame(CurrentFrame);
	}

	bool bLoop = BillboardComp.IsLooping();
	if (ImGui::Checkbox("Loop", &bLoop))
	{
		BillboardComp.SetLooping(bLoop);
	}

	if (BillboardComp.IsPlaying())
	{
		if (ImGui::Button("Pause")) { BillboardComp.Pause(); }
	}
	else
	{
		if (ImGui::Button("Play")) { BillboardComp.Play(); }
	}
	ImGui::SameLine();
	if (ImGui::Button("Stop")) { BillboardComp.Stop(); }
}

void FImguiPropertyWindow::ShowDirectionLightSettings(UDirectionLightComponent& LightComp) const
{
	ImGui::Separator();
	ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Direction Light Settings");

	FVector LightCol = LightComp.GetLightColor();
	if (ImGui::ColorEdit3("Light Color", &LightCol.X))
	{
		LightComp.SetLightColor(LightCol);
	}

	float LightIntensity = LightComp.GetIntensity();
	if (ImGui::DragFloat("Intensity", &LightIntensity, 0.05f, 0.0f, 50.0f))
	{
		LightComp.SetIntensity(LightIntensity);
	}

	FVector Direction = LightComp.GetLightDirection();
	if (ImGui::DragFloat3("Light Direction", &Direction.X, 1.0f, 89.0f))
	{
		Direction.Normalize();
		LightComp.SetLightDirection(Direction);
	}
}

void FImguiPropertyWindow::ShowPointLightSettings(UPointLightComponent& LightComp) const
{
	ImGui::Separator();
	ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Point Light Settings");

	FVector LightCol = LightComp.GetLightColor();
	if (ImGui::ColorEdit3("Light Color", &LightCol.X))
	{
		LightComp.SetLightColor(LightCol);
	}

	float LightIntensity = LightComp.GetIntensity();
	if (ImGui::DragFloat("Intensity", &LightIntensity, 0.05f, 0.0f, 50.0f))
	{
		LightComp.SetIntensity(LightIntensity);
	}

	float FallOffStart = LightComp.GetFallOffStart();
	if (ImGui::DragFloat("FallOffStart", &FallOffStart, 0.05f, 0.0f, 50.0f))
	{
		LightComp.SetFallOffStart(FallOffStart);
	}

	float FallOffEnd = LightComp.GetFallOffEnd();
	if (ImGui::DragFloat("FallOffEnd", &FallOffEnd, 0.05f, 0.0f, 50.0f))
	{
		LightComp.SetFallOffEnd(FallOffEnd);
	}
}

void FImguiPropertyWindow::ShowSpotLightSettings(USpotLightComponent& LightComp) const
{
	ImGui::Separator();
	ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "Spot Light Settings");

	FVector LightCol = LightComp.GetLightColor();
	if (ImGui::ColorEdit3("Light Color", &LightCol.X))
	{
		LightComp.SetLightColor(LightCol);
	}

	float LightIntensity = LightComp.GetIntensity();
	if (ImGui::DragFloat("Intensity", &LightIntensity, 0.05f, 0.0f, 50.0f))
	{
		LightComp.SetIntensity(LightIntensity);
	}

	float FallOffStart = LightComp.GetFallOffStart();
	if (ImGui::DragFloat("FallOffStart", &FallOffStart, 0.05f, 0.0f, 50.0f))
	{
		LightComp.SetFallOffStart(FallOffStart);
	}

	float FallOffEnd = LightComp.GetFallOffEnd();
	if (ImGui::DragFloat("FallOffEnd", &FallOffEnd, 0.05f, 0.0f, 50.0f))
	{
		LightComp.SetFallOffEnd(FallOffEnd);
	}

	FVector Direction = LightComp.GetLightDirection();
	if (ImGui::DragFloat3("Light Direction", &Direction.X, 1.0f, 89.0f))
	{
		Direction.Normalize();
		LightComp.SetLightDirection(Direction);
	}

	float SpotPower = LightComp.GetSpotPower();
	if (ImGui::DragFloat("SpotPower", &SpotPower, 0.05f, 0.0f, 50.0f))
	{
		LightComp.SetSpotPower(SpotPower);
	}
}

void FImguiPropertyWindow::ShowProjectileMovementSettings(UProjectileMovementComponent& MovComp) const
{
	FVector Velocity = MovComp.GetVelocity();
	if (ImGui::DragFloat3("Velocity", &Velocity.X, 1.0f, 89.0f))
	{
		MovComp.SetVelocity(Velocity);
	}

	float Gravity = MovComp.ProjectileGravityScale;
	if (ImGui::DragFloat("Gravity", &Gravity, 0.05f, 0.0f, 50.0f))
	{
		MovComp.ProjectileGravityScale = Gravity;
	}

	bool bIsHoming = MovComp.bIsHomingProjectile;
	if (ImGui::Checkbox("IsHoming", &bIsHoming))
	{
		MovComp.bIsHomingProjectile = bIsHoming;
	}

	float Hommer = MovComp.HomingAccelerationMagnitude;
	if (ImGui::DragFloat("Acceleration", &Hommer, 0.05f, 0.0f, 50.0f))
	{
		MovComp.HomingAccelerationMagnitude = Hommer;
	}
}

void FImguiPropertyWindow::ShowRotationMovementSettings(URotationMovementComponent& MovComp) const
{
	FVector RotationRate = MovComp.GetRotationRate();
	if (ImGui::DragFloat3("Rotation Rate", &RotationRate.X, 1.0f, 89.0f))
	{
		MovComp.SetRotationRate(RotationRate);
	}
}

void FImguiPropertyWindow::ShowStaticMeshSettings(AActor& Actor, UStaticMeshComponent& MeshComp, bool bIsRoot) const
{
	ImGui::Separator();
	ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Static Mesh Settings");


	FAssetRegistry& Registry = FAssetRegistry::GetInstance();

	if (ImGui::BeginTable(
		"StaticMeshAssetSlots",
		3,
		ImGuiTableFlags_SizingStretchSame
	))
	{
		ImGui::TableNextRow();

		ImGui::TableSetColumnIndex(0);
		ShowStaticMeshSlot(MeshComp);

		if (MeshComp.GetMaterialSlotLength() > 1)
		{
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ShowApplyAllMaterialSlot(MeshComp);

			bool bAllSlotsHaveMaterial = true;
			for (int i = 0; i < MeshComp.GetMaterialSlotLength(); ++i)
			{
				const FMaterialInstance* Instance = MeshComp.GetMaterialInstance(i);
				if (!Instance || !Instance->Material)
				{
					bAllSlotsHaveMaterial = false;
					break;
				}
			}

			if (bAllSlotsHaveMaterial)
			{
				ImGui::TableSetColumnIndex(1);
				ShowApplyAllTextureSlot(MeshComp);

				ImGui::TableSetColumnIndex(2);
				ShowApplyAllPipelineSlot(MeshComp);
			}
		}

		for (int i = 0; i < MeshComp.GetMaterialSlotLength(); ++i)
		{
			ImGui::PushID(i);

			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ShowMaterialSlot(MeshComp, i);

			const FMaterialInstance* Instance = MeshComp.GetMaterialInstance(i);
			if (Instance && Instance->Material)
			{
				ImGui::TableSetColumnIndex(1);
				ShowTextureSlot(MeshComp, i);

				ImGui::TableSetColumnIndex(2);
				ShowPipelineSlot(MeshComp, i);
			}

			ImGui::PopID();
		}

		ImGui::EndTable();
	}
}

void FImguiPropertyWindow::ShowMaterialSlot(UStaticMeshComponent& MeshComp, int Slot) const
{
	const FMaterialInstance* Instance = MeshComp.GetMaterialInstance(Slot);
	UMaterial* Material = Instance ? Instance->Material : nullptr;

	ImGui::Spacing();
	ImGui::TextDisabled("Material");

	// 슬롯 만들기
	float FullWidth = ImGui::GetContentRegionAvail().x;
	const FString Label = Material ? Material->GetID().ToString() : "No Material";
	ImGui::Button(Label.c_str(), ImVec2(FullWidth, SlotSize));

	// 드롭 타깃은 아이템을 그린 직후여야 한다.
	if (!ImGui::BeginDragDropTarget()) { return; }

	if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(ContentDragPayloadType))
	{
		const auto* Dropped = static_cast<const FContentDragPayload*>(Payload->Data);

		if (Dropped->Ptr)
		{
			UMaterial* NewMaterial = Dropped->Ptr->Cast<UMaterial>();

			if (NewMaterial)
			{
				MeshComp.SetMaterial(NewMaterial, Slot);
			}
		}
	}

	ImGui::EndDragDropTarget();
}

void FImguiPropertyWindow::ShowPipelineSlot(UStaticMeshComponent& MeshComp, int Slot) const
{
	UPipeline* Pipeline = MeshComp.GetMaterialInstance(Slot)->Pipeline;

	ImGui::Spacing();
	ImGui::TextDisabled("Pipeline");

	// 슬롯 만들기
	float FullWidth = ImGui::GetContentRegionAvail().x;
	ImGui::Button(Pipeline->GetID().ToString().c_str(), ImVec2(FullWidth, SlotSize));

	// 드롭 타깃은 아이템을 그린 직후여야 한다.
	if (!ImGui::BeginDragDropTarget()) { return; }

	if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(ContentDragPayloadType))
	{
		const auto* Dropped = static_cast<const FContentDragPayload*>(Payload->Data);

		if (Dropped->Ptr)
		{
			UPipeline* NewPipeline = Dropped->Ptr->Cast<UPipeline>();

			if (NewPipeline)
			{
				MeshComp.SetPipeline(NewPipeline, Slot);
			}
		}
	}

	ImGui::EndDragDropTarget();
}

void FImguiPropertyWindow::ShowTextureSlot(UStaticMeshComponent& MeshComp, int Slot) const
{
	UTexture* TextureAsset = MeshComp.GetMaterialInstance(Slot)->Texture;
	FTexture* CurrentTexture = nullptr;
	
	if (TextureAsset)
	{
		CurrentTexture = TextureAsset->Get();
	}

	ImGui::Spacing();
	ImGui::TextDisabled("Texture");

	// 슬롯 만들기
	float FullWidth = ImGui::GetContentRegionAvail().x;
	if (CurrentTexture && CurrentTexture->GetSRV())
	{
		const ImTextureID TexId = reinterpret_cast<ImTextureID>(CurrentTexture->GetSRV());
		ImGui::Image(TexId, ImVec2(FullWidth, SlotSize));
	}
	else
	{
		ImGui::Button("No\nTexture", ImVec2(FullWidth, SlotSize));
	}

	// 드롭 타깃은 아이템을 그린 직후여야 한다.
	if (!ImGui::BeginDragDropTarget()) { return; }

	if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(ContentDragPayloadType))
	{
		const auto* Dropped = static_cast<const FContentDragPayload*>(Payload->Data);

		if (Dropped->Ptr)
		{
			UTexture* Texture = Dropped->Ptr->Cast<UTexture>();

			if (Texture)
			{
				MeshComp.SetTexture(Texture, Slot);
			}
		}
	}

	ImGui::EndDragDropTarget();
}

void FImguiPropertyWindow::ShowStaticMeshSlot(UStaticMeshComponent& MeshComp) const
{
	const UStaticMesh* StaticMesh = MeshComp.GetMesh();

	ImGui::Spacing();
	ImGui::TextDisabled("StaticMesh");

	// 슬롯 만들기
	float FullWidth = ImGui::GetContentRegionAvail().x;
	
	const FString Label = StaticMesh ? StaticMesh->GetID().ToString() : "No StaticMesh";
	ImGui::Button(Label.c_str(), ImVec2(FullWidth, SlotSize));

	// 드롭 타깃은 아이템을 그린 직후여야 한다.
	if (!ImGui::BeginDragDropTarget()) { return; }

	if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(ContentDragPayloadType))
	{
		const auto* Dropped = static_cast<const FContentDragPayload*>(Payload->Data);

		if (Dropped->Ptr)
		{
			UStaticMesh* NewStaticMesh = Dropped->Ptr->Cast<UStaticMesh>();

			if (NewStaticMesh)
			{
				MeshComp.SetMesh(NewStaticMesh);
			}
		}
	}

	ImGui::EndDragDropTarget();
}


void FImguiPropertyWindow::ShowApplyAllMaterialSlot(UStaticMeshComponent& MeshComp) const
{
	ImGui::Spacing();
	ImGui::TextDisabled("Material");

	// 슬롯 만들기
	float FullWidth = ImGui::GetContentRegionAvail().x;
	ImGui::Button("Apply All Material", ImVec2(FullWidth, SlotSize));

	// 드롭 타깃은 아이템을 그린 직후여야 한다.
	if (!ImGui::BeginDragDropTarget()) { return; }

	if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(ContentDragPayloadType))
	{
		const auto* Dropped = static_cast<const FContentDragPayload*>(Payload->Data);

		if (Dropped->Ptr)
		{
			UMaterial* NewMaterial = Dropped->Ptr->Cast<UMaterial>();

			if (NewMaterial)
			{
				for (int i = 0; i < MeshComp.GetMaterialSlotLength(); ++i)
				{
					MeshComp.SetMaterial(NewMaterial, i);
				}
			}
		}
	}

	ImGui::EndDragDropTarget();
}

void FImguiPropertyWindow::ShowApplyAllPipelineSlot(UStaticMeshComponent& MeshComp) const
{
	ImGui::Spacing();
	ImGui::TextDisabled("Pipeline");

	// 슬롯 만들기
	float FullWidth = ImGui::GetContentRegionAvail().x;
	ImGui::Button("Apply All Pipeline", ImVec2(FullWidth, SlotSize));

	// 드롭 타깃은 아이템을 그린 직후여야 한다.
	if (!ImGui::BeginDragDropTarget()) { return; }

	if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(ContentDragPayloadType))
	{
		const auto* Dropped = static_cast<const FContentDragPayload*>(Payload->Data);

		if (Dropped->Ptr)
		{
			UPipeline* NewPipeline = Dropped->Ptr->Cast<UPipeline>();

			if (NewPipeline)
			{
				for (int i = 0; i < MeshComp.GetMaterialSlotLength(); ++i)
				{
					MeshComp.SetPipeline(NewPipeline, i);
				}
			}
		}
	}

	ImGui::EndDragDropTarget();
}

void FImguiPropertyWindow::ShowApplyAllTextureSlot(UStaticMeshComponent& MeshComp) const
{
	ImGui::Spacing();
	ImGui::TextDisabled("Texture");

	// 슬롯 만들기
	float FullWidth = ImGui::GetContentRegionAvail().x;
	ImGui::Button("Apply All Texture", ImVec2(FullWidth, SlotSize));

	// 드롭 타깃은 아이템을 그린 직후여야 한다.
	if (!ImGui::BeginDragDropTarget()) { return; }

	if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(ContentDragPayloadType))
	{
		const auto* Dropped = static_cast<const FContentDragPayload*>(Payload->Data);

		if (Dropped->Ptr)
		{
			UTexture* Texture = Dropped->Ptr->Cast<UTexture>();

			if (Texture)
			{
				for (int i = 0; i < MeshComp.GetMaterialSlotLength(); ++i)
				{
					MeshComp.SetTexture(Texture, i);
				}
			}
		}
	}

	ImGui::EndDragDropTarget();
}


void FImguiPropertyWindow::ShowGizmoSettings(FEditor& Editor) const
{
	static const char* GizmoModes[4] = { "None", "Translation", "Rotation", "Scale" };
	int SelectedItem = static_cast<int>(Editor.GetGizmo().Mode);
	if (ImGui::Combo("Gizmo Mode", &SelectedItem, GizmoModes, 4))
	{
		Editor.GetGizmo().Mode = static_cast<EGizmoMode>(SelectedItem);
	}

	if (SelectedItem == 3) // Scale
	{
		static const char* GizmoSpaces[] = { "Local" };
		SelectedItem = static_cast<int>(Editor.GetGizmo().GetSpace()) - 1;
		if (ImGui::Combo("Gizmo Space", &SelectedItem, GizmoSpaces, 1))
		{
			Editor.GetGizmo().SetGizmoSpace(static_cast<EGizmoSpace>(SelectedItem - 1));
		}
	}
	else if (SelectedItem != 0) // Translation, Rotation
	{
		static const char* GizmoSpaces[] = { "World", "Local" };
		SelectedItem = static_cast<int>(Editor.GetGizmo().GetSpace());
		if (ImGui::Combo("Gizmo Space", &SelectedItem, GizmoSpaces, 2))
		{
			Editor.GetGizmo().SetGizmoSpace(static_cast<EGizmoSpace>(SelectedItem));
		}
	}
}
