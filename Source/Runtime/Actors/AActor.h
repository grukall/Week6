#pragma once

#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/CoreUObject/USceneComponent.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Engine/EngineBaseTypes.h"
#include "Runtime/Core/FName.h"
#include "Runtime/Core/FGuid.h"
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
	explicit AActor() = default;

	virtual void Serialize(FArchive& Archive) const override;
	virtual void Deserialize(const FArchive& Archive) override;

public:
	void Initialize() override;
	void Release() override;
	[[nodiscard]] ULevel* GetLevel() const { return OwningLevel; }
	[[nodiscard]] UWorld* GetWorld() const;

	// 액터를 식별하는 값들 (런타임 약참조는 FUObjectArray의 슬롯/시리얼 번호가 맡는다)
	// 이름: 레벨 안에서 유일. 저장 파일과 에디터 표시에 쓴다. 레벨에 추가될 때 ULevel이 부여한다.
	[[nodiscard]] const FName& GetName() const { return Name; }
	// Guid: 저장 파일에 보존되고 에디터 월드와 PIE 월드의 같은 액터를 서로 찾는 키로 쓴다.
	[[nodiscard]] const FGuid& GetGuid() const { return Guid; }

	void CreateRootComponent(UClass* ClassType);

	void SetRootComponent(UActorComponent* Component);
	USceneComponent* GetRootComponent() const { return RootComponent; }
	const TArray<USceneComponent*>& GetAttachedComponents() const { return AttachedComp; }


	FTransform GetTransform() const { return RootComponent ? RootComponent->GetRelativeTransform() : FTransform{}; }
	void SetTransform(const FTransform& NewTransform) { if (RootComponent) RootComponent->SetRelativeTransform(NewTransform); }

	//하위 컴포넌트 월드 Tranform도 바뀐다.
	void MarkComponentsTransformDirty();

	const TMap<UActorComponent*, UActorComponent*>& GetDuplicateRemap() const;
	void RemapExternalAttachments(const TMap<UActorComponent*, UActorComponent*>& WorldRemap);

	void AddComponent(UActorComponent* Addcomp);
	void DeleteComponent(UActorComponent* Addcomp);
	virtual void Register(UWorld *World);
	virtual void BeginPlay();
	virtual void Tick(float DeltaTime, ELevelTick eTickType);
	virtual void EndPlay();
	virtual void Unregister();

	[[nodiscard]] bool IsRegistered() const { return RegisteredWorld != nullptr; }
	[[nodiscard]] bool HasBegunPlay() const { return bHasBegunPlay; }
	bool IsTickEnabled() const { return bTickEnabled; }
	bool IsShouldTickIfViewportsOnly() const { return bShouldTickIfViewportsOnly; }
	bool ShouldTick(ELevelTick TickType) const;

	void Destroy();

protected:
	USceneComponent* RootComponent = nullptr;
	TArray<USceneComponent*> AttachedComp;
	TArray<UActorComponent*> OwnedComponents;
	TMap<UActorComponent*, UActorComponent*> DuplicateRemap;
	bool bTickEnabled = false;
	bool bRegistered = false;

	//에디터에서만 Tick해야 하는 액터인 경우 true로 설정
	bool bShouldTickIfViewportsOnly = false;

private:
	ULevel* OwningLevel = nullptr;     // 소속 레벨 (ULevel::AddActor/RemoveActor가 관리)
	UWorld* RegisteredWorld = nullptr; // Register된 월드 (컴포넌트가 FScene에 연결된 상태)
	bool bHasBegunPlay = false;

	FName Name;  // 기본값 None. ULevel::AddActor가 유일한 이름을 부여한다.
	FGuid Guid;  // 기본값 무효. ULevel::AddActor가 새로 발급하거나, 파일에서 읽은 값을 보존한다.
};
