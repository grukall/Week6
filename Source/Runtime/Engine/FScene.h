#pragma once

#include "Runtime/Core/IntTypes.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/CoreUObject/UPrimitiveComponent.h"
#include "Runtime/CoreUObject/USceneComponent.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Engine/FSceneBVH.h"
#include "Runtime/Engine/FCulling.h"
#include <concepts>
#include <type_traits>

class UWorld;

#include "ThirdParty/Json/json.hpp"

class FScene{
    
public:
    UWorld* World;

    // 렌더링 컴포넌트 목록 반환
    [[nodiscard]] const TArray<UPrimitiveComponent*>& GetRenderComponents() const;
    [[nodiscard]] FRenderResourceLibrary* GetRenderResourceLibrary() const {
    return RenderResourceLibrary;
    }
    void SetRenderResourceLibrary(FRenderResourceLibrary* InRenderResourceLibrary);
    void AddRenderComponent(UPrimitiveComponent *prim);
    void RemoveRenderComponent(UPrimitiveComponent *prim);
  
    FSceneBVH& GetSceneBVH() { return SceneBVH; }
    const FSceneBVH& GetSceneBVH() const { return SceneBVH; }

    // 컬링 전용 월드 AABB 배열 (RenderComponents와 같은 인덱스)
    [[nodiscard]] const TArray<FAxisAlignedBoundingBox>& GetCullDataList() const { return CullDataList; }

    void MarkBoundsDirty(UPrimitiveComponent* Prim);

    // dirty 컴포넌트만 월드 AABB 재계산. 렌더 전에 프레임당 1회
    void UpdateDirtyBounds();

    //캐시해두는 오클루전 대상 Getter
    [[nodiscard]] const TArray<uint8>& GetOcclusionTargetFlags() const { return OcclusionTargetFlags; }

    void Release();

private:
  TArray<UPrimitiveComponent*> RenderComponents; // 렌더링큐 (Draw용)
  TMap<UPrimitiveComponent*, size_t> RenderIndices;

  FRenderResourceLibrary* RenderResourceLibrary = nullptr;
  

  FSceneBVH SceneBVH;

  //RenderComponents와 같은 인덱스
  TArray<FAxisAlignedBoundingBox> CullDataList;
  // 이번 프레임 재계산 대상
  TArray<UPrimitiveComponent*> DirtyBoundsList;

  //캐시해둘 오클루전 대상
  TArray<uint8> OcclusionTargetFlags;
};
