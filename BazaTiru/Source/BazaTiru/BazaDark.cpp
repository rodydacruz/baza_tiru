#include "BazaDark.h"

#include "BazaGameMode.h"
#include "BazaPlayer.h"
#include "BazaSfx.h"
#include "BazaShot.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"

ABazaDark::ABazaDark()
{
	// dark.tscn uses a RectangleShape2D left at Godot's default 20x20.
	HitBox = CreateHitBox(TEXT("HitBox"), FVector2D(20.f, 20.f));

	Shell = CreateShape(TEXT("Shell"), BazaMesh::Cube, FVector(17.f, 17.f, 17.f), FLinearColor(0.16f, 0.1f, 0.28f));
	Shell->SetRelativeRotation(FRotator(45.f, 0.f, 45.f));

	// A lit core showing through the shell, so a damaged dark reads at a glance.
	Core = CreateShape(TEXT("Core"), BazaMesh::Sphere, FVector(9.f, 9.f, 9.f), FLinearColor(0.75f, 0.3f, 1.f));
}

void ABazaDark::BeginPlay()
{
	Super::BeginPlay();

	DarkId = FMath::Rand();

	OnActorBeginOverlap.AddDynamic(this, &ABazaDark::HandleOverlap);
}

void ABazaDark::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FVector2D Position = GetGamePosition();
	Position.X -= MoveSpeed * DeltaSeconds;
	SetGamePosition(Position);

	Spin += DeltaSeconds * 45.f;
	Shell->SetRelativeRotation(FRotator(45.f, Spin, 45.f));

	if (Position.X <= -100.f)
	{
		ReleaseSlot();
		Destroy();
	}
}

void ABazaDark::ReleaseSlot()
{
	if (bSlotReleased)
	{
		return;
	}

	bSlotReleased = true;

	// Whichever way this dark leaves, the stage has to get its slot back, or the
	// event counter never returns to zero and nothing new ever spawns again.
	if (ABazaGameMode* Stage = GetStage())
	{
		Stage->ReleaseEventSlot();
	}
}

void ABazaDark::Shoot()
{
	ABazaGameMode* Stage = GetStage();
	if (!Stage)
	{
		return;
	}

	const FVector2D Position = GetGamePosition();

	if (ABazaShot* Bullet = Stage->SpawnAt<ABazaShot>(FVector2D(Position.X - 20.f, Position.Y)))
	{
		Bullet->Direction = FVector2D(-1.f, 0.f);
		UBazaSfx::Play(this, EBazaSfx::EnemyShot, 0.6f);
	}
}

void ABazaDark::HandleOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	if (Health <= 0)
	{
		return;
	}

	// Flying into a dark used to be harmless: dark.tscn joined no group, so the
	// player never noticed it. Now the collision costs the player a life (that
	// happens on the player's side) and bursts the dark outright.
	if (OtherActor->IsA<ABazaPlayer>())
	{
		Health = 0;
		OnScore.Broadcast(5, DarkId);
		SpawnExplosionAt(GetGamePosition(), 22.f);
		ReleaseSlot();
		Remove();
		return;
	}

	if (!OtherActor->IsA<ABazaShot>())
	{
		return;
	}

	// Still scores on every hit and still shoots back, as it always did.
	OnScore.Broadcast(1, DarkId);
	Shoot();
	FlashHit();
	UBazaSfx::Play(this, EBazaSfx::Hit);

	--Health;

	// The core dims as the shell gives way.
	Core->SetRelativeScale3D(FVector(BazaSettings::Px(9.f * Health / 3.f)) / 100.f);

	if (Health > 0)
	{
		return;
	}

	OnScore.Broadcast(5, DarkId);
	SpawnExplosionAt(GetGamePosition(), 22.f);
	ReleaseSlot();
	Remove();
}
