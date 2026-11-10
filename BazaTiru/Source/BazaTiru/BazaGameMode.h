// Basa Tiru - the stage. Ported from scripts/stage.gd.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BazaActor.h"
#include "BazaSettings.h"
#include "Engine/World.h"
#include "BazaGameMode.generated.h"

class ABazaBossBase;
class ABazaPlayer;
class ABazaPowerUp;
class ACameraActor;
enum class EBazaPowerUp : uint8;

/**
 * Godot's stage.tscn root: it owns the spawn timer, the score, the difficulty
 * phases and the game-over flag, and it is the "get_parent()" every ported actor
 * reaches for.
 */
UCLASS()
class BAZATIRU_API ABazaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABazaGameMode();

	/** The ported stand-in for Godot's get_parent() on a stage child. */
	static ABazaGameMode* Get(const UObject* WorldContext);

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	bool IsGameOver() const { return bIsGameOver; }
	int32 GetScore() const { return Score; }
	int32 GetLives() const { return Lives; }

	/** How many scripted encounters are still on the field; nothing new spawns above zero. */
	int32 GetEvents() const { return Events; }

	const ABazaBossBase* GetBoss() const { return Boss.Get(); }

	/** stage.gd: getPlayerPosition() */
	FVector2D GetPlayerPosition() const;

	/** Takes a life; false once there are none left to take. */
	bool ConsumeLife();

	/** The repair power-up. */
	void RefillLives() { Lives = StartingLives; }

	ABazaPlayer* GetPlayer() const;

	/** Drops a pickup of the given kind at a game-space position. */
	ABazaPowerUp* SpawnPowerUp(EBazaPowerUp Type, const FVector2D& Where);

	/** How long, at random between these, until the next pickup drifts in. */
	UPROPERTY(EditDefaultsOnly, Category = "Basa Tiru|Power-ups")
	float PowerUpMinInterval = 4.f;

	UPROPERTY(EditDefaultsOnly, Category = "Basa Tiru|Power-ups")
	float PowerUpMaxInterval = 7.f;

	/** player.gd would emit "destroyed" here, if the original had not commented it out. */
	void OnPlayerDestroyed();

	void OnBossDefeated();

	/** dark.gd: get_parent().events -= 1 when a dark leaves the screen. */
	void ReleaseEventSlot();

	/** Knocks the camera about. Bigger numbers for bigger deaths. */
	void AddShake(float Amount);

	UFUNCTION()
	void HandleScore(int32 Points, int32 EnemyId);

	/**
	 * The original game never actually killed the player: player.gd spawns an
	 * explosion but its queue_free() and "destroyed" signal are commented out, so
	 * the retry screen is unreachable. Tick this to play the game exactly as it
	 * was, with an indestructible ship and no way to lose.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Basa Tiru")
	bool bPlayerIsInvulnerable = false;

	UPROPERTY(EditDefaultsOnly, Category = "Basa Tiru")
	int32 StartingLives = 3;

	/** Horizontal field of view; the camera distance is derived so 320x180 fills the frame. */
	UPROPERTY(EditDefaultsOnly, Category = "Basa Tiru|Camera")
	float CameraFov = 50.f;

	/** Adds a light and a starfield so the game is visible in a blank level. */
	UPROPERTY(EditDefaultsOnly, Category = "Basa Tiru|Camera")
	bool bSpawnDefaultScenery = true;

	/**
	 * stage.gd left an executeCheatCode() stub and never filled it in. These are
	 * it: F1 shows the stage's counters, F2 adds score, F3 calls the boss down
	 * early so its phases can be watched without grinding to a hundred points.
	 */
	void CheatAddScore(int32 Points);
	void CheatSpawnBoss();
	void CheatSkipBossPhase();

	/** F5: drops each kind of pickup in turn, just ahead of the ship. */
	void CheatSpawnPowerUp();

	/**
	 * The bosses, in the order they turn up, with the score each one waits for.
	 * Adding a third is two entries here and one new ABazaBossBase subclass.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Basa Tiru|Bosses")
	TArray<TSubclassOf<ABazaBossBase>> BossRoster;

	UPROPERTY(EditDefaultsOnly, Category = "Basa Tiru|Bosses")
	TArray<int32> BossScoreGates;

	UPROPERTY(EditDefaultsOnly, Category = "Basa Tiru|Debug")
	bool bShowDebug = false;

	/** Spawns a Basa Tiru actor at a Godot-space position and hooks up its score signal. */
	template <typename T>
	T* SpawnAt(const FVector2D& Where)
	{
		UWorld* World = GetWorld();
		if (!World)
		{
			return nullptr;
		}

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		T* Spawned = World->SpawnActor<T>(
			T::StaticClass(), BazaSettings::ToWorld(Where), FRotator::ZeroRotator, Params);

		if (Spawned)
		{
			Spawned->SetGamePosition(Where);
			Spawned->OnScore.AddDynamic(this, &ABazaGameMode::HandleScore);
		}

		return Spawned;
	}

protected:
	/** stage.gd: _on_spawn_timer_timeout() */
	void OnSpawnTimerTimeout();

	/** stage.gd: _runScorePhase(score) */
	void RunScorePhase();

	void SchedulePowerUp();
	void OnPowerUpTimer();

	/** Weighted pick; repair only turns up when a life has actually been lost. */
	EBazaPowerUp PickPowerUp() const;

	void EnsureCamera();
	void SpawnScenery();

	/** Wires up -BazaShot=<seconds>, a dev-only "play, screenshot, quit". */
	void SetUpCaptureMode();

	/** How far back the camera has to sit for the 320x180 field to fill the frame. */
	float ComputeCameraDistance() const;

private:
	UPROPERTY()
	TObjectPtr<ACameraActor> Camera;

	UPROPERTY()
	TObjectPtr<ABazaPlayer> Player;

	TWeakObjectPtr<ABazaBossBase> Boss;

	FTimerHandle SpawnTimer;

	FTimerHandle PowerUpTimer;

	/** Which kind F5 drops next. */
	int32 NextCheatPowerUp = 0;

	FVector CameraHome = FVector::ZeroVector;

	float Shake = 0.f;

	int32 Score = 0;

	int32 Lives = 3;

	/** stage.gd: how many scripted encounters are still on the field. */
	int32 Events = 0;

	bool bIsGameOver = false;

	/** How far down BossRoster the run has got. */
	int32 BossesSpawned = 0;
};
