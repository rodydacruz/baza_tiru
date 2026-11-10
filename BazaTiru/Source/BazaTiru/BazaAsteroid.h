// Basa Tiru - asteroid. Ported from scripts/asteroid.gd.

#pragma once

#include "CoreMinimal.h"
#include "BazaActor.h"
#include "BazaAsteroid.generated.h"

class UProceduralMeshComponent;
class USphereComponent;

UCLASS()
class BAZATIRU_API ABazaAsteroid : public ABazaActor
{
	GENERATED_BODY()

public:
	ABazaAsteroid();

	virtual void Tick(float DeltaSeconds) override;

	/** Mean radius in game pixels. asteroid.tscn used a 6px collision circle. */
	UPROPERTY(EditAnywhere, Category = "Basa Tiru")
	float RadiusPx = 7.f;

	/** How far the rock strays from a sphere, as a fraction of its radius. */
	UPROPERTY(EditAnywhere, Category = "Basa Tiru")
	float Lumpiness = 0.32f;

protected:
	virtual void BeginPlay() override;

	/** asteroid.gd: _on_asteroid_area_entered(area) */
	UFUNCTION()
	void HandleOverlap(AActor* OverlappedActor, AActor* OtherActor);

private:
	UPROPERTY()
	TObjectPtr<USphereComponent> HitZone;

	UPROPERTY()
	TObjectPtr<UProceduralMeshComponent> Rock;

	float MoveSpeed = 100.f;

	/** asteroid.gd: var asteroid_id : int = randi() */
	int32 AsteroidId = 0;

	bool bScoreEmitted = false;

	/** Free tumble, so the rocks read as 3D rather than flat discs. */
	FRotator Spin = FRotator::ZeroRotator;
};
