// Basa Tiru - power-up drops. New for the Unreal version.
//
// Pickups drift in from the right like everything else, bobbing and turning so
// their 3D shape reads: a hexagon for the shield, a double chevron for speed, a
// gear for repair, a horseshoe magnet for homing shots and a fan of arrowheads for
// the spread. Flying into one hands its effect to the player.

#pragma once

#include "CoreMinimal.h"
#include "BazaActor.h"
#include "BazaPowerUp.generated.h"

class UProceduralMeshComponent;
class USphereComponent;
class UStaticMeshComponent;

UENUM()
enum class EBazaPowerUp : uint8
{
	/** A hexagonal ring round the ship that eats incoming fire. */
	Shield,

	/** Faster ship, and double points while it lasts. */
	Speed,

	/** Lives back to full. */
	Repair,

	/** Bolts become small orbs that bend towards the nearest enemy. */
	Magnet,

	/** Two extra bolts, angled out either side. */
	Spread,

	Count UMETA(Hidden)
};

namespace BazaPowerUps
{
	FLinearColor ColourOf(EBazaPowerUp Type);

	/** What the HUD calls it. */
	const TCHAR* LabelOf(EBazaPowerUp Type);

	/** Seconds the effect lasts; zero for one that happens once and is done. */
	float DurationOf(EBazaPowerUp Type);
}

UCLASS()
class BAZATIRU_API ABazaPowerUp : public ABazaActor
{
	GENERATED_BODY()

public:
	ABazaPowerUp();

	virtual void Tick(float DeltaSeconds) override;

	/** Sets which pickup this is and builds its shape. Call straight after spawning. */
	void Configure(EBazaPowerUp InType);

	EBazaPowerUp GetType() const { return Type; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleOverlap(AActor* OverlappedActor, AActor* OtherActor);

private:
	UPROPERTY()
	TObjectPtr<USphereComponent> HitZone;

	UPROPERTY()
	TObjectPtr<UProceduralMeshComponent> Body;

	/** The magnet's silver pole tips; hidden on every other pickup. */
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> TipLeft;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> TipRight;

	EBazaPowerUp Type = EBazaPowerUp::Shield;

	float BaseY = 0.f;

	float Age = 0.f;

	bool bCollected = false;

	/** Slower than the rocks, so there is time to go and get one. */
	static constexpr float DriftSpeed = 45.f;
};
