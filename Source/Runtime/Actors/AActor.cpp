#include "AActor.h"
#include "Runtime/Core/Log.h"
#include "Runtime/CoreUObject/UClass.h"
#include "Runtime/CoreUObject/UObjectGlobals.h"
#include "Runtime/CoreUObject/USceneComponent.h"
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

		//TODO: USceneComponent 계층 구조와 계층과 상관없느 UActorComponent 모두 직렬화 하도록 변경
		TArray<FArchive> CompArchives;
		CompArchives.reserve(AttachedComp.size() - 1);
		for (USceneComponent* ActorComponent : AttachedComp)
		{
			if (ActorComponent == RootComponent) continue;

			FArchive CompAcrchive;
			ActorComponent->Serialize(CompAcrchive);
			CompArchives.push_back(CompAcrchive);
		}

		Archive.SetArchiveArray("AttachedComponent", CompArchives);
	}
	else
	{
		Archive.SetNull("RootComponent");
		Archive.SetNull("AttachedComponent");
	}
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

	TArray<FArchive> AttachedCompArchive = Archive.GetArchiveArray("AttachedComponent");
	if (!AttachedCompArchive.empty())
	{
		for (const FArchive& Archive : AttachedCompArchive)
		{
			const FString& CompSavedTypeName = RootComponentArchive.GetString("Type");
			SavedClass = UClass::FindByName(SavedTypeName);

			USceneComponent* SceneComponent = NewObject(SavedClass)->Cast<USceneComponent>();
			if (!SceneComponent)
			{
				UE_LOG_WARN("[%s::Deserialize] AttachedComponent %s를 생성할 수 없습니다.",
					GetClass()->GetUClassName(), CompSavedTypeName);
				return;
			}

			AddComponent(SceneComponent);
		}
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
		throw EngineUtil::CreateError("이미 Root 컴포넌트가 있습니다.");
	}

	OwnedComponents.push_back(Component);
	Component->SetActorOwner(this);
	Component->Initialize();
	USceneComponent* SceneComponent = Component->Cast<USceneComponent>();

	if (RegisteredWorld)
	{
		Component->Register(RegisteredWorld);
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

	//Addcomp->ActorOwner = this;
	OwnedComponents.push_back(Addcomp);
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
