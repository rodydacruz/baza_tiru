#include "BazaShot.h"

#include "BazaAsteroid.h"
#include "BazaBossBase.h"
#include "BazaDark.h"
#include "BazaLowEnemy.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"

namespace
{
	bool IsEnemy(const AActor* Actor)
	{
		return Actor->IsA<ABazaAsteroid>()
			|| Actor->IsA<ABazaLowEnemy>()
			|| Actor->IsA<ABazaDark>()
			|| Actor->IsA<ABazaBossBase>();
	}
}

ABazaShot::ABazaShot()
{
	// shot.tscn: RectangleShape2D, size (16, 8)
	DamageZone = CreateHitBox(TEXT("DamageZone"), FVector2D(16.f, 8.f));

	Bolt = CreateShape(TEXT("Bolt"), BazaMesh::Cylinder, FVector(4.f, 4.f, 16.f), FLinearColor(1.f, 0.95f, 0.3f));
	// Cylinders stand up the Z axis; lay it along the direction of travel.
	Bolt->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));

	Orb = CreateShape(TEXT("Orb"), BazaMesh::Sphere, FVector(6.f, 6.f, 6.f), FLinearColor(0.95f, 0.22f, 0.55f));
	Orb->SetVisibility(false);
}

void ABazaShot::BeginPlay()
{
	Super::BeginPlay();

	OnActorBeginOverlap.AddDynamic(this, &ABazaShot::HandleOverlap);
}

void ABazaShot::MakeMagnetic()
{
	bMagnetic = true;
	Bolt->SetVisibility(false);
	Orb->SetVisibility(true);
}

const ABazaActor* ABazaShot::FindMagnetTarget() const
{
	const FVector2D Position = GetGamePosition();

	const ABazaActor* Best = nullptr;
	float BestDistance = MagnetReach;

	for (TActorIterator<ABazaActor> It(GetWorld()); It; ++It)
	{
		const ABazaActor* Candidate = *It;
		if (Candidate->IsRemoved() || !IsEnemy(Candidate))
		{
			continue;
		}

		const FVector2D ToCandidate = Candidate->GetGamePosition() - Position;

		// Only things ahead of the orb: bending round to chase something already
		// behind it just makes the shot loop about aimlessly.
		if (FVector2D::DotProduct(ToCandidate, Direction) <= 0.f)
		{
			continue;
		}

		const float Distance = ToCandidate.Size();
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			Best = Candidate;
		}
	}

	return Best;
}

void ABazaShot::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FVector2D Position = GetGamePosition();

	if (bMagnetic)
	{
		if (const ABazaActor* Target = FindMagnetTarget())
		{
			// Turns towards the target at a capped rate, so an orb curves in rather
			// than snapping onto it.
			const FVector2D ToTarget = Target->GetGamePosition() - Position;

			const float Current = FMath::Atan2(Direction.Y, Direction.X);
			const float Wanted = FMath::Atan2(ToTarget.Y, ToTarget.X);
			const float MaxTurn = FMath::DegreesToRadians(MagnetTurnRate) * DeltaSeconds;
			const float Turn = FMath::Clamp(FMath::FindDeltaAngleRadians(Current, Wanted), -MaxTurn, MaxTurn);

			Direction = FVector2D(FMath::Cos(Current + Turn), FMath::Sin(Current + Turn));
		}
	}

	Position += Direction * MoveSpeed * DeltaSeconds;
	SetGamePosition(Position);

	// Points along its path, which matters once shots can leave at an angle.
	SetGameRotation(FMath::RadiansToDegrees(FMath::Atan2(Direction.Y, Direction.X)));

	const bool bOffScreen =
		Position.X < -8.f || Position.X > BazaSettings::ScreenWidth + 8.f ||
		Position.Y < -20.f || Position.Y > BazaSettings::ScreenHeight + 20.f;

	if (bOffScreen)
	{
		Destroy();
	}
}

void ABazaShot::HandleOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	// shot.gd meant to stop on asteroids and gunships, but low_nemy.tscn never
	// joined the "low_nemy" group, so in practice bolts sailed through every
	// enemy except asteroids - one bolt could kill a whole row, and bolts flew
	// straight through the boss. A bolt now spends itself on the first enemy it
	// touches.
	const ABazaActor* Target = Cast<ABazaActor>(OtherActor);
	if (!Target || Target->IsRemoved())
	{
		return;
	}

	if (IsEnemy(OtherActor))
	{
		Remove();
	}
}
