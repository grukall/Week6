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

	while (!AttachedComp.empty())
	{
		USceneComponent* Component = AttachedComp.back();
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
	}
	else
	{
		Archive.SetNull("RootComponent");
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
}

void AActor::CreateRootComponent(UClass* ClassType)
{
	if (RootComponent) { return; }

	UObject* Object = NewObject(ClassType);
	USceneComponent* Component = Object->Cast<USceneComponent>();

	if (!Component)
	{
		DestroyObject(Object);
		return;
	}

	SetRootComponent(Component);
}

void AActor::SetRootComponent(USceneComponent* Component)
{
	if (RootComponent)
	{
		throw EngineUtil::CreateError("이미 Root 컴포넌트가 있습니다.");
	}

	RootComponent = Component;

	// TODO ActorOwner를 이렇게 지정하면 안됨
	RootComponent->ActorOwner = this;
	RootComponent->SetupAttachment(nullptr);
	RootComponent->Initialize();
	AttachedComp.push_back(RootComponent);

	if (RegisteredWorld)
	{
		RootComponent->Register(RegisteredWorld);
	}

	if (bHasBegunPlay)
	{
		RootComponent->BeginPlay();
	}
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

void AActor::AddComponent(USceneComponent* Addcomp)
{
	if (Addcomp == nullptr)
	{
		return;
	}

	if (RootComponent == nullptr)
	{
		RootComponent = Addcomp;
		Addcomp->SetupAttachment(nullptr);
	}

	else if (Addcomp->GetSceneOwner() == nullptr)
	{
		Addcomp->SetupAttachment(RootComponent);
	}

	Addcomp->ActorOwner = this;
	AttachedComp.push_back(Addcomp);
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

void AActor::Register(UWorld* World)
{
	if (RegisteredWorld == World)
	{
		return;
	}

	if (RegisteredWorld)
	{
		Unregister();
	}

	RegisteredWorld = World;
	for (USceneComponent* Component : AttachedComp)
	{
		if (Component)
		{
			Component->Register(World);
		}
	}
}

void AActor::BeginPlay() {
	if (!RegisteredWorld || bHasBegunPlay)
	{
		return;
	}

	bHasBegunPlay = true;
	for (USceneComponent* Component : AttachedComp)
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

	for (USceneComponent* Component : AttachedComp)
	{
		if (Component && Component->IsTickEnabled())
		{
			Component->Tick(DeltaTime);
		}
	}
}

void AActor::EndPlay() {
	if (!bHasBegunPlay)
	{
		return;
	}

	for (auto It = AttachedComp.rbegin(); It != AttachedComp.rend(); ++It)
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

	for (auto It = AttachedComp.rbegin(); It != AttachedComp.rend(); ++It)
	{
		if (*It)
		{
			(*It)->Unregister();
		}
	}
	RegisteredWorld = nullptr;
}

void AActor::Destroy() {
	if (UWorld* World = GetWorld())
	{
		World->DestroyActor(this);
		return;
	}
	DestroyObject(this);
}
