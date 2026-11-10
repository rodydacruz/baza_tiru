// Basa Tiru - BossNemy01, the first boss. Ported from scripts/boss_nemy_01.gd.
//
// Everything shared with future bosses lives in ABazaBossBase; what is left here
// is this one's attack pattern: close in, aim at the player, run off the left
// edge lobbing shots back over its shoulder, then turn around and charge.

#pragma once

#include "CoreMinimal.h"
#include "BazaBossBase.h"
#include "BazaBossNemy01.generated.h"

class UStaticMeshComponent;

UCLASS()
class BAZATIRU_API ABazaBossNemy01 : public ABazaBossBase
{
	GENERATED_BODY()

public:
	ABazaBossNemy01();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void UpdateMovement(float DeltaSeconds) override;
	virtual void OnAttackTick() override;
	virtual void OnDamaged() override;

	/** boss_nemy_01.gd: shootCanonBall(), with an optional angle off the direct line. */
	void ShootCanonBall(float SpreadDegrees = 0.f);

private:
	UPROPERTY()
	TObjectPtr<class UProceduralMeshComponent> Hull;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Prow;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Ring;

	/** boss_nemy_01.gd: var speed = 60 */
	float Speed = 60.f;

	float RingSpin = 0.f;

	/** Set once the charge back in from the left has reached its spot. */
	bool bAllInArrived = false;

	float AllInTime = 0.f;

	int32 AllInVolleys = 0;

	/** What the boss is doing once it holds the left side. */
	enum class EAllInMove : uint8
	{
		Hold,		// tracking the player's row and firing
		WindUp,		// flashing white and drawing back: the tell
		Dash,		// ramming along a line fixed at the end of the tell
		Return		// easing back to its spot
	};

	EAllInMove AllInMove = EAllInMove::Hold;

	float AllInMoveTime = 0.f;

	FVector2D DashDirection = FVector2D(1.f, 0.f);

	static constexpr float HoldTime = 2.6f;
	static constexpr float WindUpTime = 0.5f;
	static constexpr float DashTime = 0.75f;
	static constexpr float DashSpeed = 360.f;
};
