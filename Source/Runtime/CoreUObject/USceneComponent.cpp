#include "UClass.h"
#include "USceneComponent.h"
#include "ThirdParty/Json/json.hpp"
#include "UObjectGlobals.h" 
#include "UPrimitiveComponent.h"
#include "Runtime/Actors/AActor.h"
#include "Runtime/Engine/FArchive.h"
#include "Runtime/Engine/FScene.h"


IMPLEMENT_UCLASS(USceneComponent, UActorComponent)

void USceneComponent::Initialize()
{
    Super::Initialize();
}

void USceneComponent::Release()
{
    const TArray<USceneComponent*> Detaching = Children;
    for (USceneComponent* Child : Detaching)
    {
        if (Child && Child->SceneOwner == this)
        {
            Child->DetachFromParent();
        }
    }
    Children.clear();

    // 부모가 있는지를 본다. 다른 액터에 붙은 루트도 부모의 목록에서 빠져야 한다.
    if (SceneOwner)
    {
        SceneOwner->DeleteChildren(this);
        SceneOwner = nullptr;
    }
    Super::Release();
}

void USceneComponent::DeleteChildren(USceneComponent* InChildren)
{
    for (auto It = Children.begin(); It != Children.end(); It++)
    {
        if (*It == InChildren)
        {
            Children.erase(It);
            return;
        }
    }
}

bool USceneComponent::IsRootComponent() const
{
    return (ActorOwner && ActorOwner->GetRootComponent() == this);
}

void USceneComponent::AddChildren(USceneComponent* InChildren)
{
    Children.push_back(InChildren);
}

void USceneComponent::SetupAttachment(USceneComponent* InParent, bool bKeepWorldTransform)
{
    if (!InParent) 
    {
        DetachFromParent();
        return; 
    }

    // 자기 자신이나 자기 자손 밑으로는 못 붙인다. 순환이 생기면 GetGlobalTransform이 무한 재귀한다.
    for (const USceneComponent* It = InParent; It; It = It->SceneOwner)
    {
        if (It == this) { return; }
    }

    // 부모를 바꾸기 전의 월드 트랜스폼
    const FTransform WorldTransform = bKeepWorldTransform ? GetGlobalTransform() : FTransform{};

    if (SceneOwner)
    {
        SceneOwner->DeleteChildren(this);
    }

    SceneOwner = InParent;
    InParent->AddChildren(this);
    bGlobalDirty = true;

    if (bKeepWorldTransform)
    {
        SetRelativeTransformFromGlobal(WorldTransform);
    }
    else
    {
        //FTransform ParentScale;
        //ParentScale.SetScale3D(InParent->GetGlobalTransform().GetScale3D());
        FTransform DefaultTransform;

        SetRelativeTransform(FTransform().GetRelativeTo(DefaultTransform));
    }
}

void USceneComponent::DetachFromParent()
{
    if (!SceneOwner) { return; }

    // 부모를 비우기 전의 월드 트랜스폼을 구해 두고, 떼어 낸 뒤 그 자리에 다시 놓는다.
    const FTransform WorldTransform = GetGlobalTransform();

    SceneOwner->DeleteChildren(this);
    SceneOwner = nullptr;
    bGlobalDirty = true;

    SetRelativeTransformFromGlobal(WorldTransform);
}

void USceneComponent::RestoreAttachment(USceneComponent* InParent)
{
    if (!InParent || InParent == SceneOwner) { return; }

    for (const USceneComponent* It = InParent; It; It = It->SceneOwner)
    {
        if (It == this) { return; }
    }

    if (SceneOwner)
    {
        SceneOwner->DeleteChildren(this);
    }

    SceneOwner = InParent;
    InParent->AddChildren(this);
    MarkActorTransformDirty();
}

TArray<USceneComponent*>& USceneComponent::GetChildren()
{
    return (Children);
}

void USceneComponent::SetRelativeTransformFromGlobal(const FTransform& GlobalTransform)
{
    USceneComponent* Parent = GetTransformParent();
    if (Parent)
    {
        SetRelativeTransform(GlobalTransform.GetRelativeTo(Parent->GetGlobalTransform()));
    }
    else
    {
        SetRelativeTransform(GlobalTransform);
    }
}

void USceneComponent::Serialize(FArchive& Archive) const
{
    Super::Serialize(Archive);

    Archive.SetVector("Location", RelativeTransform.GetLocation());
    Archive.SetVector("Rotation", RelativeTransform.GetRotation().GetEulerXYZ());
    Archive.SetVector("Scale", RelativeTransform.GetScale3D());

    if (!SceneOwner)
    {
        return;
    }

    Archive.SetString("Parent", SceneOwner->GetName().ToString());

    const AActor* ParentActor = SceneOwner->GetActorOwner();
    if (ParentActor && ParentActor != ActorOwner)
    {
        Archive.SetString("ParentActor", ParentActor->GetGuid().ToString());
    }
}

void USceneComponent::Deserialize(const FArchive& Archive)
{
    Super::Deserialize(Archive);

    // Location
    RelativeTransform.SetLocation(Archive.GetVector("Location"));

    // Rotation
    constexpr float RadToDeg = 180.0f / std::numbers::pi_v<float>;
    FVector Rotation = Archive.GetVector("Rotation");
    for (int i = 0; i < 3; ++i)
    {
        Rotation[i] *= RadToDeg;
    }
    RelativeTransform.SetRotation(FQuaternion::FromEulerXYZDeg(Rotation));

    // Scale
    RelativeTransform.SetScale3D(Archive.GetVector("Scale"));
    MarkActorTransformDirty();
}

void USceneComponent::SetRelativeTransform(const FTransform& RelativeTransform)
{
    if (this->RelativeTransform == RelativeTransform) { return; }
    this->RelativeTransform = RelativeTransform;
    MarkActorTransformDirty();
}

USceneComponent* USceneComponent::GetTransformParent() const
{
    if (SceneOwner)
    {
        return SceneOwner;
    }

    if (!ActorOwner)
    {
        return nullptr;
    }

    USceneComponent* Root = ActorOwner->GetRootComponent();
    return Root == this ? nullptr : Root;
}

const FTransform& USceneComponent::GetGlobalTransform() const
{
    const USceneComponent* Parent = GetTransformParent();
    uint32 ParentVersion = 0;
    if (Parent)
    {
        Parent->GetGlobalTransform(); // 부모 캐시를 먼저 최신화
        ParentVersion = Parent->GlobalVersion;
    }

    if (!bGlobalDirty && CachedParent == Parent && CachedParentVersion == ParentVersion)
    {
        return CachedGlobal;
    }

    if (!Parent)
    {
        CachedGlobal = RelativeTransform;
    }
    else if (SceneOwner || bInheritRotation)
    {
        CachedGlobal = Parent->CachedGlobal * RelativeTransform;
    }
    else
    {
        // 부모 회전 무시 - 위치와 스케일만 상속
        FTransform Result;
        Result.SetScale3D(RelativeTransform.GetScale3D());
        Result.SetRotation(RelativeTransform.GetRotation()); // 자신의 회전만 사용
        Result.SetLocation(Parent->CachedGlobal.GetLocation() + RelativeTransform.GetLocation()); // 월드 축 기준 오프셋
        CachedGlobal = Result;
    }

    CachedGlobal.GetMatrix(); // 행렬도 이 시점에 한 번만 계산해 둔다
    CachedParent = Parent;
    CachedParentVersion = ParentVersion;
    bGlobalDirty = false;
    ++GlobalVersion;
    return CachedGlobal;
}

const FMatrix* USceneComponent::GetGlobalInverseMatrix() const
{
    const FTransform& Global = GetGlobalTransform();
    if (CachedInverseVersion != GlobalVersion)
    {
        // 실패하면 Inverse는 CachedGlobalInverse를 건드리지 않으므로 성공 여부를 따로 기록한다
        bCachedInverseValid = Global.GetMatrix().Inverse(CachedGlobalInverse);
        CachedInverseVersion = GlobalVersion;
    }
    return bCachedInverseValid ? &CachedGlobalInverse : nullptr;
}

void USceneComponent::MarkActorTransformDirty()
{
    bGlobalDirty = true;
    OnTransformChanged();

    if (ActorOwner)
    {
        ActorOwner->MarkComponentsTransformDirty();
    }
}

void USceneComponent::SetRelativeLocation(const FVector& RelativeLocation)
{
    FTransform NewTransform = GetRelativeTransform();
    NewTransform.SetLocation(RelativeLocation);
    SetRelativeTransform(NewTransform);
}

void USceneComponent::SetRelativeRotation(const FVector& RelativeRotationEulerAngle)
{
    FTransform NewTransform = GetRelativeTransform();
    NewTransform.SetRotation(FQuaternion::FromEulerXYZDeg(RelativeRotationEulerAngle));
    SetRelativeTransform(NewTransform);
}

void USceneComponent::SetRelativeRotation(const FQuaternion& RelativeRotation)
{
    FTransform NewTransform = GetRelativeTransform();
    NewTransform.SetRotation(RelativeRotation);
    SetRelativeTransform(NewTransform);
}

void USceneComponent::SetRelativeScale(const FVector& RelativeScale)
{
    FTransform NewTransform = GetRelativeTransform();
    NewTransform.SetScale3D(RelativeScale);
    SetRelativeTransform(NewTransform);
}

const FVector& USceneComponent::GetRelativeLocation() const
{
    return GetRelativeTransform().GetLocation();
}

const FQuaternion& USceneComponent::GetRelativeRotation() const
{
    return GetRelativeTransform().GetRotation();
}

const FVector& USceneComponent::GetRelativeScale() const
{
    return GetRelativeTransform().GetScale3D();
}
