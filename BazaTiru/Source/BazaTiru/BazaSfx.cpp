#include "BazaSfx.h"

#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWaveProcedural.h"
#include "TimerManager.h"

namespace
{
	/** Shortest gap between two starts of the same sound, in seconds. */
	float MinGap(EBazaSfx Sound)
	{
		switch (Sound)
		{
		case EBazaSfx::Shoot:          return 0.05f;
		case EBazaSfx::EnemyShot:      return 0.08f;
		case EBazaSfx::Cannon:         return 0.06f;
		case EBazaSfx::Hit:            return 0.04f;
		case EBazaSfx::ExplosionSmall: return 0.05f;
		default:                       return 0.f;
		}
	}

	/** Sounds that always get a voice, and are never pitch-wobbled. */
	bool IsCue(EBazaSfx Sound)
	{
		switch (Sound)
		{
		case EBazaSfx::ExplosionBig:
		case EBazaSfx::PlayerHit:
		case EBazaSfx::Respawn:
		case EBazaSfx::BossWarning:
		case EBazaSfx::BossDefeated:
		case EBazaSfx::GameOver:
			return true;
		default:
			return false;
		}
	}

	/** A pulse wave: +1 for Duty of each cycle, -1 for the rest. 0.125 and 0.25 are the NES voices. */
	float Pulse(float Phase, float Duty)
	{
		return FMath::Frac(Phase) < Duty ? 1.f : -1.f;
	}

	float Triangle(float Phase)
	{
		return 4.f * FMath::Abs(FMath::Frac(Phase) - 0.5f) - 1.f;
	}

	/** Falls from 1 to 0 across the clip; higher Curve dies away faster. */
	float Decay(float Progress, float Curve)
	{
		return FMath::Pow(FMath::Max(1.f - Progress, 0.f), Curve);
	}

	/** A few milliseconds of fade-in, so nothing starts with a click. */
	float Attack(float Seconds)
	{
		return FMath::Min(1.f, Seconds / 0.003f);
	}

	/**
	 * Sample-and-hold noise: a random level held for Period samples, then another.
	 * A long hold sounds low and rumbling, a short one hisses. It is how the NES
	 * noise channel made its explosions.
	 */
	struct FNoise
	{
		FRandomStream Random;
		float Value = 0.f;
		int32 Hold = 0;

		explicit FNoise(int32 Seed) : Random(Seed) {}

		float Next(int32 Period)
		{
			if (--Hold <= 0)
			{
				Hold = FMath::Max(Period, 1);
				Value = Random.FRandRange(-1.f, 1.f);
			}
			return Value;
		}
	};
}

UBazaSfx* UBazaSfx::Get(const UObject* WorldContext)
{
	if (!WorldContext || !GEngine)
	{
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull);
	return World ? World->GetSubsystem<UBazaSfx>() : nullptr;
}

void UBazaSfx::Play(const UObject* WorldContext, EBazaSfx Sound, float Volume)
{
	if (UBazaSfx* Sfx = Get(WorldContext))
	{
		Sfx->PlaySound(Sound, Volume);
	}
}

bool UBazaSfx::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Only where the game actually runs; the editor's own preview worlds stay quiet.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UBazaSfx::PlaySound(EBazaSfx Sound, float Volume)
{
	UWorld* World = GetWorld();
	if (bMuted || !World)
	{
		return;
	}

	EnsureRendered();

	const int32 Index = static_cast<int32>(Sound);
	if (!Clips.IsValidIndex(Index) || Clips[Index].Num() == 0)
	{
		return;
	}

	const double Now = World->GetRealTimeSeconds();
	if (Now - LastPlayed[Index] < MinGap(Sound))
	{
		return;
	}

	Voices.RemoveAll([](const TObjectPtr<UAudioComponent>& Voice)
	{
		return !IsValid(Voice) || !Voice->IsPlaying();
	});

	if (Voices.Num() >= MaxVoices && !IsCue(Sound))
	{
		return;
	}

	LastPlayed[Index] = Now;

	// A procedural wave is consumed as it plays, so each start gets its own,
	// filled from the cached clip.
	USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(this);
	Wave->SetSampleRate(SampleRate);
	Wave->NumChannels = 1;
	Wave->Duration = Durations[Index];
	Wave->SoundGroup = SOUNDGROUP_Default;
	Wave->bLooping = false;
	Wave->QueueAudio(Clips[Index].GetData(), Clips[Index].Num());

	// A little pitch wobble so repeated shots and blasts do not sound stamped out.
	// Cues are tunes, and stay in tune. The player's own shot wobbles least: it is
	// the one heard constantly, and a wide wobble on it sounds sloppy, not varied.
	float Pitch = 1.f;
	if (Sound == EBazaSfx::Shoot)
	{
		Pitch = FMath::FRandRange(0.97f, 1.03f);
	}
	else if (!IsCue(Sound))
	{
		Pitch = FMath::FRandRange(0.93f, 1.07f);
	}

	UAudioComponent* Voice = UGameplayStatics::SpawnSound2D(
		World, Wave, MasterVolume * Volume, Pitch, 0.f, nullptr,
		/*bPersistAcrossLevelTransition=*/false, /*bAutoDestroy=*/true);

	if (!Voice)
	{
		return;
	}

	Voices.Add(Voice);

	// A procedural wave has no natural end to its data as far as the mixer is
	// concerned, so each voice is stopped once its clip has run out.
	FTimerHandle StopHandle;
	World->GetTimerManager().SetTimer(
		StopHandle,
		FTimerDelegate::CreateWeakLambda(Voice, [Voice]() { Voice->Stop(); }),
		Durations[Index] / Pitch + 0.1f,
		false);
}

void UBazaSfx::EnsureRendered()
{
	if (Clips.Num() > 0)
	{
		return;
	}

	const int32 Count = static_cast<int32>(EBazaSfx::Count);
	Clips.SetNum(Count);
	Durations.SetNumZeroed(Count);
	LastPlayed.Init(-1000.0, Count);

	// Renders one clip: Voice is asked for a sample at each step, given the index,
	// the time in seconds and how far through the clip it is (0 to 1).
	auto Render = [this](EBazaSfx Sound, float Seconds, TFunctionRef<float(float T, float N)> Voice)
	{
		const int32 SampleCount = FMath::CeilToInt(Seconds * SampleRate);
		TArray<uint8>& Bytes = Clips[static_cast<int32>(Sound)];
		Bytes.SetNumUninitialized(SampleCount * sizeof(int16));

		int16* Out = reinterpret_cast<int16*>(Bytes.GetData());

		for (int32 Index = 0; Index < SampleCount; ++Index)
		{
			const float T = static_cast<float>(Index) / SampleRate;
			const float N = static_cast<float>(Index) / SampleCount;

			float Sample = FMath::Clamp(Voice(T, N), -1.f, 1.f);

			// Coarse amplitude steps: the crunch that makes it sound 8-bit rather
			// than merely synthesised.
			Sample = FMath::RoundToFloat(Sample * 24.f) / 24.f;

			Out[Index] = static_cast<int16>(Sample * 30000.f);
		}

		Durations[static_cast<int32>(Sound)] = Seconds;
	};

	// Player shot: heard five times a second for the whole game, so it has to sit
	// back. The first version, a narrow pulse diving from 1500 Hz, was bright and
	// nasal and wore on the ear. This one is a soft, low "pew": mostly triangle,
	// the gentlest NES voice, with a little square for bite and a few milliseconds
	// of noise at the front for the snap, dying away fast.
	{
		float Phase = 0.f;
		FNoise Noise(5);
		Render(EBazaSfx::Shoot, 0.07f, [&](float T, float N)
		{
			Phase += FMath::Lerp(620.f, 240.f, FMath::Sqrt(N)) / SampleRate;

			const float Tone = Triangle(Phase) * 0.75f + Pulse(Phase, 0.5f) * 0.25f;
			const float Snap = T < 0.006f ? Noise.Next(3) * 0.5f : 0.f;

			return (Tone + Snap) * Attack(T) * Decay(N, 2.2f) * 0.3f;
		});
	}

	// Gunship and dark bolts: lower and duller than the player's, so the ear can
	// tell whose shot that was.
	{
		float Phase = 0.f;
		Render(EBazaSfx::EnemyShot, 0.12f, [&](float T, float N)
		{
			Phase += FMath::Lerp(720.f, 280.f, N) / SampleRate;
			return Pulse(Phase, 0.5f) * Attack(T) * Decay(N, 1.2f) * 0.3f;
		});
	}

	// Boss cannon: a thump with some grit in it.
	{
		float Phase = 0.f;
		FNoise Noise(11);
		Render(EBazaSfx::Cannon, 0.22f, [&](float T, float N)
		{
			Phase += FMath::Lerp(240.f, 70.f, N) / SampleRate;
			const float Body = Pulse(Phase, 0.5f) * 0.7f + Noise.Next(12) * 0.3f;
			return Body * Attack(T) * Decay(N, 1.6f) * 0.5f;
		});
	}

	// A shot landing on something that did not die: a short tick.
	{
		float Phase = 0.f;
		FNoise Noise(23);
		Render(EBazaSfx::Hit, 0.06f, [&](float T, float N)
		{
			Phase += 950.f / SampleRate;
			return (Pulse(Phase, 0.5f) * 0.5f + Noise.Next(2) * 0.5f) * Attack(T) * Decay(N, 2.f) * 0.35f;
		});
	}

	// Small blast: hissing noise that drops in pitch as it dies.
	{
		FNoise Noise(37);
		Render(EBazaSfx::ExplosionSmall, 0.35f, [&](float T, float N)
		{
			const int32 Period = FMath::RoundToInt(FMath::Lerp(6.f, 28.f, N));
			return Noise.Next(Period) * Attack(T) * Decay(N, 2.2f) * 0.6f;
		});
	}

	// Big blast: the same idea, longer and lower, with a sub rumble under it.
	{
		FNoise Noise(41);
		float Phase = 0.f;
		Render(EBazaSfx::ExplosionBig, 1.1f, [&](float T, float N)
		{
			const int32 Period = FMath::RoundToInt(FMath::Lerp(16.f, 90.f, N));
			Phase += FMath::Lerp(70.f, 38.f, N) / SampleRate;
			const float Rumble = Triangle(Phase);
			return (Noise.Next(Period) * 0.75f + Rumble * 0.35f) * Attack(T) * Decay(N, 1.5f) * 0.8f;
		});
	}

	// Losing a life: a long fall with a wobble in it.
	{
		float Phase = 0.f;
		FNoise Noise(53);
		Render(EBazaSfx::PlayerHit, 0.55f, [&](float T, float N)
		{
			const float Wobble = 1.f + 0.06f * FMath::Sin(T * 2.f * PI * 18.f);
			Phase += FMath::Lerp(820.f, 70.f, FMath::Pow(N, 0.7f)) * Wobble / SampleRate;
			return (Pulse(Phase, 0.5f) * 0.75f + Noise.Next(10) * 0.25f) * Attack(T) * Decay(N, 1.1f) * 0.5f;
		});
	}

	// Back in the fight: a rising arpeggio, C E G C.
	{
		float Phase = 0.f;
		const float Notes[] = { 523.25f, 659.25f, 783.99f, 1046.5f };
		Render(EBazaSfx::Respawn, 0.36f, [&](float T, float N)
		{
			const int32 Step = FMath::Min(FMath::FloorToInt(N * 4.f), 3);
			Phase += Notes[Step] / SampleRate;
			return Pulse(Phase, 0.125f) * Attack(T) * Decay(N, 0.6f) * 0.35f;
		});
	}

	// Boss on its way: a two-tone siren under the WARNING banner.
	{
		float Phase = 0.f;
		Render(EBazaSfx::BossWarning, 1.35f, [&](float T, float N)
		{
			const bool bHigh = FMath::FloorToInt(T / 0.17f) % 2 == 0;
			Phase += (bHigh ? 466.16f : 349.23f) / SampleRate;
			const float Fade = FMath::Min(1.f, (1.f - N) / 0.08f);
			return Pulse(Phase, 0.5f) * Attack(T) * Fade * 0.4f;
		});
	}

	// Boss down: a quick fanfare climbing an octave and a bit.
	{
		float Phase = 0.f;
		const float Notes[] = { 523.25f, 659.25f, 783.99f, 1046.5f, 1318.5f };
		Render(EBazaSfx::BossDefeated, 0.9f, [&](float T, float N)
		{
			const int32 Step = FMath::Min(FMath::FloorToInt(N * 6.f), 4);
			Phase += Notes[Step] / SampleRate;
			const float Hold = Step == 4 ? Decay((N - 4.f / 6.f) * 3.f, 1.f) : 1.f;
			return Pulse(Phase, 0.25f) * Attack(T) * Hold * 0.4f;
		});
	}

	// Picking something up: two quick upward sweeps, the second an octave higher.
	{
		float Phase = 0.f;
		Render(EBazaSfx::PowerUp, 0.24f, [&](float T, float N)
		{
			const float Half = N < 0.5f ? N * 2.f : (N - 0.5f) * 2.f;
			const float Base = N < 0.5f ? 520.f : 1040.f;
			Phase += FMath::Lerp(Base, Base * 1.5f, Half) / SampleRate;
			return Pulse(Phase, 0.125f) * Attack(T) * Decay(N, 0.7f) * 0.4f;
		});
	}

	// Out of lives: four notes going down, softened with a triangle underneath.
	{
		float Phase = 0.f;
		const float Notes[] = { 392.f, 329.63f, 261.63f, 196.f };
		Render(EBazaSfx::GameOver, 1.2f, [&](float T, float N)
		{
			const int32 Step = FMath::Min(FMath::FloorToInt(N * 4.f), 3);
			Phase += Notes[Step] / SampleRate;
			const float Tone = Pulse(Phase, 0.5f) * 0.55f + Triangle(Phase) * 0.45f;
			const float Hold = Step == 3 ? Decay((N - 0.75f) * 4.f, 1.2f) : 1.f;
			return Tone * Attack(T) * Hold * 0.45f;
		});
	}
}
