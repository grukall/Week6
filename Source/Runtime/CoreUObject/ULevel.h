#pragma once

#include "Runtime/CoreUObject/UObject.h"

class AActor;
class UWorld;

class ULevel final : public UObject {
    DECLARE_UCLASS(ULevel, UObject)
    GENERATED_BODY()

public:
    void Tick(float DeltaTime);
    // 액터 목록 반환
    [[nodiscard]] const TArray<AActor*>& GetActors() const;

    void Initialize(UWorld* InWorld);
    void Release();
    virtual void Serialize(FArchive& Archive) const override;
    virtual void Deserialize(const FArchive& Archive) override;
    void RemoveActor(AActor* Actor);
    void AddActor(AActor* Actor);
    //AActor* SpawnActor(UClass* ClassType);

private:
    UWorld* OwningWorld = nullptr;
    TArray<AActor*> Actors;

};