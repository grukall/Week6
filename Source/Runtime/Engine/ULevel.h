#pragma once

#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/Core/TMap.h"
#include "Runtime/Core/FName.h"
#include "Runtime/Core/FGuid.h"
#include "Runtime/Core/FString.h"
#include "Runtime/Actors/AActor.h"
#include "EngineBaseTypes.h"
#include "FArchive.h"

class UWorld;

class ULevel : public UObject
{
	GENERATED_BODY()
	DECLARE_UCLASS(ULevel, UObject)

public:
	void Initialize() override;
	void Release() override;
	void Activate();
	void Deactivate();
	void Tick(float DeltaTime, ELevelTick eTickType);

	virtual void Serialize(FArchive &Archive) const override;
	virtual void Deserialize(const FArchive &Archive) override;

	// 이 레벨이 속한 월드. 액터/컴포넌트 등록(Register)은 이 월드를 기준으로 한다.
	void SetOwningWorld(UWorld* InWorld) { OwningWorld = InWorld; }
	[[nodiscard]] UWorld* GetOwningWorld() const { return OwningWorld; }

	[[nodiscard]] bool IsActive() const { return bActive; }
	[[nodiscard]] virtual const TArray<AActor*> *GetActors() const { return &Actors; }
	void AddActor(AActor* Actor);
	void RemoveActor(AActor* Actor);

	// 이름은 레벨 안에서 유일하고, Guid는 이 레벨의 액터 사이에서 유일하다.
	[[nodiscard]] AActor* FindActorByName(const FName& Name) const;
	[[nodiscard]] AActor* FindActorByGuid(const FGuid& Guid) const;

	bool SetActorName(AActor* Actor, const FName& NewName);
	bool SetActorGuid(AActor* Actor, const FGuid& NewGuid);

private:
	FName MakeUniqueActorName(const AActor* Actor);

	TArray<AActor*> Actors;
	TMap<FName, AActor*> ActorsByName;
	TMap<FGuid, AActor*> ActorsByGuid;
	TMap<FString, int32> NextNameNumber; // 클래스 이름별 다음 번호

	UWorld* OwningWorld = nullptr;
	bool bActive = false;
};