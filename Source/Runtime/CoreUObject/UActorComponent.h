#pragma once

#include "ThirdParty/Json/json.hpp"
#include "UObject.h"

class FScene;
class AActor;
class FArchive;
class UWorld;
class ULevel;

class UActorComponent : public UObject
{
	GENERATED_BODY()
	DECLARE_UCLASS(UActorComponent, UObject)
	friend class AActor;

public:
    virtual void Initialize() override;
    virtual void Release() override;
    virtual void DuplicateSubObjects() override;
    
    AActor* GetActorOwner() const { return ActorOwner; }
    void SetActorOwner(AActor* Owner) { ActorOwner = Owner; } //selectedacotor 한테 textcomponent 바로 붙여야해서 만듦

    UWorld* GetWorld() const;
    ULevel* GetLevel() const;

    virtual void Register(UWorld& InWorld);
    virtual void BeginPlay();
    virtual void TickComponent(float DeltaTime) {}
    virtual void EndPlay();
    virtual void Unregister();

    [[nodiscard]] bool IsRegistered() const;
    [[nodiscard]] bool HasBegunPlay() const { return bHasBegunPlay; }
    [[nodiscard]] bool IsTickEnabled() const { return bTickEnabled; }

	virtual void Serialize(FArchive& Archive) const override;
	virtual void Deserialize(const FArchive& Archive) override;
    
protected:
    UActorComponent() = default;

public:
    void SetBatchIndex(int32 Index) { BatchIndex = Index; }
    int32 GetBatchIndex() const { return BatchIndex; }

protected:
    AActor* ActorOwner = nullptr;
    UWorld* World = nullptr;
    bool bHasBegunPlay = false;
    bool bTickEnabled = false;
    int32 BatchIndex = -1;
};
