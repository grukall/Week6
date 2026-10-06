#include "FImguiWorldOutliner.h"
#include "Runtime/Actors/AActor.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/USceneComponent.h"
#include "Runtime/Engine/FScene.h"
#include "ThirdParty/Imgui/imgui.h"
#include <string>
#include <algorithm>
#include <Editor/Core/EditorConstant.h>

namespace
{
	// Node가 Ancestor 자신이거나 그 자손이면 true
	bool IsSelfOrDescendant(const USceneComponent* Node, const USceneComponent* Ancestor)
	{
		for (const USceneComponent* It = Node; It; It = It->GetSceneOwner())
		{
			if (It == Ancestor) { return true; }
		}
		return false;
	}

	enum class EOutlinerChildKind : uint8
	{
		None,
		Component,
		Actor
	};

	// Parent 아래에 어떤 행으로 그릴지 분류한다. 같은 액터의 컴포넌트이거나, 붙어 있는 다른 액터의 루트만 그린다.
	EOutlinerChildKind ClassifyChild(const USceneComponent& Parent, const USceneComponent* Child)
	{
		// Children에 남아 있어도 실제 부모가 다르면 그리지 않는다.
		if (!Child || Child->GetSceneOwner() != &Parent)
		{
			return EOutlinerChildKind::None;
		}

		const AActor* ChildActor = Child->GetActorOwner();
		if (!ChildActor)
		{
			return EOutlinerChildKind::None;
		}
		if (ChildActor == Parent.GetActorOwner())
		{
			return EOutlinerChildKind::Component;
		}
		return ChildActor->GetRootComponent() == Child ? EOutlinerChildKind::Actor : EOutlinerChildKind::None;
	}

	bool HasOutlinerChildren(USceneComponent& Comp)
	{
		for (const USceneComponent* Child : Comp.GetChildren())
		{
			if (ClassifyChild(Comp, Child) != EOutlinerChildKind::None) { return true; }
		}
		return false;
	}

	// 액터 행 바로 아래에 그릴 컴포넌트인지. 나머지는 부모 컴포넌트 아래에서 그린다.
	bool IsTopLevelComponent(const AActor& Actor, const USceneComponent* Comp)
	{
		return Comp && (Comp == Actor.GetRootComponent() || !Comp->GetSceneOwner());
	}

	// 다른 액터에 붙은 액터는 최상위 목록이 아니라 부모 아래에서 그린다.
	bool IsAttachedActor(const AActor& Actor)
	{
		const USceneComponent* Root = Actor.GetRootComponent();
		return Root && Root->GetSceneOwner();
	}

	FOutlinerItem MakeActorItem(AActor* Actor)
	{
		FOutlinerItem Item;
		Item.Type = EOutlinerItemRowType::Actor;
		Item.Actor = Actor;
		Item.UUID = Actor->GetUUID();

		const char* ClassName = Actor->GetClass() ? Actor->GetClass()->GetDisplayName().c_str() : "Actor";
		Item.DisplayLabel = FString(ClassName) + " (ID: " + std::to_string(Item.UUID) + ")";

		Item.LowerLabel = Item.DisplayLabel;
		std::transform(Item.LowerLabel.begin(), Item.LowerLabel.end(), Item.LowerLabel.begin(),
			[](unsigned char c) { return static_cast<char>(::tolower(c)); });

		return Item;
	}
}

void FImguiWorldOutliner::Process(FEditor& Editor)
{
	if (Editor.bHideUI || Editor.bZenMode)
	{
		return;
	}

	ImGui::Begin("World Outliner");

	ULevel* Level = Editor.GetCurrentLevel();
	if (!Level)
	{
		ImGui::TextDisabled("No Active Scene");
		ImGui::End();
		return;
	}

	ImGui::Checkbox("아웃라이너 최적화 적용", &bUseOptimized);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("체크: 캐쉬된 라벨 및 화면에 보이는 일부 노드만 랜더\n"
			"해제: 매 프레임 동적 생성 및 전체 순회");
	}
	ImGui::Separator();

	// 검색 필터 버퍼
	const bool bFilterChanged = ShowSearchBar();
	ImGui::Separator();

	auto& Actors = Level->GetActors();
	AActor* SelectedActor = Editor.GetSelectedActor();
	UActorComponent* SelectedComponent = Editor.GetSelectedComponent();
	// 액터 목록 표시
	ImGui::BeginChild("ActorList", ImVec2(0.0f, -ImGui::GetFrameHeightWithSpacing()), false);

	if (bUseOptimized)
	{
		if (bCacheDirty || Level != LastLevel || Actors.size() != LastActorCount)
		{
			RefreshCache(Level);
			UpdateFilter(CurrentFilterStr.c_str());
			RebuildDisplayList();
			LastLevel = Level;
			bDisplayListDirty = false;
		}
		else if (bFilterChanged || bDisplayListDirty)
		{
			if (bFilterChanged)
			{
				UpdateFilter(CurrentFilterStr.c_str());
			}
			RebuildDisplayList();
			bDisplayListDirty = false;
		}

		ImGuiListClipper Clipper;
		Clipper.Begin(static_cast<int>(DisplayList.size()));

		while (Clipper.Step())
		{
			for (int i = Clipper.DisplayStart; i < Clipper.DisplayEnd; ++i)
			{
				ShowActorNode_Cached(Editor, DisplayList[i], SelectedActor);
			}
		}
	}
	else
	{
		for (AActor* Actor : Actors)
		{
			if (!Actor || IsAttachedActor(*Actor))
			{
				continue;
			}
			//액터 노드 표시
			ShowActorNode(Editor, Actor, CurrentFilterStr.c_str(), SelectedActor);
		}
	}

	ImGui::EndChild();

	if (PendingAttach)
	{
		ApplyAttach(*PendingAttach);
		PendingAttach.reset();
		bCacheDirty = true;
	}

	ImGui::Separator();

	// 하단 컨트롤 영역
	if (SelectedActor)
	{
		if (ImGui::Button("Delete"))
		{
			if (SelectedComponent)
			{
				// 선택을 먼저 풀어야 삭제된 컴포넌트의 트랜스폼이 액터 루트에 적용되지 않는다.
				AActor* OwnerActor = SelectedComponent->GetActorOwner();
				Editor.UnSelectActor();
				if (OwnerActor)
				{
					OwnerActor->DeleteComponent(SelectedComponent);
				}
				bCacheDirty = true;
			}
			else
			{
				AActor* ActorToDelete = SelectedActor;
				Editor.UnSelectActor();
				ActorToDelete->Destroy();
				bCacheDirty = true;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Add"))
		{
			ImGui::OpenPopup("Components");
		}
		ImGui::SameLine();
		ImGui::TextUnformatted(SelectedActor ? "<None>" : EditorConstant::SpawnableComponents[0]->GetDisplayName().c_str());
		if (ImGui::BeginPopup("Components"))
		{
			for (const auto Item : EditorConstant::SpawnableComponents)
			{
				const bool bIsSelected = SelectedActor->GetClass() == Item;
				const char* ItemDisplayName = Item->GetUClassName().c_str();
				if (ImGui::Selectable(ItemDisplayName, bIsSelected))
				{
					USceneComponent* NewSceneComponent = NewObject(Item)->Cast<USceneComponent>();
					SelectedActor->AddComponent(NewSceneComponent);
					bCacheDirty = true;
				}

				if (bIsSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndPopup();
		}

	}
	else
	{
		ImGui::TextDisabled("No Selection");
	}

	ImGui::End();
}

void FImguiWorldOutliner::RefreshCache(ULevel* InLevel)
{
	CachedActors.clear();
	const auto& Actors = InLevel->GetActors();
	CachedActors.reserve(Actors.size());

	for (AActor* Actor : Actors)
	{
		if (!Actor) { continue; }

		CachedActors.push_back(MakeActorItem(Actor));
	}

	LastActorCount = Actors.size();
	bCacheDirty = false;
}

void FImguiWorldOutliner::UpdateFilter(const FString& FilterStr)
{
	FilteredIndices.clear();
	FilteredIndices.reserve(CachedActors.size());

	const bool bHasFilter = !FilterStr.empty();

	for (int32 i = 0; i < static_cast<int32>(CachedActors.size()); ++i)
	{
		if (!bHasFilter || CachedActors[i].LowerLabel.find(FilterStr) != FString::npos)
		{
			FilteredIndices.push_back(i);
		}
	}

	LastFilterStr = FilterStr;
}

void FImguiWorldOutliner::ShowActorNode(FEditor& Editor, AActor* Actor, const std::string& FilterStr, AActor* SelectedActor)
{
	if (!Actor->GetClass()) { return; }

	// 검색어 필터링
	if (!FilterStr.empty())
	{
		// 액터 이름 생성
		const FString& ActorName = Actor->GetClass()->GetDisplayName();
		if (ActorName.find(FilterStr) == FString::npos)
		{
			return;
		}
	}

	const bool bIsSelected = (Actor == SelectedActor);
	ImGuiTreeNodeFlags NodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (bIsSelected)
	{
		NodeFlags |= ImGuiTreeNodeFlags_Selected;
	}

	const auto& Components = Actor->GetAttachedComponents();
	if (Components.empty())
	{
		NodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}

	// 트리 노드 렌더링
	const bool bNodeOpen = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<uintptr_t>(Actor->GetUUID())), NodeFlags, "%s (ID: %u)", Actor->GetClass()->GetDisplayName().c_str(), Actor->GetUUID());
	HandleRowDragDrop(Actor, nullptr, Actor->GetClass()->GetDisplayName().c_str());

	// 클릭 시 액터 선택
	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
	{
		Editor.SelectActor(Actor);
	}


	// 자식 컴포넌트 목록 전개
	if (bNodeOpen && !Components.empty())
	{
		for (USceneComponent* Comp : Components)
		{
			if (IsTopLevelComponent(*Actor, Comp))
			{
				ShowComponentNode(Editor, *Comp);
			}
		}

		ImGui::TreePop();
	}

}

void FImguiWorldOutliner::ShowActorNode_Cached(FEditor& Editor, const FOutlinerItem& Item, AActor* SelectedActor)
{
	const bool bIsActorRow = (Item.Type == EOutlinerItemRowType::Actor);
	if (!Item.Actor || (!bIsActorRow && !Item.Component)) return;

	// 목록을 평탄하게 펴서 그리므로 깊이만큼 직접 들여쓴다.
	const float IndentWidth = Item.Depth * 16.0f;
	if (Item.Depth > 0)
	{
		ImGui::Indent(IndentWidth);
	}

	const bool bIsOpen = ExpandedNodeUUIDs.contains(Item.UUID);
	const bool bIsSelected = bIsActorRow
		? (Item.Actor == SelectedActor)
		: (Item.Component == Editor.GetSelectedComponent());

	ImGuiTreeNodeFlags NodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (bIsSelected)
	{
		NodeFlags |= ImGuiTreeNodeFlags_Selected;
	}
	if (!Item.bHasChildren)
	{
		NodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}

	const bool bNodeOpen = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<uintptr_t>(Item.UUID)), NodeFlags, "%s", Item.DisplayLabel.c_str());
	HandleRowDragDrop(Item.Actor, bIsActorRow ? nullptr : Item.Component, Item.DisplayLabel.c_str());

	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
	{
		if (bIsActorRow)
		{
			Editor.SelectActor(Item.Actor);
		}
		else
		{
			Editor.SelectComponent(Item.Component);
		}
	}

	if (Item.bHasChildren)
	{
		if (bNodeOpen != bIsOpen)
		{
			if (bNodeOpen)
			{
				ExpandedNodeUUIDs.insert(Item.UUID);
			}
			else
			{
				ExpandedNodeUUIDs.erase(Item.UUID);
			}
			bDisplayListDirty = true;
		}

		// 자식은 다음 행들이 따로 그리므로 바로 닫는다.
		if (bNodeOpen)
		{
			ImGui::TreePop();
		}
	}

	if (Item.Depth > 0)
	{
		ImGui::Unindent(IndentWidth);
	}
}

void FImguiWorldOutliner::ShowComponentNode(FEditor& Editor, USceneComponent& Comp)
{

	const bool bIsSelected = (&Comp == Editor.GetSelectedComponent());
	const bool bHasChildren = HasOutlinerChildren(Comp);

	const char* CompClassName = Comp.GetClass() ? Comp.GetClass()->GetDisplayName().c_str() : "Component";

	ImGuiTreeNodeFlags NodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (bIsSelected)
	{
		NodeFlags |= ImGuiTreeNodeFlags_Selected;
	}
	if (!bHasChildren)
	{
		NodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}

	const bool bNodeOpen = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<uintptr_t>(Comp.GetUUID())), NodeFlags, "%s (ID: %u)", CompClassName, Comp.GetUUID());
	HandleRowDragDrop(Comp.GetActorOwner(), &Comp, CompClassName);

	// 클릭 시 컴포넌트 선택
	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
	{
		Editor.SelectComponent(&Comp);
	}

	// 자식 컴포넌트와 붙어 있는 액터 전개
	if (bNodeOpen && bHasChildren)
	{
		for (USceneComponent* Child : Comp.GetChildren())
		{
			switch (ClassifyChild(Comp, Child))
			{
			case EOutlinerChildKind::Component:
				ShowComponentNode(Editor, *Child);
				break;
			case EOutlinerChildKind::Actor:
				ShowActorNode(Editor, Child->GetActorOwner(), std::string(), Editor.GetSelectedActor());
				break;
			case EOutlinerChildKind::None:
				break;
			}
		}

		ImGui::TreePop();
	}
}

void FImguiWorldOutliner::HandleRowDragDrop(AActor* RowActor, USceneComponent* RowComp, const char* Label)
{
	if (ImGui::BeginDragDropSource())
	{
		// ImGui가 내부 버퍼로 복사하므로 지역 변수를 넘겨도 된다.
		FOutlinerDragPayload Data{ RowActor, RowComp };
		ImGui::SetDragDropPayload(OutlinerDragPayloadType, &Data, sizeof(Data));
		ImGui::TextUnformatted(Label);
		ImGui::EndDragDropSource();
	}

	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(OutlinerDragPayloadType))
		{
			const auto* Dragged = static_cast<const FOutlinerDragPayload*>(Payload->Data);
			PendingAttach = FOutlinerAttachRequest{ *Dragged, RowActor, RowComp };
		}
		ImGui::EndDragDropTarget();
	}
}

void FImguiWorldOutliner::ApplyAttach(const FOutlinerAttachRequest& Request)
{
	AActor* DraggedActor = Request.Dragged.Actor;
	AActor* TargetActor = Request.TargetActor;
	if (!DraggedActor || !TargetActor) { return; }

	// 액터 행은 루트 컴포넌트로 바꿔서 컴포넌트끼리의 문제로 통일한다.
	USceneComponent* Dragged = Request.Dragged.Component ? Request.Dragged.Component : DraggedActor->GetRootComponent();
	USceneComponent* Target = Request.TargetComp ? Request.TargetComp : TargetActor->GetRootComponent();
	if (!Dragged || !Target) { return; }

	// 자기 자신이나 자기 자손 밑으로는 못 붙인다. 순환이 생기면 GetGlobalTransform이 무한 재귀한다.
	if (IsSelfOrDescendant(Target, Dragged)) { return; }
	if (Dragged->GetSceneOwner() == Target) { return; }

	const bool bIsRoot = (Dragged == DraggedActor->GetRootComponent());

	if (DraggedActor == TargetActor)
	{
		// 같은 액터 안에서 부모만 바꾼다. 루트는 옮길 수 없다.
		if (bIsRoot) { return; }
		Dragged->SetupAttachment(Target, true);
	}
	else if (bIsRoot)
	{
		// 액터 → 액터. 트랜스폼 부모만 바꾸고 소유 액터는 유지한다.
		Dragged->SetupAttachment(Target, true);
	}
	else
	{
		// 컴포넌트를 다른 액터로 넘기려면 AActor 쪽에 소유권 이전 함수가 필요하다.
		return;
	}

	Dragged->MarkActorTransformDirty();
}



bool FImguiWorldOutliner::ShowSearchBar()
{
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::InputTextWithHint("##OutlinerFilter", "Search...", FilterBuffer, sizeof(FilterBuffer)))
	{
		CurrentFilterStr = FilterBuffer;

		std::transform(CurrentFilterStr.begin(), CurrentFilterStr.end(), CurrentFilterStr.begin(),
			[](unsigned char c) { return static_cast<char>(::tolower(c)); });

		return true;;
	}

	return false;
}

void FImguiWorldOutliner::RebuildDisplayList()
{
	DisplayList.clear();

	for (int32 ItemIndex : FilteredIndices)
	{
		const FOutlinerItem& ActorItem = CachedActors[ItemIndex];
		if (!ActorItem.Actor || IsAttachedActor(*ActorItem.Actor)) continue;

		AppendActorRows(ActorItem, 0);
	}
}

void FImguiWorldOutliner::AppendActorRows(const FOutlinerItem& ActorItem, int32 Depth)
{
	AActor* Actor = ActorItem.Actor;
	const auto& Components = Actor->GetAttachedComponents();

	FOutlinerItem Row = ActorItem;
	Row.Depth = Depth;
	Row.bHasChildren = !Components.empty();
	DisplayList.push_back(Row);

	if (!ExpandedNodeUUIDs.contains(ActorItem.UUID)) return;

	for (USceneComponent* Comp : Components)
	{
		if (IsTopLevelComponent(*Actor, Comp))
		{
			AppendComponentRows(*Comp, Depth + 1);
		}
	}
}

void FImguiWorldOutliner::AppendComponentRows(USceneComponent& Comp, int32 Depth)
{
	const bool bHasChildren = HasOutlinerChildren(Comp);

	FOutlinerItem Row;
	Row.Type = EOutlinerItemRowType::Component;
	Row.UUID = Comp.GetUUID();
	const char* CompName = Comp.GetClass() ? Comp.GetClass()->GetDisplayName().c_str() : "Component";
	Row.DisplayLabel = FString(CompName) + " (ID: " + std::to_string(Row.UUID) + ")";
	Row.Depth = Depth;
	Row.bHasChildren = bHasChildren;
	Row.Component = &Comp;
	Row.Actor = Comp.GetActorOwner();
	DisplayList.push_back(Row);

	if (!bHasChildren || !ExpandedNodeUUIDs.contains(Comp.GetUUID())) return;

	for (USceneComponent* Child : Comp.GetChildren())
	{
		switch (ClassifyChild(Comp, Child))
		{
		case EOutlinerChildKind::Component:
			AppendComponentRows(*Child, Depth + 1);
			break;
		case EOutlinerChildKind::Actor:
			AppendActorRows(MakeActorItem(Child->GetActorOwner()), Depth + 1);
			break;
		case EOutlinerChildKind::None:
			break;
		}
	}
}