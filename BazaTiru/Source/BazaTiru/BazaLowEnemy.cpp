#include "BazaLowEnemy.h"

#include "BazaGameMode.h"
#include "BazaPlayer.h"
#include "BazaSfx.h"
#include "BazaShipMesh.h"
#include "BazaShot.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ABazaLowEnemy::ABazaLowEnemy()
{
	// low_nemy.tscn: CircleShape2D, radius 8.062258
	HitBox = CreateHitSphere(TEXT("HitBox"), 8.062258f);

	// The same generated hull as the player, flipped to face left and given
	// stubbier, meaner proportions.
	Body = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(RootComponent);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetCastShadow(false);
	Body->bUseAsyncCooking = false;

	ConstructorHelpers::FObjectFinder<UMaterialInterface> BodyMaterial(BazaMesh::Material);
	if (BodyMaterial.Succeeded())
	{
		Body->SetMaterial(0, BodyMaterial.Object);
	}

	RegisterTintedMesh(Body, FLinearColor(0.9f, 0.3f, 0.25f));

	Cannon = CreateShape(TEXT("Cannon"), BazaMesh::Cylinder, FVector(3.f, 3.f, 9.f), FLinearColor(0.25f, 0.22f, 0.3f));
	Cannon->SetRelativeLocation(FVector(BazaSettings::Px(-9.f), 0.f, 0.f));
	Cannon->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
}

void ABazaLowEnemy::BeginPlay()
{
	Super::BeginPlay();

	EnemyId = FMath::Rand();

	FBazaShipParams Ship;
	Ship.Length = BazaSettings::Px(19.f);
	Ship.Span = BazaSettings::Px(17.f);
	Ship.Depth = BazaSettings::Px(8.f);
	Ship.WingSweep = 0.25f;
	Ship.NoseLength = 0.35f;
	Ship.bDorsalFin = false;
	Ship.bFacesBackwards = true;

	BazaShipMesh::Build(Body, Ship);

	BaseY = GetGamePosition().Y;
	DriftPhase = FMath::FRandRange(0.f, 6.28f);

	OnActorBeginOverlap.AddDynamic(this, &ABazaLowEnemy::HandleOverlap);

	// low_nemy.tscn: shot_timer, autostart, Godot's default 1 second, repeating.
	GetWorldTimerManager().SetTimer(
		ShotTimer, this, &ABazaLowEnemy::OnShotTimerTimeout, 1.f, true);
}

void ABazaLowEnemy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Drift += DeltaSeconds;

	FVector2D Position = GetGamePosition();
	Position.X -= Speed * DeltaSeconds;

	// lowney.gd flew dead straight. A slow weave makes a gunship worth tracking
	// instead of something you shoot where it already is.
	Position.Y = BaseY + FMath::Sin(Drift * 2.2f + DriftPhase) * DriftAmplitude;

	SetGamePosition(Position);

	// Lean into the weave.
	SetActorRotation(FRotator(0.f, 0.f, FMath::Cos(Drift * 2.2f + DriftPhase) * 18.f));

	// lowney.gd never cleans itself up, so every gunship that survives keeps
	// ticking and firing forever off-screen. Culling it at the same boundary the
	// asteroids use costs nothing visible and stops the actor count climbing.
	if (Position.X <= -100.f)
	{
		Destroy();
	}
}

void ABazaLowEnemy::OnShotTimerTimeout()
{
	Shoot();
}

void ABazaLowEnemy::Shoot()
{
	ABazaGameMode* Stage = GetStage();
	if (!Stage)
	{
		return;
	}

	const FVector2D Position = GetGamePosition();

	if (ABazaShot* Bullet = Stage->SpawnAt<ABazaShot>(FVector2D(Position.X - 20.f, Position.Y)))
	{
		// lowney.gd always fired flat left, which never threatened a player sitting
		// on a different row. It leads on the ship now, but only from behind: a
		// gunship the player has already slipped past still fires straight.
		const FVector2D ToPlayer = Stage->GetPlayerPosition() - Position;

		Bullet->Direction = (ToPlayer.X < -8.f)
			? ToPlayer.GetSafeNormal()
			: FVector2D(-1.f, 0.f);

		UBazaSfx::Play(this, EBazaSfx::EnemyShot, 0.8f);
	}
}

void ABazaLowEnemy::HandleOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	// Note this catches the player's bolts and the gunships' own, since both are
	// the same scene and shared the "shot" group in Godot.
	// Rammed as well as shot: lowney.gd let the ship fly straight through a
	// gunship. Now the collision costs the player a life and the gunship too.
	const bool bHit = OtherActor->IsA<ABazaShot>() || OtherActor->IsA<ABazaPlayer>();
	if (!bHit || bScoreEmitted)
	{
		return;
	}

	bScoreEmitted = true;
	OnScore.Broadcast(10, EnemyId);
	SpawnExplosionAt(GetGamePosition(), 20.f);

	if (ABazaGameMode* Stage = GetStage())
	{
		Stage->AddShake(5.f);
	}

	Remove();
}
