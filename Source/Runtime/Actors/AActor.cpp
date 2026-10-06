#include "AActor.h"
#include "Runtime/Core/Log.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/CoreUObject/USceneComponent.h"
#include "Runtime/Engine/FArchive.h"
#include "Runtime/Engine/FScene.h"
#include "Runtime/CoreUObject/ULevel.h"

IMPLEMENT_UCLASS(AActor, UObject)

void AActor::Initialize()
{
	Super::Initialize();
	World = nullptr;
	bHasBegunPlay = false;
	bTickEnabled = false;
}

void AActor::Release()
{
	
	ULevel* RegisteredLevel = World ? World->GetPersistentLevel() : nullptr;
	if (bHasBegunPlay)
	{
		EndPlay();
	}

	if (World)
	{
		Unregister();
	}

	if (RegisteredLevel)
	{
		RegisteredLevel->RemoveActor(this);
	}

	while (!OwnedComponents.empty())
	{
		UActorComponent* Component = OwnedComponents.back();
		std::erase(OwnedComponents, Component);
		std::erase(AttachedComp, Component);

		if (RootComponent == Component)
		{
			RootComponent = nullptr;
		}

		DestroyObject(Component);
	}

	if (RootComponent)
	{
		USceneComponent* RemainingRoot = RootComponent;
		RootComponent = nullptr;
		DestroyObject(RemainingRoot);
	}

	Super::Release();
}

void AActor::DuplicateSubObjects()
{
	DuplicateRemap.clear();
	Super::DuplicateSubObjects();

	for (UActorComponent*& ActorComponent : OwnedComponents)
	{
		UActorComponent* NewActorComponent = (DuplicateAs(ActorComponent));
		NewActorComponent->ActorOwner = this;
		DuplicateRemap[ActorComponent] = NewActorComponent;
		ActorComponent = NewActorComponent;
	}

	RemapComponent(RootComponent);

	for (USceneComponent*& SceneComponent : AttachedComp)
	{
		RemapComponent(SceneComponent);
	}

	for (USceneComponent* SceneComponent : AttachedComp)
	{
		if (!SceneComponent || !SceneComponent->SceneOwner)
		{
			continue;
		}

		// 같은 액터 안의 부모만 여기서 연결한다.
		auto It = DuplicateRemap.find(SceneComponent->SceneOwner);
		if (It == DuplicateRemap.end())
		{
			continue;
		}

		USceneComponent* NewParent = It->second->Cast<USceneComponent>();
		if (NewParent)
		{
			SceneComponent->SceneOwner = NewParent;
			NewParent->Children.push_back(SceneComponent);
		}
	}

	World = nullptr;
	bHasBegunPlay = false;
}

const TMap<UActorComponent*, UActorComponent*>& AActor::GetDuplicateRemap() const
{
	return DuplicateRemap;
}

void AActor::RemapExternalAttachments(const TMap<UActorComponent*, UActorComponent*>& WorldRemap)
{
	for (USceneComponent* SceneComponent : AttachedComp)
	{
		if (!SceneComponent || !SceneComponent->SceneOwner)
		{
			continue;
		}

		auto It = WorldRemap.find(SceneComponent->SceneOwner);
		if (It == WorldRemap.end())
		{
			continue;
		}

		USceneComponent* NewParent = It->second->Cast<USceneComponent>();
		if (NewParent)
		{
			SceneComponent->SceneOwner = NewParent;
			NewParent->Children.push_back(SceneComponent);
		}
	}
}


void AActor::Serialize(FArchive& Archive) const
{
	Super::Serialize(Archive);

	if (RootComponent)
	{
		FArchive RootArchive{};
		RootComponent->Serialize(RootArchive);
		Archive.SetArchive("RootComponent", RootArchive);
	}
	else
	{
		Archive.SetNull("RootComponent");
	}
}

void AActor::Deserialize(const FArchive& Archive)
{
	Super::Deserialize(Archive);

	if (Archive.IsNull("RootComponent"))
	{
		if (RootComponent)
		{
			UE_LOG_WARN("[%s::Deserialize] RootComponent(%s)에 대한 직렬화 데이터가 "
				"누락되었습니다.",
				GetClass()->GetUClassName(),
				RootComponent->GetClass()->GetUClassName());
		}
		return;
	}

	FArchive RootComponentArchive = Archive.GetArchive("RootComponent");
	const FString& SavedTypeName = RootComponentArchive.GetString("Type");
	UClass* SavedClass = UClass::FindByName(SavedTypeName);

	if (SavedClass == nullptr)
	{
		UE_LOG_WARN("[%s::Deserialize] 알 수 없는 타입 %s",
			GetClass()->GetUClassName(), SavedTypeName);
		return;
	}

	if (RootComponent == nullptr)
	{
		CreateRootComponent(SavedClass);

		if (RootComponent == nullptr)
		{
			UE_LOG_WARN("[%s::Deserialize] RootComponent %s를 생성할 수 없습니다.",
				GetClass()->GetUClassName(), SavedTypeName);
			return;
		}
	}

	if (RootComponent->GetClass() != SavedClass)
	{
		UE_LOG_WARN("[%s::Deserialize] 기본 RootComponent (%s)와 저장된 타입 "
			"(%s)가 일치하지 않습니다.",
			GetClass()->GetUClassName(),
			RootComponent->GetClass()->GetUClassName(), SavedTypeName);
		return;
	}

	RootComponent->Deserialize(RootComponentArchive);
}

void AActor::CreateRootComponent(UClass* ClassType)
{
	if (RootComponent) { return; }

	UObject* Object = NewObject(ClassType);
	UActorComponent* ActorComponent = Object->Cast<UActorComponent>();

	if (!ActorComponent)
	{
		DestroyObject(Object);
		return;
	}
	SetRootComponent(ActorComponent);
	
	return;
}

void AActor::SetRootComponent(UActorComponent* Component)
{
	if (RootComponent)
	{
		throw EngineUtil::CreateError("이미 Root 컴포넌트가 있습니다.");
	}

	OwnedComponents.push_back(Component);
	Component->ActorOwner = this;
	Component->Initialize();
	USceneComponent* SceneComponent = Component->Cast<USceneComponent>();

	if (World)
	{
		Component->Register(*World);
	}

	if (bHasBegunPlay)
	{
		Component->BeginPlay();
	}

	if (!SceneComponent)
	{
		return;
	}

	RootComponent = SceneComponent;

	RootComponent->SetupAttachment(nullptr);
	AttachedComp.push_back(RootComponent);

	
}

void AActor::MarkComponentsTransformDirty()
{
	for (USceneComponent* Component : AttachedComp)
	{
		if (Component)
		{
			Component->OnTransformChanged();
		}
	}
}

void AActor::DeleteComponent(UActorComponent* Addcomp)
{
	for (auto It = OwnedComponents.begin(); It != OwnedComponents.end(); It++)
	{
		if (*It == Addcomp)
		{
			USceneComponent* CastSceneComponent = (*It)->Cast<USceneComponent>();
			if (CastSceneComponent)
			{
				// 루트 컴포넌트는 삭제못하게 막는다.
				if (RootComponent == CastSceneComponent)
				{
					return;
				}
				// 부모의 자식 목록에서 빠지는 것은 USceneComponent::Release가 처리한다.
				std::erase(AttachedComp, CastSceneComponent);
			}
			UActorComponent* TempComponet = *It;
			OwnedComponents.erase(It);
			DestroyObject(TempComponet);
			return;
		}
	}
}

void AActor::AddComponent(UActorComponent* Addcomp)
{
	if (Addcomp == nullptr)
	{
		return;
	}

	USceneComponent* CastSceneComponent = Addcomp->Cast<USceneComponent>();
	if (CastSceneComponent)
	{
		if (RootComponent == nullptr)
		{
			RootComponent = CastSceneComponent;
			CastSceneComponent->SetupAttachment(nullptr);
		}

		else if (CastSceneComponent->GetSceneOwner() == nullptr) // 들어온 컴포넌트가 부모가 없는 경우
		{
			CastSceneComponent->SetupAttachment(RootComponent);
		}

		if (CastSceneComponent->GetActorOwner() != this)
		{
			CastSceneComponent->SetActorOwner(this);
		}

		AttachedComp.push_back(CastSceneComponent);
	}

	OwnedComponents.push_back(Addcomp);
	Addcomp->Initialize();

	if (World)
	{
		Addcomp->Register(*World);
	}

	if (bHasBegunPlay)
	{
		Addcomp->BeginPlay();
	}
}

void AActor::Register(UWorld& InWorld)
{
	if (World == &InWorld)
	{
		return;
	}

	if (World)
	{
		Unregister();
	}

	World = &InWorld;
	for (UActorComponent* Component : OwnedComponents)
	{
		if (Component)
		{
			Component->Register(InWorld);
		}
	}
}

void AActor::BeginPlay() {
	if (!World || bHasBegunPlay)
	{
		return;
	}

	bHasBegunPlay = true;
	for (UActorComponent* Component : OwnedComponents)
	{
		if (Component)
		{
			Component->BeginPlay();
		}
	}
}

void AActor::Tick(float DeltaTime) {
	if (!bTickEnabled || !bHasBegunPlay)
	{
		return;
	}

	for (UActorComponent* Component : OwnedComponents)
	{
		if (Component && Component->IsTickEnabled())
		{
			Component->TickComponent(DeltaTime);
		}
	}
}

void AActor::EndPlay() {
	if (!bHasBegunPlay)
	{
		return;
	}

	for (auto It = OwnedComponents.rbegin(); It != OwnedComponents.rend(); ++It)
	{
		if (*It)
		{
			(*It)->EndPlay();
		}
	}
	bHasBegunPlay = false;
}

void AActor::Unregister() {
	if (bHasBegunPlay)
	{
		EndPlay();
	}

	if (!World)
	{
		return;
	}

	for (auto It = OwnedComponents.rbegin(); It != OwnedComponents.rend(); ++It)
	{
		if (*It)
		{
			(*It)->Unregister();
		}
	}
	World = nullptr;
}

void AActor::Destroy() {
	if (World)
	{
		World->DestroyActor(this);
		return;
	}
	DestroyObject(this);
}

USceneComponent* AActor::GetRootComponent() const
{
	return RootComponent;
}
const TArray<USceneComponent*>& AActor::GetAttachedComponents() const
{
	return AttachedComp;
}

FTransform AActor::GetTransform() const
{
	return RootComponent ? RootComponent->GetRelativeTransform() : FTransform{};
}

void AActor::SetTransform(const FTransform& NewTransform)
{
	if (RootComponent) RootComponent->SetRelativeTransform(NewTransform);
}

bool AActor::IsActorTickEnabled()
{
	return bTickEnabled;
}

bool AActor::IsActorEditorTickEnabled()
{
	return bTickInEditor;
}

UWorld* AActor::GetWorld() const
{ 
	return World;
}

ULevel* AActor::GetLevel() const
{
	if (!World)
	{
		return nullptr;
	}

	return World->GetPersistentLevel();
}

bool AActor::IsRegistered() const
{
	return World != nullptr;
}

bool AActor::HasBegunPlay() const
{
	return bHasBegunPlay;
}
