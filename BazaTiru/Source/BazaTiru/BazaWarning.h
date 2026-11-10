// Basa Tiru - the incoming-threat marker. Ported from scenes/t_warning.gd.

#pragma once

#include "CoreMinimal.h"
#include "BazaActor.h"
#include "BazaWarning.generated.h"

class ABazaCanonBall;
class UStaticMeshComponent;

/**
 * Sits on the left edge while the boss is hidden off-screen, halfway between the
 * player and the cannon ball that is about to come back, tilted to point at it.
 */
UCLASS()
class BAZATIRU_API ABazaWarning : public ABazaActor
{
	GENERATED_BODY()

public:
	ABazaWarning();

	virtual void Tick(float DeltaSeconds) override;

	/** t_warning.gd: var threat = null */
	UPROPERTY()
	TWeakObjectPtr<ABazaCanonBall> Threat;

protected:
	/** t_warning.gd: getAngleBetweenTwoVector(vector1, vector2) */
	static float GetAngleBetweenTwoVectors(const FVector2D& First, const FVector2D& Second);

private:
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Arrow;
};
