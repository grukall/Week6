#include "AActor.h"
#include "Runtime/Core/Log.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/CoreUObject/USceneComponent.h"
#include "Runtime/CoreUObject/UProjectileMovementComponent.h"
#include "Runtime/Engine/FArchive.h"
#include "Runtime/Engine/ULevel.h"
#include "Runtime/Engine/UWorld.h"

IMPLEMENT_UCLASS(AActor, UObject)

void AActor::Initialize()
{
	Super::Initialize();
	OwningLevel = nullptr;
	RegisteredWorld = nullptr;
	bHasBegunPlay = false;
	bTickEnabled = false;
	Name = FName();
	Guid.Invalidate();
}

UWorld* AActor::GetWorld() const
{
	return OwningLevel ? OwningLevel->GetOwningWorld() : nullptr;
}

void AActor::Release()
{
	if (bHasBegunPlay)
	{
		EndPlay();
	}

	if (RegisteredWorld)
	{
		Unregister();
	}

	// 등록 여부와 상관없이 소속 레벨의 목록에서 제외한다 (댕글링 포인터 방지).
	if (OwningLevel)
	{
		OwningLevel->RemoveActor(this);
	}

	while (!OwnedComponents.empty())
	{
		UActorComponent* Component = OwnedComponents.back();
		std::erase(OwnedComponents, Component);

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

	ComponentsByName.clear();
	NextComponentNameNumber.clear();

	Super::Release();
}

void AActor::Serialize(FArchive& Archive) const
{
	Super::Serialize(Archive);

	Archive.SetString("Name", Name.ToString());
	Archive.SetString("Guid", Guid.ToString());

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

	TArray<FArchive> OwnedArchives;
	OwnedArchives.reserve(OwnedComponents.size());

	for (UActorComponent* ActorComponent : OwnedComponents)
	{
		if (!ActorComponent || ActorComponent == RootComponent) continue;

		FArchive OwnedArchive;
		ActorComponent->Serialize(OwnedArchive);
		OwnedArchives.push_back(OwnedArchive);
	}
	Archive.SetArchiveArray("OwnedComponents", OwnedArchives);
}

void AActor::Deserialize(const FArchive& Archive)
{
	Super::Deserialize(Archive);

	// 이름과 Guid는 레벨이 유일성을 관리하므로 레벨을 통해 바꾼다. (이전 파일에는 없을 수 있음)
	if (OwningLevel)
	{
		if (!Archive.IsNull("Name"))
		{
			const FName SavedName(Archive.GetString("Name"));
			if (!OwningLevel->SetActorName(this, SavedName))
			{
				UE_LOG_WARN("[%s::Deserialize] 저장된 이름 %s을(를) 사용할 수 없어 %s을(를) 유지합니다.",
					GetClass()->GetUClassName(), SavedName.ToString(), Name.ToString());
			}
		}

		if (!Archive.IsNull("Guid"))
		{
			FGuid SavedGuid;
			if (FGuid::Parse(Archive.GetString("Guid"), SavedGuid))
			{
				OwningLevel->SetActorGuid(this, SavedGuid);
			}
		}
	}

	DeserializeRootComponent(Archive);

	if (!Archive.IsNull("OwnedComponents"))
	{
		DeserializeOwnedComponents(Archive.GetArchiveArray("OwnedComponents"));
	}
}

void AActor::DeserializeRootComponent(const FArchive& Archive)
{
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
	const FString SavedTypeName = RootComponentArchive.GetString("Type");
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

	if (!RootComponentArchive.IsNull("Name"))
	{
		SetComponentName(RootComponent, FName(RootComponentArchive.GetString("Name")));
	}

	RootComponent->Deserialize(RootComponentArchive);
}

void AActor::DeserializeOwnedComponents(const TArray<FArchive>& ComponentArchives)
{
	for (const FArchive& ComponentArchive : ComponentArchives)
	{
		const FString SavedTypeName = ComponentArchive.GetString("Type");
		UClass* SavedClass = UClass::FindByName(SavedTypeName);
		if (SavedClass == nullptr)
		{
			UE_LOG_WARN("[%s::Deserialize] 알 수 없는 타입 %s",
				GetClass()->GetUClassName(), SavedTypeName);
			continue;
		}

		const FName SavedName = ComponentArchive.IsNull("Name") ? FName() : FName(ComponentArchive.GetString("Name"));

		UActorComponent* Component = SavedName.IsNone() ? nullptr : FindComponentByName(SavedName);
		if (Component && (Component == RootComponent || Component->GetClass() != SavedClass))
		{
			Component = nullptr;
		}

		if (Component == nullptr)
		{
			if (!SavedClass->IsChildOrSelfOf(UActorComponent::StaticClass()))
			{
				UE_LOG_WARN("[%s::Deserialize] OwnedComponents %s를 생성할 수 없습니다.",
					GetClass()->GetUClassName(), SavedTypeName);
				continue;
			}

			UObject* Object = NewObject(SavedClass);
			Component = Object ? Object->Cast<UActorComponent>() : nullptr;

			Component->SetName(SavedName);
			AddComponent(Component);
		}

		Component->Deserialize(ComponentArchive);
	}

	for (const FArchive& ComponentArchive : ComponentArchives)
	{
		if (ComponentArchive.IsNull("Name") || ComponentArchive.IsNull("Parent") || !ComponentArchive.IsNull("ParentActor")) // ParentActor가 있으면 Level에서 설정
		{
			continue;
		}

		UActorComponent* Component = FindComponentByName(FName(ComponentArchive.GetString("Name")));
		UActorComponent* ParentComponent = FindComponentByName(FName(ComponentArchive.GetString("Parent")));
		USceneComponent* Child = Component ? Component->Cast<USceneComponent>() : nullptr;
		USceneComponent* Parent = ParentComponent ? ParentComponent->Cast<USceneComponent>() : nullptr;

		if (Child == nullptr || Parent == nullptr)
		{
			UE_LOG_WARN("[%s::Deserialize] %s의 부모 %s을(를) 찾을 수 없습니다.",
				GetClass()->GetUClassName(), ComponentArchive.GetString("Name"), ComponentArchive.GetString("Parent"));
			continue;
		}

		Child->RestoreAttachment(Parent);
	}
}

namespace 
{
	void Restore(USceneComponent* Child, const FArchive& ComponentArchive, const TMap<FString, AActor*>& LoadedActorsByGuid)
	{
		if (Child == nullptr || ComponentArchive.IsNull("Parent") || ComponentArchive.IsNull("ParentActor"))
		{
			return;
		}

		const auto ParentActorIt = LoadedActorsByGuid.find(ComponentArchive.GetString("ParentActor"));
		AActor* ParentActor = ParentActorIt != LoadedActorsByGuid.end() ? ParentActorIt->second : nullptr;
		UActorComponent* ParentComponent = ParentActor ? ParentActor->FindComponentByName(FName(ComponentArchive.GetString("Parent"))) : nullptr;
		USceneComponent* ParentSceneComponent = ParentComponent ? ParentComponent->Cast<USceneComponent>() : nullptr;

		if (ParentSceneComponent == nullptr)
		{
			return;
		}

		Child->RestoreAttachment(ParentSceneComponent);
	}

	void RestoreHomingTarget(UProjectileMovementComponent* Projectile, const FArchive& ComponentArchive, const TMap<FString, AActor*>& LoadedActorsByGuid)
	{
		if (Projectile == nullptr || ComponentArchive.IsNull("HomingTarget") || ComponentArchive.IsNull("ParentHomingTarget"))
		{
			return;
		}

		const auto ParentHomingTargetIt = LoadedActorsByGuid.find(ComponentArchive.GetString("ParentHomingTarget"));
		AActor* ParentHomingTarget = ParentHomingTargetIt != LoadedActorsByGuid.end() ? ParentHomingTargetIt->second : nullptr;
		UActorComponent* HomingTarget = ParentHomingTarget ? ParentHomingTarget->FindComponentByName(FName(ComponentArchive.GetString("HomingTarget"))) : nullptr;
		USceneComponent* HomingTargetComponent = HomingTarget ? HomingTarget->Cast<USceneComponent>() : nullptr;

		if (HomingTargetComponent == nullptr)
		{
			return;
		}

		Projectile->HomingTargetComponent = HomingTargetComponent;
	}
}

void AActor::RestoreExternalAttachments(const FArchive& Archive, const TMap<FString, AActor*>& LoadedActorsByGuid)
{
	if (!Archive.IsNull("RootComponent"))
	{
		Restore(RootComponent, Archive.GetArchive("RootComponent"), LoadedActorsByGuid);
	}

	if (Archive.IsNull("OwnedComponents"))
	{
		return;
	}

	for (const FArchive& ComponentArchive : Archive.GetArchiveArray("OwnedComponents"))
	{
		if (ComponentArchive.IsNull("Name"))
		{
			continue;
		}

		UActorComponent* Component = FindComponentByName(FName(ComponentArchive.GetString("Name")));
		RestoreHomingTarget(Component ? Component->Cast<UProjectileMovementComponent>() : nullptr, ComponentArchive, LoadedActorsByGuid);
		Restore(Component ? Component->Cast<USceneComponent>() : nullptr, ComponentArchive, LoadedActorsByGuid);
	}
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
		UE_LOG_WARN("이미 Root 컴포넌트가 있습니다.");
		return;
	}

	AddComponent(Component);
}

void AActor::MarkComponentsTransformDirty()
{
	for (UActorComponent* ActorComponent : OwnedComponents)
	{
		USceneComponent* Component = ActorComponent->Cast<USceneComponent>();
		if (!Component)
		{
			continue;
		}

		Component->OnTransformChanged();

		// 같은 액터의 자식은 AttachedComp에 있으므로, 여기서는 붙어 있는 다른 액터로만 내려간다.
		for (USceneComponent* Child : Component->GetChildren())
		{
			if (!Child || Child->GetSceneOwner() != Component)
			{
				continue;
			}

			AActor* ChildActor = Child->GetActorOwner();
			if (ChildActor == nullptr)
			{
				Child->OnTransformChanged();
			}
			else if (ChildActor != this)
			{
				ChildActor->MarkComponentsTransformDirty();
			}
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
				//std::erase(AttachedComp, CastSceneComponent);
			}
			UActorComponent* TempComponet = *It;
			if (auto NameIt = ComponentsByName.find(TempComponet->GetName()); NameIt != ComponentsByName.end() && NameIt->second == TempComponet)
			{
				ComponentsByName.erase(NameIt);
			}
			OwnedComponents.erase(It);
			DestroyObject(TempComponet);
			return;
		}
	}
}

UActorComponent* AActor::FindComponentByName(const FName& Name) const
{
	const auto It = ComponentsByName.find(Name);
	return It != ComponentsByName.end() ? It->second : nullptr;
}

bool AActor::SetComponentName(UActorComponent* Component, const FName& NewName)
{
	if (!Component || NewName.IsNone())
	{
		return false;
	}

	const auto Existing = ComponentsByName.find(NewName);
	if (Existing != ComponentsByName.end() && Existing->second != Component)
	{
		return false;
	}

	if (auto It = ComponentsByName.find(Component->GetName()); It != ComponentsByName.end() && It->second == Component)
	{
		ComponentsByName.erase(It);
	}
	Component->SetName(NewName);
	ComponentsByName[NewName] = Component;
	return true;
}

void AActor::RegisterComponentName(UActorComponent* Component)
{
	const auto Existing = ComponentsByName.find(Component->GetName());
	if (Component->GetName().IsNone() || (Existing != ComponentsByName.end() && Existing->second != Component))
	{
		Component->SetName(MakeUniqueComponentName(Component));
	}
	ComponentsByName[Component->GetName()] = Component;
}


void AActor::AddComponent(UActorComponent* Addcomp)
{
	if (Addcomp == nullptr)
	{
		return;
	}

	Addcomp->SetActorOwner(this);

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
	}

	OwnedComponents.push_back(Addcomp);
	RegisterComponentName(Addcomp);
	Addcomp->Initialize();

	if (RegisteredWorld)
	{
		Addcomp->Register(RegisteredWorld);
	}

	if (bHasBegunPlay)
	{
		Addcomp->BeginPlay();
	}
}

void AActor::Register(UWorld* InWorld)
{
	if (RegisteredWorld == InWorld)
	{
		return;
	}

	if (RegisteredWorld)
	{
		Unregister();
	}

	RegisteredWorld = InWorld;
	for (UActorComponent* Component : OwnedComponents)
	{
		if (Component)
		{
			Component->Register(InWorld);
		}
	}
	bRegistered = true;
}

void AActor::BeginPlay() {
	if (!RegisteredWorld || bHasBegunPlay)
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

void AActor::Tick(float DeltaTime, ELevelTick eTickType) 
{
	
}

void AActor::EndPlay()
{
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

	if (!RegisteredWorld)
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
	RegisteredWorld = nullptr;
	bRegistered = false;
}

bool AActor::ShouldTick(ELevelTick TickType) const
{
	if (!bTickEnabled || !bRegistered) return false;

	switch (TickType)
	{
	case LEVELTICK_All:           return bHasBegunPlay;              // 게임 틱은 BeginPlay 이후만
	case LEVELTICK_ViewportsOnly: return bShouldTickIfViewportsOnly; // 편집 월드는 opt-in한 액터만
	default:                      return false;                      // TimeOnly, PauseTick
	}
}

void AActor::Destroy() {
	if (UWorld* World = GetWorld())
	{
		World->DestroyActor(this);
		return;
	}
	DestroyObject(this);
}


// "클래스이름_번호" 형식의 아직 쓰이지 않은 이름을 만든다.
FName AActor::MakeUniqueComponentName(const UActorComponent* Component)
{
	// "UActorComponent" -> "ActorComponent"
	FString Base = Component->GetClass()->GetUClassName();
	if (Base.size() > 1 && Base[0] == 'U' && Base[1] >= 'A' && Base[1] <= 'Z') {
		Base.erase(0, 1);
	}

	int32& Number = NextComponentNameNumber[Base];
	while (true) {
		const FName Candidate(Base + "_" + std::to_string(Number++));
		if (!ComponentsByName.contains(Candidate)) {
			return Candidate;
		}
	}
}