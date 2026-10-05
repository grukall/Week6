#include "pch.h"
#include "UWorld.h"
#include "ULevel.h"
#include "FScene.h"
#include <Runtime/Core/TArray.h>

IMPLEMENT_UCLASS(UWorld, UObject)

void UWorld::Initialize(EWorldType _WorldType)
{
	if (bInitialized) {
		return;
	}
	bInitialized = true;

	PersistentLevel = NewObject<ULevel>();
	PersistentLevel->Initialize();
	PersistentLevel->SetOwningWorld(this);

	Scene = MakeUnique<FScene>();

	WorldType = _WorldType;
}

void UWorld::Release()
{
	if (bHasBegunPlay) {
		EndPlay();
	}

	// 레벨을 파괴하는 동안 액터가 RemoveActor()를 호출할 수 있으므로,
	// 파괴된 레벨 포인터가 남지 않게 멤버를 먼저 비운다.
	ULevel* LevelToDestroy = PersistentLevel;
	PersistentLevel = nullptr;

	if (LevelToDestroy) {
		// DestroyObject가 내부에서 Release()를 호출한다.
		DestroyObject(LevelToDestroy);
	}

	// 레벨(액터, 컴포넌트)이 FScene에서 모두 빠진 뒤에 FScene을 해제한다.
	Scene.Reset();

	bInitialized = false;
}

void UWorld::RemoveActor(AActor* Actor)
{
	if (!Actor) {
		return;
	}

	// 액터가 소속된 레벨에서만 제외한다 (등록 여부와 무관).
	if (ULevel* Level = Actor->GetLevel()) {
		Level->RemoveActor(Actor);
	}
}

void UWorld::BeginPlay()
{
	if (bHasBegunPlay) {
		return;
	}

	bHasBegunPlay = true;

	if (PersistentLevel)
	{
		const TArray<AActor*> *Actors = PersistentLevel->GetActors();
		if (Actors)
		{
			for (AActor* Actor : *Actors)
			{
				Actor->BeginPlay();
			}
		}
	}
}

void UWorld::EndPlay()
{
	if (!bHasBegunPlay) {
		return;
	}

	if (PersistentLevel)
	{
		const TArray<AActor*>* Actors = PersistentLevel->GetActors();
		if (Actors)
		{
			for (AActor* Actor : *Actors)
			{
				Actor->EndPlay();
			}
		}
	}

	bHasBegunPlay = false;
}

void UWorld::Serialize(FArchive& Archive) const
{
	Super::Serialize(Archive);

	FArchive PersistentLevelArchive{};
	PersistentLevel->Serialize(PersistentLevelArchive);
	Archive.SetArchive("PersistentLevel", PersistentLevelArchive);
}

void UWorld::Deserialize(const FArchive & Archive)
{
	Super::Deserialize(Archive);

	FArchive PersistentLevelArchive = Archive.GetArchive("PersistentLevel");
	PersistentLevel->Deserialize(PersistentLevelArchive);
}

AActor* UWorld::SpawnActor(UClass* ClassType) {
	UObject* Object = NewObject(ClassType);
	AActor* Actor = Object->Cast<AActor>();
	if (!Actor) {
		DestroyObject(Object);
		return nullptr;
	}
	Actor->Initialize();

	PersistentLevel->AddActor(Actor);

	if (PersistentLevel->IsActive()) {
		Actor->Register(this);
	}
	if (bHasBegunPlay) {
		Actor->BeginPlay();
	}

	return Actor;
}
void UWorld::DestroyActor(AActor* Actor)
{
	if (Actor == nullptr)
		return;

	// 레벨 목록에서는 AActor::Release가 OwningLevel->RemoveActor로 제외한다.
	DestroyObject(Actor);
}


void UWorld::Tick(float DeltaTime, ELevelTick eTickType)
{
	//TODO : World 시간이라는 개념이 생기면 여기에 시간 갱신

	if (eTickType == ELevelTick::LEVELTICK_TimeOnly)
	{
		return;
	}

	if (PersistentLevel) {
		PersistentLevel->Tick(DeltaTime, eTickType);
	}
}