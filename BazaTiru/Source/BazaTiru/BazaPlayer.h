// Basa Tiru - the player ship. Ported from scripts/player.gd.

#pragma once

#include "CoreMinimal.h"
#include "BazaActor.h"
#include "BazaPlayer.generated.h"

class USphereComponent;
class UStaticMeshComponent;
enum class EBazaPowerUp : uint8;

UCLASS()
class BAZATIRU_API ABazaPlayer : public ABazaActor
{
	GENERATED_BODY()

public:
	ABazaPlayer();

	virtual void Tick(float DeltaSeconds) override;

	static constexpr float MoveSpeed = 150.f;

	/** Where stage.tscn parked the ship, and where it comes back after dying. */
	static const FVector2D StartPosition;

	bool IsInvulnerable() const { return InvulnerableRemaining > 0.f; }

	/** A pickup has been flown into. */
	void ApplyPowerUp(EBazaPowerUp Type);

	/** Seconds left on a timed power-up; zero when it is not running. */
	float GetPowerUpRemaining(EBazaPowerUp Type) const;

	/** Speed doubles every point scored while it runs. */
	int32 GetScoreMultiplier() const { return SpeedRemaining > 0.f ? 2 : 1; }

	/** Counts every trigger pull, so a boss can notice a volley and copy it. */
	int32 GetVolleyCount() const { return VolleyCount; }

	/** +1 facing right, -1 facing left. */
	float GetFacing() const { return Facing; }

protected:
	virtual void BeginPlay() override;

	/** player.gd: _on_player_area_entered(area) */
	UFUNCTION()
	void HandleOverlap(AActor* OverlappedActor, AActor* OtherActor);

	/** player.gd: _on_reload_timer_timeout() */
	void OnReloadTimerTimeout();

	void Shoot();

	/** The death player.gd never got round to wiring up. */
	void TakeHit();
	void Respawn();

private:
	UPROPERTY()
	TObjectPtr<USphereComponent> HitZone;

	UPROPERTY()
	TObjectPtr<class UProceduralMeshComponent> Hull;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Thruster;

	/** The shield power-up: a hexagonal frame turning round the ship. */
	UPROPERTY()
	TObjectPtr<class UProceduralMeshComponent> ShieldRing;

	float ShieldRemaining = 0.f;
	float SpeedRemaining = 0.f;
	float MagnetRemaining = 0.f;
	float SpreadRemaining = 0.f;

	float ShieldSpin = 0.f;

	/** Capture mode's -BazaAutoFire: fire as if space were held. */
	bool bAutoFire = false;

	FTimerHandle ReloadTimer;

	FTimerHandle RespawnTimer;

	bool bCanShot = true;

	int32 VolleyCount = 0;

	/** Hidden and untouchable between dying and coming back. */
	bool bDead = false;

	float InvulnerableRemaining = 0.f;

	/** How far the hull is leaned over, in degrees, eased towards the input. */
	float Bank = 0.f;

	/** +1 facing right, -1 facing left. Left only while the left arrow is held. */
	float Facing = 1.f;

	/** 0 fully facing right, 1 fully facing left; eased so the turn is visible. */
	float FlipProgress = 0.f;

	float ThrusterPhase = 0.f;
};
