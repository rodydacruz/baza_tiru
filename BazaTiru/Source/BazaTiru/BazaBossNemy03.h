// Basa Tiru - BossNemy03, "Barku Mai" (the mother ship). The last boss. New for
// the Unreal version.
//
// Taller than the screen, it does not fly in so much as close the right-hand side
// of the field like a wall. Only the core in the middle of its face can be hurt;
// the rest is armour, apart from four turrets that can be shot off. It never lets
// the player sit still: aimed fire, curtains of cannon balls with one gap in them,
// gunships out of its hangars and, at the end, a spiral out of the core. Every few
// seconds it rams the length of the screen along one half, and always leaves the
// other half open to escape into.

#pragma once

#include "CoreMinimal.h"
#include "BazaBossBase.h"
#include "BazaBossNemy03.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UCLASS()
class BAZATIRU_API ABazaBossNemy03 : public ABazaBossBase
{
	GENERATED_BODY()

public:
	ABazaBossNemy03();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void UpdateMovement(float DeltaSeconds) override;
	virtual void OnAttackTick() override;
	virtual void OnDamaged() override;
	virtual bool AcceptsShot(const ABazaShot* Shot) override;

private:
	/** The ram, step by step. None is the ordinary bobbing and shooting. */
	enum class EChargeStep : uint8
	{
		None,
		Aim,
		Ram,
		Hold,
		Recover
	};

	void UpdateCharge(float DeltaSeconds, FVector2D& Position);
	void BeginAim(bool bTop);
	void ShowLane(bool bVisible);

	/** Opens or closes the shutters over the core. Only an open core takes damage. */
	void SetCoreOpen(bool bOpen);

	void FireBall(const FVector2D& From, const FVector2D& Direction, float Speed);
	void FireFromTurret();
	void FireCurtain();
	void FireSpiral();
	void LaunchFighters();

	/** Screen row of a point on the hull, from its offset on the hull. */
	float RowOf(float OffsetY) const { return GetGamePosition().Y + OffsetY; }

	/** The screen X of the hull's front face. */
	float FaceX() const { return GetGamePosition().X - HullHalfWidth; }

	static constexpr int32 TurretCount = 4;
	static constexpr float HullHalfWidth = 45.f;

	UPROPERTY()
	TObjectPtr<UBoxComponent> HullZone;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Hull;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Armour;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Core;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> ShutterTop;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> ShutterBottom;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Ribs;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> TurretBases;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> TurretBarrels;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> HangarLights;

	/** The danger stripes laid over the half about to be rammed. */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> LaneStripes;

	float TurretHealth[TurretCount] = {};

	int32 NextTurret = 0;

	EChargeStep Charge = EChargeStep::None;

	float StepTime = 0.f;

	/** Seconds until the next ram. */
	float ChargeCooldown = 5.f;

	bool bChargeTop = true;

	/** The last phase sometimes follows a ram straight up with one down the other half. */
	bool bChainCharge = false;

	bool bCoreOpen = true;

	/** 0 shut, 1 open; eased so the shutters visibly slide. */
	float CoreOpenness = 1.f;

	float BobTime = 0.f;

	float FighterCooldown = 3.f;

	int32 AttackTicks = 0;

	float SpiralAngle = 0.f;

	float RumbleCooldown = 0.f;

	float Blink = 0.f;

	float DeathBlastCooldown = 0.f;
};
