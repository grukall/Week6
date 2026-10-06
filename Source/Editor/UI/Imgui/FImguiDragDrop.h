#pragma once

#include "Runtime/Asset/UAsset.h";
#include "Runtime/CoreUObject/USceneComponent.h";
#include "Runtime/Actors/AActor.h";

struct FContentDragPayload
{
    UAsset* Ptr;
};

struct FOutlinerDragPayload
{
    AActor* Actor;
    USceneComponent* Component;
};

inline constexpr const char* ContentDragPayloadType = "ENGINE_CONTENT";
inline constexpr const char* OutlinerDragPayloadType = "ENGINE_OUTLINER";
inline constexpr const char* ComponentDragPayloadType = "ENGINE_COMPONENT";