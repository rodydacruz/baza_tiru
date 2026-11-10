#include "BazaWarning.h"

#include "BazaCanonBall.h"
#include "BazaGameMode.h"
#include "Components/StaticMeshComponent.h"

ABazaWarning::ABazaWarning()
{
	// A marker only; it never collides with anything.
	Arrow = CreateShape(TEXT("Arrow"), BazaMesh::Cone, FVector(10.f, 10.f, 14.f), FLinearColor(1.f, 0.15f, 0.15f));
}

void ABazaWarning::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const ABazaGameMode* Stage = GetStage();
	const ABazaCanonBall* Ball = Threat.Get();

	// The marker only lives while its cannon ball is still out beyond the left edge.
	if (!Ball || !Stage || Stage->IsGameOver() || Ball->GetGamePosition().X >= -10.f)
	{
		Destroy();
		return;
	}

	const FVector2D PlayerPosition = Stage->GetPlayerPosition();
	const FVector2D ThreatPosition = Ball->GetGamePosition();

	// t_warning.gd: move_to_the_middle - halfway up the gap between the two.
	FVector2D Position = GetGamePosition();
	Position.Y = ThreatPosition.Y + (PlayerPosition.Y - ThreatPosition.Y) * 0.5f;
	SetGamePosition(Position);

	SetGameRotation(-GetAngleBetweenTwoVectors(ThreatPosition, Position));
}

float ABazaWarning::GetAngleBetweenTwoVectors(const FVector2D& First, const FVector2D& Second)
{
	const FVector2D Arrow = Second - First;

	// Godot's Vector2.angle_to(Vector2(0, 1)) is atan2(cross, dot) against "down",
	// which for a down vector reduces to atan2(Arrow.X, Arrow.Y).
	const float Radians = FMath::Atan2(Arrow.X, Arrow.Y);

	return FMath::Abs(FMath::RadiansToDegrees(Radians));
}
