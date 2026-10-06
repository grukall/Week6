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

void ULevel::Tick(float DeltaTime, ELevelTick eTickType)
{
    for (AActor* Actor : Actors)
    {
        if (!Actor) continue;

        if (Actor->ShouldTick(eTickType))
            Actor->Tick(DeltaTime, eTickType);

        for (UActorComponent* C : Actor->GetOwnedComponents())
            if (C && C->ShouldTick(eTickType))
                C->Tick(DeltaTime);
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
    
    TArray<AActor*> LoadedActors(ActorArchives.size(), nullptr);
    TMap<FString, AActor*> LoadedActorsByGuid;

    for (size_t i = 0; i < ActorArchives.size(); ++i) {
        const FArchive& Item = ActorArchives[i];
        UClass *ClassType = UClass::FindByName(Item.GetString("Type"));
        if (ClassType == nullptr) {
            continue;
        }

        AActor *Actor = OwningWorld->SpawnActor(ClassType);
        if (!Actor) {
            continue;
        }
        Actor->Deserialize(Item);

        LoadedActors[i] = Actor;
        if (!Item.IsNull("Guid")) {
            LoadedActorsByGuid[Item.GetString("Guid")] = Actor;
        }
    }

    for (size_t i = 0; i < ActorArchives.size(); ++i) {
        if (LoadedActors[i]) {
            LoadedActors[i]->RestoreExternalAttachments(ActorArchives[i], LoadedActorsByGuid);
        }
    }
}

// Actor->OwningLevel을 이 레벨로 설정한다 (AActor가 friend로 허용).
void ULevel::AddActor(AActor* Actor)
{
    if (!Actor) {
        return;
    }

    Actors.push_back(Actor);
    Actor->OwningLevel = this;

    // 이름이 없거나 이 레벨에서 이미 쓰이고 있으면 새로 부여한다.
    if (Actor->Name.IsNone() || ActorsByName.contains(Actor->Name)) {
        Actor->Name = MakeUniqueActorName(Actor);
    }
    ActorsByName[Actor->Name] = Actor;

    // Guid가 없거나 이 레벨에서 이미 쓰이고 있으면 새로 발급한다.
    if (!Actor->Guid.IsValid() || ActorsByGuid.contains(Actor->Guid)) {
        Actor->Guid = FGuid::NewGuid();
    }
    ActorsByGuid[Actor->Guid] = Actor;
}

// 목록에서 제외하고 Actor->OwningLevel을 해제한다 (파괴하지는 않음).
void ULevel::RemoveActor(AActor* Actor)
{
    if (!Actor) {
        return;
    }

    std::erase(Actors, Actor);

    if (auto It = ActorsByName.find(Actor->Name); It != ActorsByName.end() && It->second == Actor) {
        ActorsByName.erase(It);
    }
    if (auto It = ActorsByGuid.find(Actor->Guid); It != ActorsByGuid.end() && It->second == Actor) {
        ActorsByGuid.erase(It);
    }

    if (Actor->OwningLevel == this) {
        Actor->OwningLevel = nullptr;
    }
}

AActor* ULevel::FindActorByName(const FName& Name) const
{
    const auto It = ActorsByName.find(Name);
    return It != ActorsByName.end() ? It->second : nullptr;
}

AActor* ULevel::FindActorByGuid(const FGuid& Guid) const
{
    const auto It = ActorsByGuid.find(Guid);
    return It != ActorsByGuid.end() ? It->second : nullptr;
}

// 액터의 이름을 바꾼다. 이미 다른 액터가 쓰는 이름이면 변경하지 않고 false를 반환한다.
bool ULevel::SetActorName(AActor* Actor, const FName& NewName)
{
    if (!Actor || Actor->OwningLevel != this || NewName.IsNone()) {
        return false;
    }
    if (Actor->Name == NewName) {
        return true;
    }

    const auto Existing = ActorsByName.find(NewName);
    if (Existing != ActorsByName.end() && Existing->second != Actor) {
        return false;
    }

    if (auto It = ActorsByName.find(Actor->Name); It != ActorsByName.end() && It->second == Actor) {
        ActorsByName.erase(It);
    }
    Actor->Name = NewName;
    ActorsByName[NewName] = Actor;
    return true;
}

// 액터의 Guid를 바꾼다. 무효하거나 이미 다른 액터가 쓰는 Guid면 변경하지 않고 false를 반환한다.
bool ULevel::SetActorGuid(AActor* Actor, const FGuid& NewGuid)
{
    if (!Actor || Actor->OwningLevel != this || !NewGuid.IsValid()) {
        return false;
    }
    if (Actor->Guid == NewGuid) {
        return true;
    }

    const auto Existing = ActorsByGuid.find(NewGuid);
    if (Existing != ActorsByGuid.end() && Existing->second != Actor) {
        return false;
    }

    if (auto It = ActorsByGuid.find(Actor->Guid); It != ActorsByGuid.end() && It->second == Actor) {
        ActorsByGuid.erase(It);
    }
    Actor->Guid = NewGuid;
    ActorsByGuid[NewGuid] = Actor;
    return true;
}

// "클래스이름_번호" 형식의 아직 쓰이지 않은 이름을 만든다.
FName ULevel::MakeUniqueActorName(const AActor* Actor)
{
    // "AAppleNormalActor" -> "AppleNormalActor" (UE처럼 접두 'A' 제거)
    FString Base = Actor->GetClass()->GetUClassName();
    if (Base.size() > 1 && Base[0] == 'A' && Base[1] >= 'A' && Base[1] <= 'Z') {
        Base.erase(0, 1);
    }

    int32& Number = NextNameNumber[Base];
    while (true) {
        const FName Candidate(Base + "_" + std::to_string(Number++));
        if (!ActorsByName.contains(Candidate)) {
            return Candidate;
        }
    }
}

