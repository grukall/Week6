#include "Runtime/CoreUObject/ULevel.h"
#include "Runtime/Engine/FArchive.h"
#include "Runtime/Actors/AActor.h"
#include "Runtime/CoreUObject/UWorld.h"


IMPLEMENT_UCLASS_NO_COPY(ULevel, UObject)

void ULevel::RemoveActor(AActor* Actor) { std::erase(Actors, Actor); }
void ULevel::AddActor(AActor* Actor) { Actors.push_back(Actor); }

void ULevel::Initialize(UWorld* InWorld)
{
    OwningWorld = InWorld;
}

void ULevel::Release()
{
    while (!Actors.empty()) {
        AActor* Actor = Actors.back();
        Actors.pop_back();
        DestroyObject(Actor);
    }
    OwningWorld = nullptr;
}

[[nodiscard]] const TArray<AActor*>& ULevel::GetActors() const
{
    return Actors;
}

void ULevel::Serialize(FArchive& Archive) const {
    Super::Serialize(Archive);

    TArray<FArchive> ActorArchives;

    for (const auto& Item : Actors) {
        if (!Item) {
            continue;
        }

        FArchive ItemArchive;
        Item->Serialize(ItemArchive);
        ActorArchives.push_back(ItemArchive);
    }

    Archive.SetArchiveArray("Actors", ActorArchives);
}

void ULevel::Deserialize(const FArchive& Archive) {
    Super::Deserialize(Archive);

    if (Archive.IsNull("Actors")) {
        // Actor 목록이 비어있음
        return;
    }

    TArray<FArchive> ActorArchives = Archive.GetArchiveArray("Actors");

    for (const auto& Item : ActorArchives) {
        UClass* ClassType = UClass::FindByName(Item.GetString("Type"));
        if (ClassType == nullptr) {
            continue;
        }

        AActor* Actor = OwningWorld->SpawnActor(ClassType);
        if (!Actor) {
            continue;
        }
        Actor->Deserialize(Item);

        if (OwningWorld->IsActive()) {
            Actor->Register(*OwningWorld);
        }
        if (OwningWorld->HasBegunPlay()) {
            Actor->BeginPlay();
        }
    }
}




//void ULevel::Tick(float DeltaTime) {
//    if (bHasBegunPlay) {
//        for (AActor* Actor : Actors) {
//            if (Actor) {
//                Actor->Tick(DeltaTime);
//            }
//        }
//    }
//}