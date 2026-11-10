// Basa Tiru - the "dark", the swarm that fills the field after score 20.
// Ported from scripts/dark.gd.

#pragma once

#include "CoreMinimal.h"
#include "BazaActor.h"
#include "BazaDark.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UCLASS()
class BAZATIRU_API ABazaDark : public ABazaActor
{
	GENERATED_BODY()

public:
	ABazaDark();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	/** dark.gd: shoot() */
	void Shoot();

	/** dark.gd: _on_area_entered(area) */
	UFUNCTION()
	void HandleOverlap(AActor* OverlappedActor, AActor* OtherActor);

	/** The stage counts these; it must be told exactly once, however this one goes. */
	void ReleaseSlot();

private:
	UPROPERTY()
	TObjectPtr<UBoxComponent> HitBox;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Shell;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Core;

	float MoveSpeed = 100.f;

	/**
	 * dark.gd gave these no health at all: a dark scored a point when hit, shot
	 * back, and carried on regardless, so the only way to be rid of one was to
	 * let it cross the screen. Three hits now puts one down.
	 */
	int32 Health = 3;

	/** dark.gd: var asteroid_id : int = randi() */
	int32 DarkId = 0;

	bool bSlotReleased = false;

	float Spin = 0.f;
};
