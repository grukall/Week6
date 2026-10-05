#pragma once

#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/CoreUObject/USceneComponent.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"

#include <type_traits>
#include <concepts>

class FScene;
class ULevel;
class UWorld;
class USceneComponent;

class AActor : public UObject
{
	DECLARE_UCLASS(AActor, UObject)
	GENERATED_BODY()

	friend class ULevel;

protected:
	USceneComponent* RootComponent = nullptr;
	TArray<USceneComponent*> AttachedComp;
	TMap<USceneComponent*, USceneComponent*> DuplicateRemap;
	bool bTickEnabled = false;
	bool bTickInEditor = false;

	explicit AActor() = default;

	virtual void Serialize(FArchive& Archive) const override;
	virtual void Deserialize(const FArchive& Archive) override;

public:
	
	void DuplicateSubObjects() override;
	void Initialize() override;
	void Release() override;
	UWorld* GetWorld() const;

	void CreateRootComponent(UClass* ClassType);

	void SetRootComponent(USceneComponent* Component);
	USceneComponent* GetRootComponent() const;
	const TArray<USceneComponent*>& GetAttachedComponents() const;


	FTransform GetTransform() const;
	void SetTransform(const FTransform& NewTransform);

	bool IsActorTickEnabled();
	bool IsActorEditorTickEnabled();

	//하위 컴포넌트 월드 Tranform도 바뀐다.
	void MarkComponentsTransformDirty();

	void AddComponent(USceneComponent* Addcomp);
	virtual void Register(UWorld& InWorld);
	virtual void BeginPlay();
	virtual void Tick(float DeltaTime);
	virtual void EndPlay();
	virtual void Unregister();

	[[nodiscard]] bool IsRegistered() const;
	[[nodiscard]] bool HasBegunPlay() const;

	void Destroy();

private:
	UWorld* Owner = nullptr; // SpawnActor될 때 설정됨
	bool bHasBegunPlay = false;

protected:
	template<typename T>
	void RemapComponent(T*& Ptr)
	{
		auto It = DuplicateRemap.find(Ptr);
		if (It != DuplicateRemap.end())
		{
			Ptr = It->second->template Cast<T>();
		}
	}
};
