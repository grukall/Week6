#pragma once

#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/CoreUObject/ULevel.h"
#include "Runtime/Actors/AActor.h"


class FScene;

enum EWorldType
{
    None,
    Editor,
    EditorPreview,
    PIE,
    Game,
};

class UWorld final : public UObject {
    DECLARE_UCLASS_NO_COPY(UWorld, UObject)
    GENERATED_BODY()

public:
    void Initialize() override;
    void Release() override;
    virtual void Serialize(FArchive& Archive) const override;
    virtual void Deserialize(const FArchive& Archive) override;

    void CreateWorld(EWorldType InWorldType);

    void InitializeActorsForPlay();
    static UWorld* DuplicateWorldForPIE(UWorld* InWorld);


    void SetPersistentLevel(ULevel* InLevel);
    void SetWorldType(EWorldType InWorldType);


    ULevel* GetPersistentLevel() const;


    bool IsPlayInEditor() const;
    bool IsEditorWorld() const;

    void EndPlay();
    void BeginPlay();
    void Activate();
    void Deactivate();
    void CleanupWorld();
    

    [[nodiscard]] bool IsActive() const { return bActive; }
    [[nodiscard]] bool HasBegunPlay() const { return bHasBegunPlay; }

    AActor* SpawnActor(UClass* ClassType);
    void DestroyActor(AActor* Actor);


    // GameTechLab에서는 Sub Level은 고려하지 않습니다.
    FScene* Scene;
    EWorldType WorldType = None;

    void Tick(float DeltaTime);

    // 위치와 크기를 지정하여 액터 생성
    template <typename TActor, typename... TArgs>
        requires std::derived_from<TActor, AActor>
    TActor* SpawnActor(const FVector& Location, const FVector& Scale,
        TArgs &&...Args) {
        TActor* Actor = NewObject<TActor>(std::forward<TArgs>(Args)...);
        Actor->Initialize();

        if (Actor->GetRootComponent()) {
            FTransform Transform{};
            Transform.SetLocation(Location);
            Transform.SetScale3D(Scale);
            Actor->GetRootComponent()->SetRelativeTransform(Transform);
        }

        PersistentLevel->AddActor(Actor);

        if (bActive) {
            Actor->Register(*this);
        }
        if (bHasBegunPlay) {
            Actor->BeginPlay();
        }
        return Actor;
    }

    // 기본 위치와 크기로 액터 생성
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

protected:
    UWorld() = default;

private:
    bool bActive = false;
    bool bHasBegunPlay = false;
    bool bInitialized = false;

    ULevel* PersistentLevel = nullptr;

};
