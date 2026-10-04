#pragma once

#include "Runtime/CoreUObject/UObject.h"
#include "Runtime/Core/TArray.h"
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

private:
	TArray<AActor*> Actors;

	UWorld* OwningWorld = nullptr;
	bool bActive = false;
};