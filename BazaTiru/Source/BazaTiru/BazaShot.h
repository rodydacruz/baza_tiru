// Basa Tiru - laser bolt, fired by the player and by the low enemies.
// Ported from scripts/shot.gd.

#pragma once

#include "CoreMinimal.h"
#include "BazaActor.h"
#include "BazaShot.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UCLASS()
class BAZATIRU_API ABazaShot : public ABazaActor
{
	GENERATED_BODY()

public:
	ABazaShot();

	virtual void Tick(float DeltaSeconds) override;

	/** shot.gd: @export var MOVE_SPEED := 500 */
	UPROPERTY(EditAnywhere, Category = "Basa Tiru")
	float MoveSpeed = 500.f;

	/** shot.gd: var DIRECTION := Vector2.ZERO, set by whoever fired it. */
	UPROPERTY(EditAnywhere, Category = "Basa Tiru")
	FVector2D Direction = FVector2D::ZeroVector;

	/**
	 * Set on the player's own bolts. player.gd treated every shot as a threat and
	 * got away with it only because its bolts always left from well clear of the
	 * hull; spread and magnet shots that start close or curve do not.
	 */
	bool bFiredByPlayer = false;

	/**
	 * The magnet power-up: the bolt becomes a small orb that bends towards the
	 * nearest enemy. Call straight after spawning.
	 */
	void MakeMagnetic();

	bool IsMagnetic() const { return bMagnetic; }

protected:
	virtual void BeginPlay() override;

	/** shot.gd: _on_shot_area_entered(area) */
	UFUNCTION()
	void HandleOverlap(AActor* OverlappedActor, AActor* OtherActor);

	/** The closest enemy worth chasing, or null if nothing is in reach. */
	const ABazaActor* FindMagnetTarget() const;

private:
	UPROPERTY()
	TObjectPtr<UBoxComponent> DamageZone;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Bolt;

	/** The magnet round: hidden until MakeMagnetic swaps it in for the bolt. */
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Orb;

	bool bMagnetic = false;

	/** How hard a magnet orb can bend, in degrees per second. */
	static constexpr float MagnetTurnRate = 420.f;

	/** How far, in game pixels, a magnet orb can feel an enemy. */
	static constexpr float MagnetReach = 150.f;
};
