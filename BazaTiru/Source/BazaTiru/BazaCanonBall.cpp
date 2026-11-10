#include "BazaCanonBall.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"

ABazaCanonBall::ABazaCanonBall()
{
	// canon_ball.tscn: RectangleShape2D, size (16, 8)
	DamageZone = CreateHitBox(TEXT("DamageZone"), FVector2D(16.f, 8.f));

	Ball = CreateShape(TEXT("Ball"), BazaMesh::Sphere, FVector(12.f, 12.f, 12.f), FLinearColor(1.f, 0.35f, 0.f));

	// tiro.gd has an _on_shot_area_entered, but canon_ball.tscn never wires the
	// area_entered signal to it, so a cannon ball passes through everything. The
	// player is the one that notices the overlap, in player.gd.
}

void ABazaCanonBall::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FVector2D Position = GetGamePosition();
	Position += Velocity * DeltaSeconds;
	SetGamePosition(Position);

	const bool bLeftTheScreen = Position.X < -8.f && Phase != EBazaPhase::Hide;
	// Out through the top or bottom too, now that balls can be thrown at steep
	// angles - otherwise they flew on forever.
	const bool bLeftVertically = Position.Y < -20.f || Position.Y > BazaSettings::ScreenHeight + 20.f;

	if (bLeftTheScreen || bLeftVertically || Position.X > BazaSettings::ScreenWidth + 8.f)
	{
		Destroy();
	}
}
