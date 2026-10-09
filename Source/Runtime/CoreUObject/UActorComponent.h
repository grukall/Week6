#pragma once

#include "UObject.h"
#include "Runtime/Engine/EngineBaseTypes.h"
#include "Runtime/Core/FName.h"

class UWorld;
class ULevel;
class AActor;

class UActorComponent : public UObject
{
	GENERATED_BODY()
	DECLARE_UCLASS(UActorComponent, UObject)

public:
	virtual void Initialize() override;
	virtual void Release() override;

	virtual void Serialize(FArchive& Archive) const override;
	virtual void Deserialize(const FArchive& Archive) override;

	AActor* GetActorOwner() const { return ActorOwner; }
	const FName& GetName() const { return Name; }
	UWorld* GetWorld() const { return World; }
	ULevel* GetLevel() const;
	
	void SetActorOwner(AActor* Owner) { ActorOwner = Owner; }
	void SetName(FName InName) { Name = InName; }
	

	[[nodiscard]] bool IsRegistered() const { return World != nullptr; }
	[[nodiscard]] bool HasBegunPlay() const { return bHasBegunPlay; }
	[[nodiscard]] bool IsTickEnabled() const { return bTickEnabled; }

	virtual void BeginPlay();
	virtual void Tick(float DeltaTime) {}
	virtual void EndPlay();

	virtual void Register(UWorld* InWorld);
	virtual void Unregister();

	void SetBatchIndex(int32 Index) { BatchIndex = Index; }
	int32 GetBatchIndex() const { return BatchIndex; }
	bool ShouldTick(ELevelTick TickType) const;

protected:
	AActor* ActorOwner = nullptr;
	UWorld* World = nullptr;
	bool bHasBegunPlay = false;
	bool bTickEnabled = false;
	bool bTickInEditor = false;
	int32 BatchIndex = -1;

private:
	FName Name;  // 기본값 None. AActor가 유일한 이름을 부여한다.
};
