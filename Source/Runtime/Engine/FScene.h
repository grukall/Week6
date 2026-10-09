#pragma once

#include "Runtime/Actors/AActor.h"
#include "Runtime/Core/IntTypes.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/CoreUObject/UPrimitiveComponent.h"
#include "Runtime/CoreUObject/USceneComponent.h"
#include "Runtime/CoreUObject/ULightComponent.h"
#include "Runtime/Rendering/FRenderResourceLibrary.h"
#include "Runtime/Engine/FSceneBVH.h"
#include "Runtime/Engine/FCulling.h"
#include <concepts>
#include <type_traits>


#include "ThirdParty/Json/json.hpp"
#include <Runtime/CoreUObject/UExponentialHeightFogComponent.h>

class FScene
{

public:

  void Initialize();
  void Release();

  // 렌더링 컴포넌트 목록 반환
  [[nodiscard]] const TArray<UPrimitiveComponent*>& GetPrimitives() const;

  //virtual void Serialize(FArchive& Archive) const override;
  //virtual void Deserialize(const FArchive& Archive) override;

  void AddLightComponent(ULightComponent* Light);
  void RemoveLightComponent(ULightComponent* Light);
  TArray<ULightComponent*> GetLightComponents() const { return LightComponents; }

  void AddFogComponent(UExponentialHeightFogComponent* Fog);
  void RemoveFogComponent(UExponentialHeightFogComponent* Fog);
  TArray<UExponentialHeightFogComponent*> GetFogComponents() const { return FogComponents; }

  void Addprimitive(UPrimitiveComponent *prim);
  void RemovePrimitive(UPrimitiveComponent *prim);

    FSceneBVH& GetSceneBVH() { return SceneBVH; }
    const FSceneBVH& GetSceneBVH() const { return SceneBVH; }

    // 컬링 전용 월드 AABB 배열 (RenderComponents와 같은 인덱스)
    [[nodiscard]] const TArray<FAxisAlignedBoundingBox>& GetCullDataList() const { return CullDataList; }

    void MarkBoundsDirty(UPrimitiveComponent* Prim);

    // dirty 컴포넌트만 월드 AABB 재계산. 렌더 전에 프레임당 1회
    void UpdateDirtyBounds();

    //캐시해두는 오클루전 대상 Getter
    [[nodiscard]] const TArray<uint8>& GetOcclusionTargetFlags() const { return OcclusionTargetFlags; }

private:
  TArray<UPrimitiveComponent*> RenderComponents; // 렌더링큐 (Draw용)
  TArray<ULightComponent*> LightComponents;
  TArray<UExponentialHeightFogComponent*> FogComponents;

  FSceneBVH SceneBVH;

  //RenderComponents와 같은 인덱스
  TArray<FAxisAlignedBoundingBox> CullDataList;
  // 이번 프레임 재계산 대상
  TArray<UPrimitiveComponent*> DirtyBoundsList;

  //캐시해둘 오클루전 대상
  TArray<uint8> OcclusionTargetFlags;
};
