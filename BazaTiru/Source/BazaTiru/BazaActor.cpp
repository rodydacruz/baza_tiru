#include "BazaActor.h"

#include "BazaExplosion.h"
#include "BazaGameMode.h"
#include "BazaSfx.h"
#include "Components/BoxComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ABazaActor::ABazaActor()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);
}

void ABazaActor::BeginPlay()
{
	// SpawnActor places the actor and runs BeginPlay before SpawnAt has a chance
	// to call SetGamePosition, so without this every subclass reading its own
	// position in BeginPlay saw (0, 0) - which is how the gunships' weave ended up
	// centred on the top edge of the screen. The world transform is already
	// right at this point, so read it back.
	GamePosition = BazaSettings::ToGame(GetActorLocation());

	Super::BeginPlay();

	// A dynamic material instance cannot be created during construction, so
	// CreateShape only recorded what it wanted; this is where it happens.
	TintMaterials.Reserve(TintedMeshes.Num());

	for (UMeshComponent* Mesh : TintedMeshes)
	{
		TintMaterials.Add(Mesh ? Mesh->CreateAndSetMaterialInstanceDynamic(0) : nullptr);
	}

	PushTints();
}

void ABazaActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// The timer manager belongs to the game instance and outlives the level. A
	// timer still pending on an actor that has gone would fire into freed memory,
	// so nothing leaves the game with one set.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}

	Super::EndPlay(EndPlayReason);
}

void ABazaActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (FlashRemaining > 0.f)
	{
		FlashRemaining -= DeltaSeconds;

		if (FlashRemaining <= 0.f)
		{
			FlashRemaining = 0.f;
			PushTints();
		}
	}
}

void ABazaActor::PushTints()
{
	const bool bFlashing = FlashRemaining > 0.f;

	for (int32 Index = 0; Index < TintMaterials.Num(); ++Index)
	{
		UMaterialInstanceDynamic* Dynamic = TintMaterials[Index];
		if (!Dynamic)
		{
			continue;
		}

		FLinearColor Colour = bFlashing ? FLinearColor::White : TintColors[Index];
		Colour *= VisualOpacity;
		Colour.A = 1.f;

		// BasicShapeMaterial exposes "Color"; the second name covers a custom
		// material swapped in later. Unknown parameters are simply ignored.
		Dynamic->SetVectorParameterValue(TEXT("Color"), Colour);
		Dynamic->SetVectorParameterValue(TEXT("BaseColor"), Colour);
	}
}

void ABazaActor::FlashHit(float Seconds)
{
	FlashRemaining = FMath::Max(FlashRemaining, Seconds);
	PushTints();
}

void ABazaActor::Remove()
{
	if (bRemoved)
	{
		return;
	}

	bRemoved = true;

	// Gone as far as the game is concerned: invisible, inert, not colliding.
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);

	// And actually deleted once whatever overlap is being handled has finished.
	SetLifeSpan(0.05f);
}

void ABazaActor::SetMeshTint(UMeshComponent* Mesh, const FLinearColor& Colour)
{
	const int32 Index = TintedMeshes.IndexOfByKey(Mesh);
	if (Index == INDEX_NONE)
	{
		return;
	}

	TintColors[Index] = Colour;
	PushTints();
}

void ABazaActor::SetVisualOpacity(float Opacity)
{
	VisualOpacity = FMath::Clamp(Opacity, 0.f, 1.f);
	PushTints();
}

void ABazaActor::SetGamePosition(const FVector2D& NewPosition)
{
	GamePosition = NewPosition;
	SetActorLocation(BazaSettings::ToWorld(GamePosition));
}

void ABazaActor::SetGameRotation(float Degrees)
{
	// The play plane is XZ, so a spin within it is a rotation about Unreal's Y
	// axis, which is Pitch. Godot's positive rotation goes clockwise on screen,
	// Unreal's positive pitch goes the other way, hence the sign flip.
	SetActorRotation(FRotator(-Degrees, 0.f, 0.f));
}

ABazaGameMode* ABazaActor::GetStage() const
{
	return ABazaGameMode::Get(this);
}

void ABazaActor::SpawnExplosionAt(const FVector2D& Where, float SizePx)
{
	if (ABazaGameMode* Stage = GetStage())
	{
		if (ABazaExplosion* Explosion = Stage->SpawnAt<ABazaExplosion>(Where))
		{
			Explosion->Configure(SizePx);
		}
	}

	// The blast decides its own sound. The smallest ones are hit sparks, and
	// whatever was hit already made its own tick.
	if (SizePx >= 40.f)
	{
		UBazaSfx::Play(this, EBazaSfx::ExplosionBig);
	}
	else if (SizePx >= 12.f)
	{
		UBazaSfx::Play(this, EBazaSfx::ExplosionSmall, FMath::GetMappedRangeValueClamped(
			FVector2D(12.f, 40.f), FVector2D(0.6f, 1.f), SizePx));
	}
}

USphereComponent* ABazaActor::CreateHitSphere(FName Name, float RadiusPx)
{
	USphereComponent* Sphere = CreateDefaultSubobject<USphereComponent>(Name);
	Sphere->SetupAttachment(RootComponent);
	Sphere->InitSphereRadius(BazaSettings::Px(RadiusPx));

	// Area2D semantics: report overlaps, block nothing.
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sphere->SetCollisionObjectType(ECC_WorldDynamic);
	Sphere->SetCollisionResponseToAllChannels(ECR_Overlap);
	Sphere->SetGenerateOverlapEvents(true);

	return Sphere;
}

UBoxComponent* ABazaActor::CreateHitBox(FName Name, FVector2D SizePx)
{
	UBoxComponent* Box = CreateDefaultSubobject<UBoxComponent>(Name);
	Box->SetupAttachment(RootComponent);

	// Godot rectangles are given as a full size; Unreal wants half-extents.
	const float HalfX = BazaSettings::Px(SizePx.X) * 0.5f;
	const float HalfZ = BazaSettings::Px(SizePx.Y) * 0.5f;
	Box->InitBoxExtent(FVector(HalfX, HalfZ, HalfZ));

	Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Box->SetCollisionObjectType(ECC_WorldDynamic);
	Box->SetCollisionResponseToAllChannels(ECR_Overlap);
	Box->SetGenerateOverlapEvents(true);

	return Box;
}

UStaticMeshComponent* ABazaActor::CreateShape(FName Name, const TCHAR* MeshPath, FVector SizePx, FLinearColor Color)
{
	UStaticMeshComponent* Mesh = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);

	ConstructorHelpers::FObjectFinder<UStaticMesh> MeshAsset(MeshPath);
	if (MeshAsset.Succeeded())
	{
		Mesh->SetStaticMesh(MeshAsset.Object);

		// Every engine primitive is 100uu across, so the scale is just the size we want.
		Mesh->SetRelativeScale3D(FVector(
			BazaSettings::Px(SizePx.X),
			BazaSettings::Px(SizePx.Y),
			BazaSettings::Px(SizePx.Z)) / 100.f);
	}

	ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialAsset(BazaMesh::Material);
	if (MaterialAsset.Succeeded())
	{
		Mesh->SetMaterial(0, MaterialAsset.Object);
	}

	RegisterTintedMesh(Mesh, Color);

	return Mesh;
}

void ABazaActor::RegisterTintedMesh(UMeshComponent* Mesh, FLinearColor Color)
{
	if (!Mesh || TintedMeshes.Contains(Mesh))
	{
		return;
	}

	TintedMeshes.Add(Mesh);
	TintColors.Add(Color);
}
