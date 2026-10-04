#pragma once

#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/CoreUObject/USceneComponent.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include <type_traits>
#include <concepts>

class UWorld;
class ULevel;

class AActor : public UObject
{
	DECLARE_UCLASS(AActor, UObject)
	GENERATED_BODY()

	friend class ULevel;

protected:
	USceneComponent* RootComponent = nullptr;
	TArray<USceneComponent*> AttachedComp;
	bool bTickEnabled = false;

	explicit AActor() = default;

	virtual void Serialize(FArchive& Archive) const override;
	virtual void Deserialize(const FArchive& Archive) override;

public:
	void Initialize() override;
	void Release() override;
	// 이 액터가 속한 레벨 (로드/스폰 시 ULevel::AddActor가 설정. 등록 여부와 무관)
	[[nodiscard]] ULevel* GetLevel() const { return OwningLevel; }
	// 소속 레벨이 속한 월드. 레벨이 없으면 nullptr
	[[nodiscard]] UWorld* GetWorld() const;

	void CreateRootComponent(UClass* ClassType);

	void SetRootComponent(USceneComponent* Component);
	USceneComponent* GetRootComponent() const { return RootComponent; }
	const TArray<USceneComponent*>& GetAttachedComponents() const { return AttachedComp; }


	FTransform GetTransform() const { return RootComponent ? RootComponent->GetRelativeTransform() : FTransform{}; }
	void SetTransform(const FTransform& NewTransform) { if (RootComponent) RootComponent->SetRelativeTransform(NewTransform); }

	//하위 컴포넌트 월드 Tranform도 바뀐다.
	void MarkComponentsTransformDirty();

	void AddComponent(USceneComponent* Addcomp);
	virtual void Register(UWorld *World);
	virtual void BeginPlay();
	virtual void Tick(float DeltaTime);
	virtual void EndPlay();
	virtual void Unregister();

	[[nodiscard]] bool IsRegistered() const { return RegisteredWorld != nullptr; }
	[[nodiscard]] bool HasBegunPlay() const { return bHasBegunPlay; }

	void Destroy();

private:
	ULevel* OwningLevel = nullptr;     // 소속 레벨 (ULevel::AddActor/RemoveActor가 관리)
	UWorld* RegisteredWorld = nullptr; // Register된 월드 (컴포넌트가 FScene에 연결된 상태)
	bool bHasBegunPlay = false;
};
