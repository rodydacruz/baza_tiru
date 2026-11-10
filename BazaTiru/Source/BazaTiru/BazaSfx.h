// Basa Tiru - sound effects, synthesised in code.
//
// The Godot game had no audio at all. Rather than ship a folder of .wav files,
// every effect here is rendered once from a few lines of maths - pulse waves and
// sample-and-hold noise, the same two ingredients an NES sound chip had - and
// played back through a procedural sound wave. Swapping in recorded sounds later
// only means changing what Play() hands to the audio engine.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BazaSfx.generated.h"

class UAudioComponent;

UENUM()
enum class EBazaSfx : uint8
{
	Shoot,
	EnemyShot,
	Cannon,
	Hit,
	ExplosionSmall,
	ExplosionBig,
	PlayerHit,
	Respawn,
	BossWarning,
	BossDefeated,
	GameOver,
	PowerUp,

	Count UMETA(Hidden)
};

UCLASS()
class BAZATIRU_API UBazaSfx : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Fire and forget, from anything that lives in a game world. */
	static void Play(const UObject* WorldContext, EBazaSfx Sound, float Volume = 1.f);

	static UBazaSfx* Get(const UObject* WorldContext);

	void PlaySound(EBazaSfx Sound, float Volume = 1.f);

	void ToggleMute() { bMuted = !bMuted; }
	bool IsMuted() const { return bMuted; }

	/** Everything is scaled by this; 8-bit square waves are loud at full scale. */
	float MasterVolume = 0.4f;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	/** Renders every clip the first time anything asks for a sound. */
	void EnsureRendered();

	static constexpr int32 SampleRate = 22050;

	/** More voices than this and the small stuff is dropped, so a storm stays a noise rather than a wall. */
	static constexpr int32 MaxVoices = 20;

	/** One buffer of 16-bit mono PCM per EBazaSfx. */
	TArray<TArray<uint8>> Clips;

	TArray<float> Durations;

	/** Real time each sound last started, so ten bolts in a frame make one sound. */
	TArray<double> LastPlayed;

	UPROPERTY()
	TArray<TObjectPtr<UAudioComponent>> Voices;

	bool bMuted = false;
};
