#include "BazaBossNemy02.h"

#include "BazaGameMode.h"
#include "BazaPlayer.h"
#include "BazaSfx.h"
#include "BazaShot.h"
#include "Components/StaticMeshComponent.h"

namespace
{
	const FLinearColor EyeIdle(0.15f, 0.35f, 0.5f);
	const FLinearColor EyeWarning(1.f, 0.2f, 0.15f);

	/** Health fractions at which the echo closes in on the ship. */
	constexpr float CloserThreshold = 0.6f;
	constexpr float ClosestThreshold = 0.3f;

	/** Seconds behind the ship, phase by phase. */
	constexpr float SlowEcho = 2.f;
	constexpr float MidEcho = 1.5f;
	constexpr float FastEcho = 1.f;

	/** How long before a recorded volley comes back the eye lights up. */
	constexpr float WarningLead = 0.35f;

	/**
	 * Echo bolts are slower than the ship's own, so a volley coming back is
	 * something to dodge rather than something that has already happened.
	 */
	constexpr float EchoBoltSpeed = 280.f;

	/** The boss's column: its right-hand limit, and how far the ship's X moves it. */
	constexpr float HomeInsetPx = 24.f;
	constexpr float MirrorXScale = 0.5f;
}

ABazaBossNemy02::ABazaBossNemy02()
{
	ConfigureBoss(/*TotalHealth=*/1400.f, /*HitRadiusPx=*/34.f, /*HealthBarOffsetPx=*/40.f);

	DisplayName = NSLOCTEXT("BasaTiru", "BossNemy02", "SPELHU");

	// Tall and thin, a pane of glass: whatever the ship does, this does after it.
	EntryTargetX = BazaSettings::ScreenWidth - 34.f;
	EntrySpeed = 110.f;
	DefeatScore = 400;

	Pane = CreateShape(TEXT("Pane"), BazaMesh::Cube, FVector(10.f, 26.f, 62.f), FLinearColor(0.2f, 0.45f, 0.62f));

	FrameTop = CreateShape(TEXT("FrameTop"), BazaMesh::Cube, FVector(20.f, 30.f, 9.f), FLinearColor(0.72f, 0.78f, 0.85f));
	FrameTop->SetRelativeLocation(FVector(0.f, 0.f, BazaSettings::Px(34.f)));

	FrameBottom = CreateShape(TEXT("FrameBottom"), BazaMesh::Cube, FVector(20.f, 30.f, 9.f), FLinearColor(0.72f, 0.78f, 0.85f));
	FrameBottom->SetRelativeLocation(FVector(0.f, 0.f, BazaSettings::Px(-34.f)));

	// Sits proud of the pane on the camera side, and lights just before an echo
	// volley. Y is depth, and the camera is at positive Y.
	Eye = CreateShape(TEXT("Eye"), BazaMesh::Sphere, FVector(14.f, 14.f, 14.f), EyeIdle);
	Eye->SetRelativeLocation(FVector(BazaSettings::Px(-8.f), BazaSettings::Px(16.f), 0.f));
}

void ABazaBossNemy02::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (GetPhase() == EBazaPhase::Defeated)
	{
		return;
	}

	// Recording starts while it is still sweeping in, so the replay has
	// something to play the moment the fight begins.
	RecordPlayer();

	if (GetPhase() != EBazaPhase::Entering)
	{
		ReplayEcho();
	}

	// The eye swells just before a volley comes back, so the player has a beat
	// to get off the row.
	const bool bWarning = IsVolleyImminent();

	if (bWarning != bWasWarning)
	{
		bWasWarning = bWarning;
		SetMeshTint(Eye, bWarning ? EyeWarning : EyeIdle);
	}

	EyePulse += DeltaSeconds * (bWarning ? 14.f : 3.f);
	const float Swell = bWarning ? 1.f + 0.25f * FMath::Sin(EyePulse) : 1.f;
	Eye->SetRelativeScale3D(FVector(BazaSettings::Px(14.f * Swell)) / 100.f);
}

FVector2D ABazaBossNemy02::MirrorOf(const FVector2D& PlayerPosition) const
{
	// Same row as the ship was on. Across, it is the ship's reflection squeezed
	// into the right-hand side: the ship pushing forward draws it in, falling
	// back pushes it away, and the two never have to share a spot.
	const float HomeX = BazaSettings::ScreenWidth - HomeInsetPx;
	return FVector2D(HomeX - PlayerPosition.X * MirrorXScale, PlayerPosition.Y);
}

void ABazaBossNemy02::RecordPlayer()
{
	const ABazaGameMode* Stage = GetStage();
	const ABazaPlayer* Ship = Stage ? Stage->GetPlayer() : nullptr;
	if (!Ship || Stage->IsGameOver())
	{
		return;
	}

	FEchoSample Sample;
	Sample.Time = GetWorld()->GetTimeSeconds();
	Sample.Position = Ship->GetGamePosition();
	Sample.Facing = Ship->GetFacing();

	// Only volleys fired after it showed up count; the first sample just syncs.
	const int32 Volleys = Ship->GetVolleyCount();
	Sample.bFired = !Recording.IsEmpty() && Volleys != LastVolleyCount;
	LastVolleyCount = Volleys;

	Recording.Add(Sample);
}

void ABazaBossNemy02::ReplayEcho()
{
	const float PlayUntil = GetWorld()->GetTimeSeconds() - GetEchoDelay();

	int32 Played = 0;
	bool bFiredThisFrame = false;

	for (; Played < Recording.Num() && Recording[Played].Time <= PlayUntil; ++Played)
	{
		const FEchoSample& Sample = Recording[Played];

		EchoTarget = Sample.Position;
		bHasEchoTarget = true;

		// When the delay shortens, a second's worth of samples comes due at once;
		// that should jump the boss ahead, not dump every volley in it together.
		if (Sample.bFired && !bFiredThisFrame)
		{
			FireEcho(Sample.Facing);
			bFiredThisFrame = true;
		}
	}

	if (Played > 0)
	{
		Recording.RemoveAt(0, Played, EAllowShrinking::No);
	}
}

bool ABazaBossNemy02::IsVolleyImminent() const
{
	if (GetPhase() == EBazaPhase::Entering)
	{
		return false;
	}

	const float WarnUntil = GetWorld()->GetTimeSeconds() - GetEchoDelay() + WarningLead;

	for (const FEchoSample& Sample : Recording)
	{
		if (Sample.Time > WarnUntil)
		{
			break;
		}
		if (Sample.bFired)
		{
			return true;
		}
	}

	return false;
}

void ABazaBossNemy02::UpdateMovement(float DeltaSeconds)
{
	FVector2D Position = GetGamePosition();

	// Until the recording catches up it just holds where it arrived.
	const FVector2D Target = bHasEchoTarget
		? MirrorOf(EchoTarget)
		: FVector2D(EntryTargetX, Position.Y);

	// Chased at a capped speed rather than snapped, so a sudden jump in the
	// replay - the ship respawning, the delay shortening - reads as a dash.
	Position.X = FMath::FInterpConstantTo(Position.X, Target.X, DeltaSeconds, Speed);
	Position.Y = FMath::FInterpConstantTo(Position.Y, Target.Y, DeltaSeconds, Speed);

	Position.Y = FMath::Clamp(Position.Y, 20.f, BazaSettings::ScreenHeight - 20.f);
	SetGamePosition(Position);
}

void ABazaBossNemy02::OnDamaged()
{
	const float Fraction = GetHealthFraction();

	if (CurrentPhase == EBazaPhase::Appear && Fraction <= CloserThreshold)
	{
		CurrentPhase = EBazaPhase::Hide;
		FlashHit(0.3f);
	}
	else if (CurrentPhase == EBazaPhase::Hide && Fraction <= ClosestThreshold)
	{
		CurrentPhase = EBazaPhase::AllIn;
		FlashHit(0.3f);
	}
}

float ABazaBossNemy02::GetEchoDelay() const
{
	// Read off the phase rather than stored, so F4's phase skip changes it too.
	switch (CurrentPhase)
	{
	case EBazaPhase::Hide:
		return MidEcho;
	case EBazaPhase::AllIn:
		return FastEcho;
	default:
		return SlowEcho;
	}
}

void ABazaBossNemy02::FireEcho(float PlayerFacing)
{
	ABazaGameMode* Stage = GetStage();
	if (!Stage || Stage->IsGameOver())
	{
		return;
	}

	const FVector2D Position = GetGamePosition();

	// Mirrored: the ship firing forward means this fires back at it.
	const FVector2D Heading(-PlayerFacing, 0.f);

	// Launched from outside its own hit zone. Any bolt that touches a boss hurts
	// it, its own included.
	const FVector2D Muzzle(Position.X + Heading.X * (HitRadiusPx + 8.f), Position.Y);

	auto Fire = [&](const FVector2D& From, const FVector2D& Direction)
	{
		if (ABazaShot* Shot = Stage->SpawnAt<ABazaShot>(From))
		{
			Shot->Direction = Direction;
			Shot->MoveSpeed = EchoBoltSpeed;
		}
	};

	// The ship's two barrels, five pixels either side of the row.
	for (const float Offset : { -5.f, 5.f })
	{
		Fire(FVector2D(Muzzle.X, Muzzle.Y + Offset), Heading);
	}

	// The last phase adds the spread power-up's side bolts to every echo.
	if (CurrentPhase == EBazaPhase::AllIn)
	{
		for (const float Angle : { -14.f, 14.f })
		{
			Fire(Muzzle, Heading.GetRotated(Angle));
		}
	}

	UBazaSfx::Play(this, EBazaSfx::Cannon);
}
