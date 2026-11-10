// Basa Tiru - shared machinery for every boss.
//
// boss_nemy_01.gd had the health, the phase switch, the health bar and the hit
// reaction tangled together with BossNemy01's own attack pattern. Splitting the
// two means a second boss only has to describe how it moves and how it shoots.

#pragma once

#include "CoreMinimal.h"
#include "BazaActor.h"
#include "BazaBossBase.generated.h"

class ABazaShot;
class USphereComponent;
class UStaticMeshComponent;

UCLASS(Abstract)
class BAZATIRU_API ABazaBossBase : public ABazaActor
{
	GENERATED_BODY()

public:
	ABazaBossBase();

	virtual void Tick(float DeltaSeconds) override;

	EBazaPhase GetPhase() const { return CurrentPhase; }
	float GetHealth() const { return Health; }
	float GetHealthFraction() const { return TotalHealth > 0.f ? Health / TotalHealth : 0.f; }
	int32 GetStrikeCount() const { return StrikeCount; }

	/** Shown on the HUD while the boss is still sweeping onto the field. */
	bool IsEntering() const { return CurrentPhase == EBazaPhase::Entering; }

	/** Development aid: jumps the fight on to its next phase (F4). */
	void DebugSkipPhase();

	/** The name the HUD announces it by. */
	UPROPERTY(EditDefaultsOnly, Category = "Basa Tiru|Boss")
	FText DisplayName;

protected:
	virtual void BeginPlay() override;

	// --- what a subclass fills in ---

	/** Runs every frame once the boss has finished arriving. */
	virtual void UpdateMovement(float DeltaSeconds) {}

	/** Runs on the attack timer; phase is whatever the subclass last set. */
	virtual void OnAttackTick() {}

	/** Called after every hit, so a subclass can change phase on a health threshold. */
	virtual void OnDamaged() {}

	/** Called once the entrance sweep finishes, before the first attack. */
	virtual void OnEntranceComplete() {}

	/**
	 * Whether a bolt that touched the boss counts against its health. A boss with
	 * armour or parts of its own says no here and deals with the bolt itself.
	 */
	virtual bool AcceptsShot(const ABazaShot* Shot) { return true; }

	// --- shared behaviour ---

	/** boss_nemy_01.gd: _on_area_entered(area), minus the phase-specific parts. */
	UFUNCTION()
	void HandleOverlap(AActor* OverlappedActor, AActor* OtherActor);

	/** boss_nemy_01.gd: startHitAnimation() */
	void StartHitAnimation();

	/** boss_nemy_01.gd: _draw_health() */
	void DrawHealth();

	void BeginDefeat();

	/** Set up by the subclass constructor. */
	void ConfigureBoss(float InTotalHealth, float InHitRadiusPx, float InHealthBarOffsetPx);

	UPROPERTY()
	TObjectPtr<USphereComponent> HitZone;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> HealthBar;

	EBazaPhase CurrentPhase = EBazaPhase::Entering;

	/** Where the entrance sweep stops and the fight proper begins. */
	float EntryTargetX = BazaSettings::ScreenWidth - 20.f;

	float EntrySpeed = 90.f;

	/**
	 * How often OnAttackTick fires. boss_nemy01.tscn used half a second. Set it
	 * freely in a constructor; once the fight is running, go through
	 * SetAttackInterval so the timer is actually re-armed.
	 */
	float AttackInterval = 0.5f;

	/** Changes the firing rate mid-fight, for a phase that speeds up. */
	void SetAttackInterval(float NewInterval);

	/** Points for a single hit, and for finally putting it down. */
	int32 HitScore = 10;

	int32 DefeatScore = 250;

	float Damage = 10.f;

	int32 StrikeCount = 0;

	float TotalHealth = 1000.f;

	float Health = 1000.f;

	/** boss_nemy_01.gd: var enemy_id : int = randi() */
	int32 EnemyId = 0;

	float HitRadiusPx = 36.f;

	/** How long the death throes last before the boss finally goes. */
	float DefeatDuration = 1.5f;

private:
	void RunEntrance(float DeltaSeconds);
	void RunDefeat(float DeltaSeconds);
	void ArmAttackTimer();
	void HandleAttackTimer();

	FTimerHandle AttackTimer;

	FVector HealthBarScale = FVector::OneVector;

	float DefeatElapsed = 0.f;

	float NextDeathBlast = 0.f;
};
