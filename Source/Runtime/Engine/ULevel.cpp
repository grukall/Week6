#include "ULevel.h"
#include "pch.h"
#include "Runtime/Core/Log.h"
#include "UWorld.h"
#include "Runtime/CoreUObject/UClass.h"

IMPLEMENT_UCLASS(ULevel, UObject)

void ULevel::Initialize()
{}

void ULevel::Release()
{
    while (!Actors.empty()) {
        AActor* Actor = Actors.back();
        Actors.pop_back();
        DestroyObject(Actor);
    }
}

void ULevel::Activate()
{
    if (bActive) {
        return;
    }

    if (!OwningWorld) {
        UE_LOG_ERROR("[ULevel::Activate] OwningWorld가 설정되지 않아 액터를 등록할 수 없습니다.");
        return;
    }

    for (AActor* Actor : Actors) {
        if (Actor) {
            Actor->Register(OwningWorld);
        }
    }

    if (OwningWorld->HasBegunPlay())
    {
        for (AActor* Actor : Actors) {
            if (Actor && !Actor->HasBegunPlay()) {
                Actor->BeginPlay();
            }
        }
    }

    bActive = true;
}

void ULevel::Deactivate()
{
    if (!bActive) {
        return;
    }

    for (auto It = Actors.rbegin(); It != Actors.rend(); ++It) {
        if (*It) {
            (*It)->Unregister();
        }
    }
    bActive = false;
}

void ULevel::Tick(float DeltaTime)
{
    for (AActor* Actor : Actors) {
        if (Actor) {
            Actor->Tick(DeltaTime);
        }
    }
}

void ULevel::Serialize(FArchive& Archive) const
{
    Super::Serialize(Archive);

    TArray<FArchive> ActorArchives;

    for (const auto &Item : Actors) {
    if (!Item) {
        continue;
    }

    FArchive ItemArchive;
    Item->Serialize(ItemArchive);
    ActorArchives.push_back(ItemArchive);
    }

    Archive.SetArchiveArray("Actors", ActorArchives);
}

void ULevel::Deserialize(const FArchive& Archive)
{
	Super::Deserialize(Archive);

    if (Archive.IsNull("Actors"))
    {
        // Actor 목록이 비어있음
        return;
    }
    
    TArray<FArchive> ActorArchives = Archive.GetArchiveArray("Actors");
    
    for (const auto &Item : ActorArchives) {
    UClass *ClassType = UClass::FindByName(Item.GetString("Type"));
    if (ClassType == nullptr) {
        continue;
    }
    
    AActor *Actor = OwningWorld->SpawnActor(ClassType);
    if (!Actor) {
        continue;
    }
    Actor->Deserialize(Item);
    
    }
}

void ULevel::AddActor(AActor* Actor)
{
    if (!Actor) {
        return;
    }

    Actors.push_back(Actor);
    Actor->OwningLevel = this;
}

void ULevel::RemoveActor(AActor* Actor)
{
    if (!Actor) {
        return;
    }

    std::erase(Actors, Actor);
    if (Actor->OwningLevel == this) {
        Actor->OwningLevel = nullptr;
    }
}
