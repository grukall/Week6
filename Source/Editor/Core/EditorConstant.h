#pragma once

#include "Runtime/CoreUObject/UClass.h"

#include "Runtime/Actors/AActor.h"
#include "Runtime/Actors/AAppleNormalActor.h"
#include "Runtime/Actors/AAppleBittenActor.h"
#include "Runtime/Actors/ACubeActor.h"
#include "Runtime/Actors/ASphereActor.h"
#include "Runtime/Actors/ACylinderActor.h"
#include "Runtime/Actors/ABillboardActor.h"
#include "Runtime/Actors/AAnimatedBillboardActor.h"
#include "Runtime/Actors/ASpotlightActor.h"
#include "Runtime/Actors/ATextRenderActor.h"
#include "Runtime/Actors/ACatActor.h"

#include "Runtime/CoreUObject/Mesh/UStaticMeshComponent.h"

#include "Runtime/CoreUObject/USceneComponent.h"
#include "Runtime/CoreUObject/USpotLightComponent.h"
#include "Runtime/CoreUObject/UTextInstanceComponent.h"
#include "Runtime/CoreUObject/UAnimatedBillboardComp.h"
#include "Runtime/CoreUObject/UBillBoardComp.h"

namespace EditorConstant
{

	/// <summary>
	/// 에디터에서 스폰 가능한 액터들을 정의합니다.
	/// </summary>
	inline UClass* const SpawnableActors[]
	{
	   AAppleNormalActor::StaticClass(),
	   AAppleBittenActor::StaticClass(),
	   ACubeActor::StaticClass(),
	   ASphereActor::StaticClass(),
	   ACylinderActor::StaticClass(),
	   ABillboardActor::StaticClass(),
	   AAnimatedBillboardActor::StaticClass(),
	   ASpotlightActor::StaticClass(),
	   ATextRenderActor::StaticClass(),
	};


	/// <summary>
	/// 에디터에서 부착 가능한 컴포넌트들을 정의합니다.
	/// </summary>
	inline UClass* const SpawnableComponents[]
	{
	   UStaticMeshComponent::StaticClass(),
	   USceneComponent::StaticClass(),
	   USpotLightComponent::StaticClass(),
	   UTextInstanceComponent::StaticClass(),
	   UAnimatedBillboardComp::StaticClass(),
	   UBillBoardComp::StaticClass(),
	};
}
