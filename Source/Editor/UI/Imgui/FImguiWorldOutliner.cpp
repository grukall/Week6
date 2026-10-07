#include "FImguiWorldOutliner.h"
#include "Runtime/Actors/AActor.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/USceneComponent.h"
#include "Runtime/Engine/ULevel.h"
#include "ThirdParty/Imgui/imgui.h"
#include <string>
#include <algorithm>
#include <Runtime/Engine/UWorld.h>
#include <Runtime/Core/TArray.h>
#include <Editor/Core/EditorConstant.h>
#include <ThirdParty/Imgui/imgui_internal.h>

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

	TArray<AActor*> GetChildActors(const AActor& Actor)
	{
		TArray<AActor*> ChildActors;
		for (USceneComponent* Comp : Actor.GetAttachedComponents())
		{
			if (!Comp) { continue; }

			for (USceneComponent* Child : Comp->GetChildren())
			{
				if (!Child || Child->GetSceneOwner() != Comp) { continue; }

				AActor* ChildActor = Child->GetActorOwner();
				if (ChildActor && ChildActor != &Actor && ChildActor->GetRootComponent() == Child)
				{
					ChildActors.push_back(ChildActor);
				}
			}
		}
		return ChildActors;
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
		Item.Actor = Actor;
		Item.Key = Actor->GetName().GetHash();

		const char* ClassName = Actor->GetClass() ? Actor->GetClass()->GetDisplayName().c_str() : "Actor";
		Item.DisplayLabel = Actor->GetName().ToString() + " (" + ClassName + ")";

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

	// 활성 뷰포트가 보는 월드의 액터를 보여 준다 (PIE 중에는 PIE 월드)
	UWorld* CurrentWorld = Editor.GetViewWorld();
	if (!CurrentWorld)
	{
		ImGui::TextDisabled("No Current World");
		ImGui::End();
		return;
	}

	// 검색 필터 버퍼
	const bool bFilterChanged = ShowSearchBar();
	ImGui::Separator();

	auto Actors = CurrentWorld->GetActors();
	AActor* SelectedActor = Editor.GetSelectedActor();
	UActorComponent* SelectedComponent = Editor.GetSelectedComponent();
	// 액터 목록 표시
	ImGui::BeginChild("ActorList", ImVec2(0.0f, -ImGui::GetFrameHeightWithSpacing()), false);

	if (Editor.HierarchyVersion != LastHierarchyVersion)
	{
		LastHierarchyVersion = Editor.HierarchyVersion;
		bCacheDirty = true;
	}

	if (bCacheDirty || CurrentWorld != LastWorld || Actors->size() != LastActorCount)
	{
		RefreshCache(CurrentWorld);
		UpdateFilter(CurrentFilterStr.c_str());
		RebuildDisplayList();
		LastWorld = CurrentWorld;
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

	ImGui::EndChild();

	if (PendingAttach.IsSet())
	{
		ApplyAttach(*PendingAttach.Get());
		PendingAttach.Reset();
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
					UObject* Object = NewObject(Item);
					UActorComponent* NewComponent = Object ? Object->Cast<UActorComponent>() : nullptr;
					if (NewComponent)
					{
						SelectedActor->AddComponent(NewComponent);
						bCacheDirty = true;
					}
					else if (Object)
					{
						DestroyObject(Object);
					}
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

void FImguiWorldOutliner::RefreshCache(UWorld* World)
{
	CachedActors.clear();
	size_t ActorCount = World->GetAllActorsCount();
	CachedActors.reserve(ActorCount);
	World->ForEachActors([&](AActor* Actor)
		{
			if (!Actor) { return; }

			CachedActors.push_back(MakeActorItem(Actor));
		});

	LastActorCount = ActorCount;
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

void FImguiWorldOutliner::ShowActorNode_Cached(FEditor& Editor, const FOutlinerItem& Item, AActor* SelectedActor)
{
	if (!Item.Actor) return;

	// 목록을 평탄하게 펴서 그리므로 깊이만큼 직접 들여쓴다.
	const float IndentWidth = Item.Depth * 16.0f;
	if (Item.Depth > 0)
	{
		ImGui::Indent(IndentWidth);
	}

	const bool bIsOpen = ExpandedNodeKeys.contains(Item.Key);
	const bool bIsSelected = (Item.Actor == SelectedActor);

	ImGuiTreeNodeFlags NodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (bIsSelected)
	{
		NodeFlags |= ImGuiTreeNodeFlags_Selected;
	}
	if (!Item.bHasChildren)
	{
		NodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}

	const bool bNodeOpen = ImGui::TreeNodeEx(Item.Actor, NodeFlags, "%s", Item.DisplayLabel.c_str());
	HandleRowDragDrop(Item.Actor, Item.DisplayLabel.c_str());

	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
	{
		PressedActor = Item.Actor;
	}

	if (PressedActor == Item.Actor && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
	{
		if (ImGui::IsItemHovered() && !ImGui::IsDragDropActive())
		{
			Editor.SelectActor(Item.Actor);
		}
		PressedActor = nullptr;
	}

	if (Item.bHasChildren)
	{
		if (bNodeOpen != bIsOpen)
		{
			if (bNodeOpen)
			{
				ExpandedNodeKeys.insert(Item.Key);
			}
			else
			{
				ExpandedNodeKeys.erase(Item.Key);
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

void FImguiWorldOutliner::HandleRowDragDrop(AActor* RowActor, const char* Label)
{
	if (ImGui::BeginDragDropSource())
	{
		// ImGui가 내부 버퍼로 복사하므로 지역 변수를 넘겨도 된다.
		FOutlinerDragPayload Data{ RowActor, nullptr };
		ImGui::SetDragDropPayload(OutlinerDragPayloadType, &Data, sizeof(Data));
		ImGui::TextUnformatted(Label);
		ImGui::EndDragDropSource();
	}

	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(OutlinerDragPayloadType))
		{
			const auto* Dragged = static_cast<const FOutlinerDragPayload*>(Payload->Data);
			PendingAttach.Set(FOutlinerAttachRequest{ *Dragged, RowActor, nullptr });
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
	const TArray<AActor*> ChildActors = GetChildActors(*ActorItem.Actor);

	FOutlinerItem Row = ActorItem;
	Row.Depth = Depth;
	Row.bHasChildren = !ChildActors.empty();
	DisplayList.push_back(Row);

	if (!ExpandedNodeKeys.contains(ActorItem.Key)) return;

	for (AActor* ChildActor : ChildActors)
	{
		AppendActorRows(MakeActorItem(ChildActor), Depth + 1);
	}
}