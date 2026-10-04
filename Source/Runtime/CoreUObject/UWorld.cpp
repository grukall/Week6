#include "Runtime/CoreUObject/UWorld.h"
#include "Runtime/CoreUObject/ULevel.h"
#include "FUObjectArray.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/Engine/FScene.h"

IMPLEMENT_UCLASS_NO_COPY(UWorld, UObject)


void UWorld::InitializeActorsForPlay()
{
    BeginPlay();
}

UWorld* UWorld::DuplicateWorldForPIE(UWorld* InWorld)
{
    UWorld* PIEWorld = NewObject<UWorld>();
    PIEWorld->Initialize();
    PIEWorld->SetWorldType(EWorldType::PIE);
    for (AActor* Actor : InWorld->PersistentLevel->GetActors()) 
    {
        AActor* DuplicateActor = DuplicateAs(Actor);   
        PIEWorld->PersistentLevel->AddActor(DuplicateActor);
    }
    PIEWorld->Activate();
    PIEWorld->Scene->GetSceneBVH().Build(PIEWorld->Scene->GetRenderComponents());
    return (PIEWorld);
}

void UWorld::CleanupWorld()
{
    Release();
}

void UWorld::SetPersistentLevel(ULevel* InLevel)
{
    if (PersistentLevel)
    {
        DestroyObject(PersistentLevel);
    }
    PersistentLevel = InLevel;
    InLevel->Initialize(this);
}

void UWorld::SetWorldType(EWorldType InWorldType)
{
    WorldType = InWorldType;
}

ULevel* UWorld::GetPersistentLevel() const
{
	return (PersistentLevel);
}

bool UWorld::IsPlayInEditor() const
{
	return (WorldType == EWorldType::PIE);
}

bool UWorld::IsEditorWorld() const
{
	return (WorldType == EWorldType::Editor);
}

void UWorld::DestroyActor(AActor* Actor)
{
	if (Actor == nullptr)
		return;

	PersistentLevel->RemoveActor(Actor);
	DestroyObject(Actor);
}

void UWorld::BeginPlay() {
    if (!bActive || bHasBegunPlay) {
        return;
    }

    bHasBegunPlay = true;
    for (AActor* Actor : PersistentLevel->GetActors()) {
        if (Actor) {
            Actor->BeginPlay();
        }
    }
}

void UWorld::EndPlay() {
    if (!bHasBegunPlay) {
        return;
    }

    const TArray<AActor*>& Actors = PersistentLevel->GetActors();
    for (auto It = Actors.rbegin(); It != Actors.rend(); ++It) {
        if (*It) {
            (*It)->EndPlay();
        }
    }
    bHasBegunPlay = false;
}

void UWorld::Activate() {
    if (bActive) {
        return;
    }

    for (AActor* Actor : PersistentLevel->GetActors()) {
        if (Actor) {
            Actor->Register(*this);
        }
    }
    bActive = true;
}

void UWorld::Deactivate() {
    if (!bActive) {
        return;
    }
    if (bHasBegunPlay) {
        EndPlay();
        bHasBegunPlay = false;
    }

    const TArray<AActor*>& Actors = PersistentLevel->GetActors();
    for (auto It = Actors.rbegin(); It != Actors.rend(); ++It) {
        if (*It) {
            (*It)->Unregister();
        }
    }
    bActive = false;
}

void UWorld::Serialize(FArchive& Archive) const
{
    Super::Serialize(Archive);
    PersistentLevel->Serialize(Archive);
}

void UWorld::Deserialize(const FArchive& Archive)
{
    Super::Deserialize(Archive);
    PersistentLevel->Deserialize(Archive);
}

void UWorld::Release() {

    if (bHasBegunPlay) {
        EndPlay();
    }
    if (bActive) {
        Deactivate();
    }
    DestroyObject(PersistentLevel);
    PersistentLevel = nullptr;
    Scene->Release();
    delete (Scene);
    Scene = nullptr;
    bInitialized = false;
    Super::Release();
}

void UWorld::CreateWorld(EWorldType InWorldType)
{
    Initialize();
    WorldType = InWorldType;
}

void UWorld::Initialize() {
    if (bInitialized) {
        return;
    }
    Super::Initialize();
    Scene = new FScene();
    Scene->SetRenderResourceLibrary(&FRenderResourceLibrary::Get());
    bInitialized = true;
    PersistentLevel = NewObject<ULevel>();
    PersistentLevel->Initialize(this);
}



void UWorld::Tick(float DeltaTime) {
    if (WorldType == EWorldType::Editor)
    {
        for (AActor* Actor : PersistentLevel->GetActors())
        {
            if (Actor && Actor->IsActorEditorTickEnabled() && Actor->IsActorTickEnabled())
            {
                Actor->Tick(DeltaTime);
            }
        }
    }
    else if (WorldType == EWorldType::PIE)
    {
        for (AActor* Actor : PersistentLevel->GetActors())
        {
            if (Actor && Actor->IsActorTickEnabled())
            {
                Actor->Tick(DeltaTime);
            }
        }
    }
}

AActor* UWorld::SpawnActor(UClass* ClassType) {
    UObject* Object = NewObject(ClassType);
    AActor* Actor = Object->Cast<AActor>();
    if (!Actor) {
        DestroyObject(Object);
        return nullptr;
    }
    Actor->Initialize();
    Actor->Register(*this);

    PersistentLevel->AddActor(Actor);

    return Actor;
}