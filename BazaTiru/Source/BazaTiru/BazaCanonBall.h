// Basa Tiru - the boss's cannon ball. Ported from scripts/tiro.gd
// (scene: scenes/canon_ball.tscn).

#pragma once

#include "CoreMinimal.h"
#include "BazaActor.h"
#include "BazaCanonBall.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UCLASS()
class BAZATIRU_API ABazaCanonBall : public ABazaActor
{
	GENERATED_BODY()

public:
	ABazaCanonBall();

	virtual void Tick(float DeltaSeconds) override;

	/**
	 * tiro.gd: var vector := Vector2(-1,0)
	 *
	 * The boss assigns the raw difference to the player without normalising it and
	 * without a speed multiplier, so a ball fired from far away travels much faster
	 * than a close one. That is what the game does today, so the port keeps it.
	 */
	UPROPERTY(EditAnywhere, Category = "Basa Tiru")
	FVector2D Velocity = FVector2D(-1.f, 0.f);

	/** Balls fired during the HIDE phase are allowed off the left edge and come back. */
	UPROPERTY(EditAnywhere, Category = "Basa Tiru")
	EBazaPhase Phase = EBazaPhase::Appear;

private:
	UPROPERTY()
	TObjectPtr<UBoxComponent> DamageZone;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Ball;
};
