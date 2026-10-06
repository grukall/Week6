#include "FScene.h"

#include "Runtime/Core/FString.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UPrimitiveComponent.h"
#include "Runtime/CoreUObject/USceneComponent.h"
#include "Runtime/Engine/FArchive.h"
#include <algorithm>

#include "Runtime/CoreUObject/TObjectIterator.h"
#include "Runtime/Core/Log.h"


const TArray<UPrimitiveComponent *> & FScene::GetPrimitives() const {
  return RenderComponents;
}

void FScene::Initialize() {
}

void FScene::Release() {
  RenderComponents.clear();
  CullDataList.clear();
  DirtyBoundsList.clear();
  OcclusionTargetFlags.clear();
}

//void FScene::Serialize(FArchive &Archive) const {
//  Super::Serialize(Archive);
//
//  TArray<FArchive> ActorArchives;
//
//  for (const auto &Item : Actors) {
//    if (!Item) {
//      continue;
//    }
//
//    FArchive ItemArchive;
//    Item->Serialize(ItemArchive);
//    ActorArchives.push_back(ItemArchive);
//  }
//
//  Archive.SetArchiveArray("Actors", ActorArchives);
//}
//
//void FScene::Deserialize(const FArchive &Archive) {
//  Super::Deserialize(Archive);
//
//  if (Archive.IsNull("Actors")) {
//    // Actor 목록이 비어있음
//    return;
//  }
//
//  TArray<FArchive> ActorArchives = Archive.GetArchiveArray("Actors");
//
//  for (const auto &Item : ActorArchives) {
//    UClass *ClassType = UClass::FindByName(Item.GetString("Type"));
//    if (ClassType == nullptr) {
//      continue;
//    }
//
//    AActor *Actor = SpawnActor(ClassType);
//    if (!Actor) {
//      continue;
//    }
//    Actor->Deserialize(Item);
//
//    if (bActive) {
//      Actor->Register(*this);
//    }
//    if (bHasBegunPlay) {
//      Actor->BeginPlay();
//    }
//  }
//}

void FScene::Addprimitive(UPrimitiveComponent *prim) {
  if (prim == nullptr)
    return;

  if (std::find(RenderComponents.begin(), RenderComponents.end(), prim) ==
      RenderComponents.end()) {
      //RenderComponents에 넣기 전에 인덱스 설정
	const int32 NewIndex = static_cast<int32>(RenderComponents.size());
    prim->SetSceneIndex(NewIndex);
	prim->SetBatchIndex(NewIndex);

    RenderComponents.push_back(prim);
    SceneBVH.AddObject(prim);

    //처음엔 일단 그리자
    CullDataList.push_back(MakeAlwaysVisibleCullData());
    //오클루전 대상에도 추가
    OcclusionTargetFlags.push_back(0);
    MarkBoundsDirty(prim);
  }
}

void FScene::RemovePrimitive(UPrimitiveComponent *prim) {
    if (prim == nullptr || prim->GetSceneIndex() < 0)
        return;
    //TODO 제거할 때 마지막 요소와 교환하는 방식의 Swap and Pop으로 처리하도록 수정할 것

  std::erase(RenderComponents, prim);
  SceneBVH.RemoveObject(prim);

  const size_t Index = static_cast<size_t>(prim->GetSceneIndex());
  CullDataList.erase(CullDataList.begin() + Index);

  //오클루전 대상에서 제거
  OcclusionTargetFlags.erase(OcclusionTargetFlags.begin() + Index);

  // 당겨진 원소들의 인덱스 멤버 갱신
  for (size_t i = Index; i < RenderComponents.size(); ++i)
  {
      RenderComponents[i]->SetSceneIndex(static_cast<int32>(i));
	  RenderComponents[i]->SetBatchIndex(static_cast<int32>(i));
  }

  // 파괴될 포인터가 dirty 목록에 남지 않게
  if (prim->GetBoundDirtyQueued())
  {
      std::erase(DirtyBoundsList, prim);
      prim->SetBoundDirtyQueued(false);
  }

  prim->SetSceneIndex(-1);
}

void FScene::MarkBoundsDirty(UPrimitiveComponent* Prim)
{
    // 씬에 아직 추가 전이거나 이미 대기 중이면 무시
    if (Prim == nullptr || Prim->GetSceneIndex() < 0 || Prim->GetBoundDirtyQueued())
        return;

    Prim->SetBoundDirtyQueued(true);
    DirtyBoundsList.push_back(Prim);
}

void FScene::UpdateDirtyBounds()
{
    for (UPrimitiveComponent* Prim : DirtyBoundsList)
    {
        Prim->SetBoundDirtyQueued(false);
        Prim->UpdateWorldBounds();
        int32 Index = static_cast<size_t>(Prim->GetSceneIndex());
        CullDataList[Index] = Prim->GetWorldBounds();
        OcclusionTargetFlags[Index] = Prim->IsOcclusionTarget() ? 1 : 0;
    }
    DirtyBoundsList.clear();
}

void FScene::AddLightComponent(ULightComponent* Light)
{
    if (Light == nullptr)
        return;

    if (std::find(LightComponents.begin(), LightComponents.end(), Light) ==
        LightComponents.end()) {
        // const int32 NewIndex = static_cast<int32>(LightComponents.size());

        LightComponents.push_back(Light);
    }
}

void FScene::RemoveLightComponent(ULightComponent* Light)
{
    if (Light == nullptr)
        return;

    std::erase(LightComponents, Light);
}