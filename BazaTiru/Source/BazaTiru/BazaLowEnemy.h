// Basa Tiru - the small gunship. Ported from scripts/lowney.gd.

#pragma once

#include "CoreMinimal.h"
#include "BazaActor.h"
#include "BazaLowEnemy.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class BAZATIRU_API ABazaLowEnemy : public ABazaActor
{
	GENERATED_BODY()

public:
	ABazaLowEnemy();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	/** lowney.gd: _on_shot_timer_timeout() */
	void OnShotTimerTimeout();

	/** lowney.gd: shoot() */
	void Shoot();

	/** lowney.gd: _on_area_entered(area) */
	UFUNCTION()
	void HandleOverlap(AActor* OverlappedActor, AActor* OtherActor);

private:
	UPROPERTY()
	TObjectPtr<USphereComponent> HitBox;

	UPROPERTY()
	TObjectPtr<class UProceduralMeshComponent> Body;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Cannon;

	FTimerHandle ShotTimer;

	float Speed = 100.f;

	/** The row it was spawned on; the weave is measured from there. */
	float BaseY = 0.f;

	float Drift = 0.f;

	float DriftPhase = 0.f;

	float DriftAmplitude = 14.f;

	/** lowney.gd: var enemy_id : int = randi() */
	int32 EnemyId = 0;

	bool bScoreEmitted = false;
};
