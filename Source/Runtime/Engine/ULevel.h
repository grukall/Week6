#pragma once

#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/Core/TArray.h"
#include "Runtime/Core/TMap.h"
#include "Runtime/Core/FName.h"
#include "Runtime/Core/FGuid.h"
#include "Runtime/Core/FString.h"
#include "Runtime/Actors/AActor.h"
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
	void Tick(float DeltaTime);

	virtual void Serialize(FArchive &Archive) const override;
	virtual void Deserialize(const FArchive &Archive) override;

	// 이 레벨이 속한 월드. 액터/컴포넌트 등록(Register)은 이 월드를 기준으로 한다.
	void SetOwningWorld(UWorld* InWorld) { OwningWorld = InWorld; }
	[[nodiscard]] UWorld* GetOwningWorld() const { return OwningWorld; }

	[[nodiscard]] bool IsActive() const { return bActive; }
	[[nodiscard]] virtual const TArray<AActor*> *GetActors() const { return &Actors; }
	// Actor->OwningLevel을 이 레벨로 설정한다 (AActor가 friend로 허용).
	void AddActor(AActor* Actor);
	// 목록에서 제외하고 Actor->OwningLevel을 해제한다 (파괴하지는 않음).
	void RemoveActor(AActor* Actor);

	// 이름은 레벨 안에서 유일하고, Guid는 이 레벨의 액터 사이에서 유일하다.
	[[nodiscard]] AActor* FindActorByName(const FName& Name) const;
	[[nodiscard]] AActor* FindActorByGuid(const FGuid& Guid) const;

	// 액터의 이름을 바꾼다. 이미 다른 액터가 쓰는 이름이면 변경하지 않고 false를 반환한다.
	bool SetActorName(AActor* Actor, const FName& NewName);
	// 액터의 Guid를 바꾼다. 무효하거나 이미 다른 액터가 쓰는 Guid면 변경하지 않고 false를 반환한다.
	bool SetActorGuid(AActor* Actor, const FGuid& NewGuid);

private:
	// "클래스이름_번호" 형식의 아직 쓰이지 않은 이름을 만든다.
	FName MakeUniqueActorName(const AActor* Actor);

	TArray<AActor*> Actors;
	TMap<FName, AActor*> ActorsByName;
	TMap<FGuid, AActor*> ActorsByGuid;
	TMap<FString, int32> NextNameNumber; // 클래스 이름별 다음 번호

	UWorld* OwningWorld = nullptr;
	bool bActive = false;
};