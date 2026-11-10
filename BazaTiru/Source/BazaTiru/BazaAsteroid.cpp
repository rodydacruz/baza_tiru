#include "BazaAsteroid.h"

#include "BazaGameMode.h"
#include "BazaPlayer.h"
#include "BazaRockMesh.h"
#include "BazaShot.h"
#include "Components/SphereComponent.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ABazaAsteroid::ABazaAsteroid()
{
	// asteroid.tscn: CircleShape2D, radius 6
	HitZone = CreateHitSphere(TEXT("HitZone"), 6.f);

	Rock = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Rock"));
	Rock->SetupAttachment(RootComponent);
	Rock->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Rock->SetCastShadow(false);
	Rock->bUseAsyncCooking = false;

	ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialAsset(BazaMesh::Material);
	if (MaterialAsset.Succeeded())
	{
		Rock->SetMaterial(0, MaterialAsset.Object);
	}

	// The shape is built per instance in BeginPlay, but the tint system has to
	// know about the component up front.
	RegisterTintedMesh(Rock, FLinearColor(0.52f, 0.43f, 0.37f));
}

void ABazaAsteroid::BeginPlay()
{
	Super::BeginPlay();

	AsteroidId = FMath::Rand();

	// Every asteroid gets its own solid, from its own seed: same family, no two
	// alike. The 2D version flipped between two sprite frames instead.
	FBazaRockParams Params;
	Params.Seed = AsteroidId;
	Params.Radius = BazaSettings::Px(RadiusPx);
	Params.Subdivisions = 1;
	Params.Lumpiness = Lumpiness;

	BazaRockMesh::Build(Rock, Params);

	// A slow tumble, and no two tumble alike either.
	Spin = FRotator(
		FMath::FRandRange(-70.f, 70.f),
		FMath::FRandRange(-70.f, 70.f),
		FMath::FRandRange(-70.f, 70.f));

	Rock->SetRelativeRotation(FRotator(
		FMath::FRandRange(0.f, 360.f),
		FMath::FRandRange(0.f, 360.f),
		FMath::FRandRange(0.f, 360.f)));

	OnActorBeginOverlap.AddDynamic(this, &ABazaAsteroid::HandleOverlap);
}

void ABazaAsteroid::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FVector2D Position = GetGamePosition();
	Position.X -= MoveSpeed * DeltaSeconds;
	SetGamePosition(Position);

	Rock->AddLocalRotation(Spin * DeltaSeconds);

	if (Position.X <= -100.f)
	{
		Destroy();
	}
}

void ABazaAsteroid::HandleOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	// The original also tested the "low_nemy" group here, but low_nemy.tscn is
	// never put in that group, so asteroids and low enemies pass through each
	// other. Left out on purpose to keep the collisions as they play today.
	const bool bHit = OtherActor->IsA<ABazaShot>() || OtherActor->IsA<ABazaPlayer>();
	if (!bHit || bScoreEmitted)
	{
		return;
	}

	bScoreEmitted = true;
	OnScore.Broadcast(1, AsteroidId);
	SpawnExplosionAt(GetGamePosition(), 16.f);

	if (ABazaGameMode* Stage = GetStage())
	{
		Stage->AddShake(3.f);
	}

	Remove();
}
