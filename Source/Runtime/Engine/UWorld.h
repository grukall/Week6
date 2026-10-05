#pragma once

#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/Actors/AActor.h"
#include "Runtime/Geometry/FTransform.h"
#include "Runtime/Math/FVector.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Core/PointerTypes.h"
#include "Runtime/Engine/FScene.h"
#include "Runtime/Engine/ULevel.h"
#include "EngineBaseTypes.h"
#include <concepts>
#include <utility>

class UClass;

enum class EWorldType {
    Editor,
    EditorPreview,
    PIE,
    Game,
};

struct FWorldContext
{
	EWorldType WorldType = EWorldType::Editor;
	UWorld* World = nullptr;

    FWorldContext(EWorldType InWorldType, UWorld *InWorld) : WorldType(InWorldType), World(InWorld) {}
};

class UWorld : public UObject
{
	GENERATED_BODY()
	DECLARE_UCLASS(UWorld, UObject)

public:

    [[nodiscard]] bool HasBegunPlay() const { return bHasBegunPlay; }

    void Initialize(EWorldType _WorldType);
    void Release();

    void BeginPlay();
    void Tick(float DeltaTime, ELevelTick eTickType);
    void EndPlay();

	ULevel* GetPersistentLevel() const { return PersistentLevel; }
	[[nodiscard]] FScene* GetScene() const { return Scene.get(); }
	[[nodiscard]] EWorldType GetWorldType() const { return WorldType; }
    const TArray<AActor*>* GetActors() const { return PersistentLevel ? PersistentLevel->GetActors() : &Dummy; }

	virtual void Serialize(FArchive& Archive) const override;
	virtual void Deserialize(const FArchive& Archive) override;

public:

	size_t GetAllActorsCount() const {
		return GetActors()->size();
	}

	//월드의 모든 액터를 순회하며 func 호출. func는 AActor*를 인자로 받는 함수여야 한다.
    template<typename Func>
    void ForEachActors(Func&& func) const
    {
		for (AActor* Actor : *GetActors()) {
			func(Actor);
		}
    }

public:
    //============================
    // SpawnActor Templates
    // 기본 위치와 크기로 액터 생성
	AActor* SpawnActor(UClass* ClassType);


    template <typename TActor>
        requires std::derived_from<TActor, AActor>
    TActor* SpawnActor() {
        return SpawnActor<TActor>(FVector(0.0f, 0.0f, 0.0f),
            FVector(1.0f, 1.0f, 1.0f));
    }

    // 첫번째 인자가 벡터가 아닐 때 기본 위치와 크기 전달
    template <typename TActor, typename FirstArg, typename... RestArgs>
        requires std::derived_from<TActor, AActor> &&
    (!std::is_same_v<std::decay_t<FirstArg>, FVector>)
        TActor* SpawnActor(FirstArg&& First, RestArgs &&...Rest) {
        return SpawnActor<TActor>(
            FVector(0.0f, 0.0f, 0.0f), FVector(1.0f, 1.0f, 1.0f),
            std::forward<FirstArg>(First), std::forward<RestArgs>(Rest)...);
    }

    // 위치와 크기를 지정하여 액터 생성
    template <typename TActor, typename... TArgs>
        requires std::derived_from<TActor, AActor>
    TActor* SpawnActor(const FVector& Location, const FVector& Scale,
        TArgs &&...Args) {
        ULevel* Level = PersistentLevel;
        if (!Level) {
            return nullptr;
        }

        TActor* Actor = NewObject<TActor>(std::forward<TArgs>(Args)...);
        Actor->Initialize();

        if (Actor->GetRootComponent()) {
            FTransform Transform{};
            Transform.SetLocation(Location);
            Transform.SetScale3D(Scale);
            Actor->GetRootComponent()->SetRelativeTransform(Transform);
        }

        Level->AddActor(Actor);

        if (Level->IsActive()) {
            Actor->Register(this);
        }
        if (bHasBegunPlay) {
            Actor->BeginPlay();
        }
        return Actor;
    }

    // 액터가 속한 레벨에서 액터를 제외한다 (파괴하지는 않음). AActor::Release가 호출한다.
    void RemoveActor(AActor* Actor);
    void DestroyActor(AActor* Actor);

    //============================

private:
    // 월드의 유일한 레벨. 맵 파일 하나 = 레벨 하나이고, 새로 스폰되는 액터도 여기에 들어간다.
	ULevel* PersistentLevel = nullptr;
    FWorldContext* Context = nullptr;

    TUniquePtr<FScene> Scene;
    bool bInitialized = false;
    bool bHasBegunPlay = false;

    EWorldType WorldType;

    //Dummy
	TArray<AActor*> Dummy;
};