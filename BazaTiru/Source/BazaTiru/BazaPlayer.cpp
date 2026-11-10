#include "BazaPlayer.h"

#include "BazaAsteroid.h"
#include "BazaBossBase.h"
#include "BazaCanonBall.h"
#include "BazaDark.h"
#include "BazaGameMode.h"
#include "BazaLowEnemy.h"
#include "BazaMeshBuilder.h"
#include "BazaPowerUp.h"
#include "BazaSfx.h"
#include "BazaShipMesh.h"
#include "BazaShot.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/**
	 * A deeper cousin of the mint the original UI used. The UI colour itself is
	 * almost white once it is lit, and the ship stopped reading as green.
	 */
	const FLinearColor PlayerColor(0.11f, 0.62f, 0.42f, 1.f);

	constexpr float MaxBank = 32.f;

	/** Seconds to swing from facing one way to the other. */
	constexpr float FlipTime = 0.2f;
	constexpr float RespawnDelay = 1.1f;
	constexpr float InvulnerableTime = 2.f;
}

const FVector2D ABazaPlayer::StartPosition(49.0815f, 88.2523f);

ABazaPlayer::ABazaPlayer()
{
	// player.tscn: CircleShape2D, radius 6
	HitZone = CreateHitSphere(TEXT("HitZone"), 6.f);

	// The 2D ship was two animation frames. Here it is a generated low-poly hull:
	// faceted fuselage, swept wings, a dorsal fin and engine pods. The shape is
	// built in BeginPlay; the constructor only sets the component up.
	Hull = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Hull"));
	Hull->SetupAttachment(RootComponent);
	Hull->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Hull->SetCastShadow(false);
	Hull->bUseAsyncCooking = false;

	ConstructorHelpers::FObjectFinder<UMaterialInterface> HullMaterial(BazaMesh::Material);
	if (HullMaterial.Succeeded())
	{
		Hull->SetMaterial(0, HullMaterial.Object);
	}

	RegisterTintedMesh(Hull, PlayerColor);

	// No rotation needed: the builder's nose is +X (screen right), its canopy is
	// +Y (towards the camera) and its wingspan is Z (screen vertical), which is
	// exactly this mapping. The ship is seen from above, as a shmup wants.

	Thruster = CreateShape(TEXT("Thruster"), BazaMesh::Cone, FVector(5.f, 5.f, 8.f), FLinearColor(1.f, 0.55f, 0.1f));
	Thruster->SetRelativeLocation(FVector(BazaSettings::Px(-11.f), 0.f, 0.f));
	Thruster->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));

	ShieldRing = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ShieldRing"));
	ShieldRing->SetupAttachment(RootComponent);
	ShieldRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShieldRing->SetCastShadow(false);
	ShieldRing->bUseAsyncCooking = false;
	ShieldRing->SetVisibility(false);

	if (HullMaterial.Succeeded())
	{
		ShieldRing->SetMaterial(0, HullMaterial.Object);
	}

	RegisterTintedMesh(ShieldRing, BazaPowerUps::ColourOf(EBazaPowerUp::Shield));
}

void ABazaPlayer::BeginPlay()
{
	Super::BeginPlay();

	FBazaShipParams Ship;
	Ship.Length = BazaSettings::Px(24.f);
	Ship.Span = BazaSettings::Px(19.f);
	Ship.Depth = BazaSettings::Px(9.f);
	Ship.WingSweep = 0.62f;
	Ship.NoseLength = 1.f;

	BazaShipMesh::Build(Hull, Ship);

	// The shield: six trapezoid slabs making a hexagonal frame round the hull, in
	// the screen plane, thick towards the camera.
	{
		FBazaMeshBuilder Ring;
		constexpr float Outer = 160.f;
		constexpr float Inner = 136.f;

		for (int32 Index = 0; Index < 6; ++Index)
		{
			const float A = FMath::DegreesToRadians(60.f * Index);
			const float B = FMath::DegreesToRadians(60.f * (Index + 1));

			Ring.AddSlab({
				FVector(FMath::Cos(A) * Outer, 0.f, FMath::Sin(A) * Outer),
				FVector(FMath::Cos(B) * Outer, 0.f, FMath::Sin(B) * Outer),
				FVector(FMath::Cos(B) * Inner, 0.f, FMath::Sin(B) * Inner),
				FVector(FMath::Cos(A) * Inner, 0.f, FMath::Sin(A) * Inner)
			}, FVector(0.f, 1.f, 0.f), 8.f);
		}

		Ring.Commit(ShieldRing);
	}

	OnActorBeginOverlap.AddDynamic(this, &ABazaPlayer::HandleOverlap);

	// Development aids for captures; both are off unless named on the command line.
	bAutoFire = FParse::Param(FCommandLine::Get(), TEXT("BazaAutoFire"));

	// -BazaPowered grants every timed power-up; -BazaPowered=Spread (or Shield,
	// Speed, Magnet, comma-separated) grants just those.
	FString Wanted;
	const bool bNamed = FParse::Value(FCommandLine::Get(), TEXT("BazaPowered="), Wanted);

	if (bNamed || FParse::Param(FCommandLine::Get(), TEXT("BazaPowered")))
	{
		const TPair<EBazaPowerUp, const TCHAR*> Names[] = {
			{ EBazaPowerUp::Shield, TEXT("Shield") },
			{ EBazaPowerUp::Speed,  TEXT("Speed") },
			{ EBazaPowerUp::Magnet, TEXT("Magnet") },
			{ EBazaPowerUp::Spread, TEXT("Spread") },
		};

		for (const auto& Entry : Names)
		{
			if (!bNamed || Wanted.Contains(Entry.Value))
			{
				ApplyPowerUp(Entry.Key);
			}
		}
	}
}

void ABazaPlayer::ApplyPowerUp(EBazaPowerUp Type)
{
	const float Duration = BazaPowerUps::DurationOf(Type);

	switch (Type)
	{
	case EBazaPowerUp::Shield: ShieldRemaining = Duration; break;
	case EBazaPowerUp::Speed:  SpeedRemaining = Duration;  break;
	case EBazaPowerUp::Magnet: MagnetRemaining = Duration; break;
	case EBazaPowerUp::Spread: SpreadRemaining = Duration; break;

	case EBazaPowerUp::Repair:
		if (ABazaGameMode* Stage = GetStage())
		{
			Stage->RefillLives();
		}
		break;

	default:
		break;
	}

	FlashHit(0.12f);

	// Repair gets the rising arpeggio; everything else the pickup chime.
	UBazaSfx::Play(this, Type == EBazaPowerUp::Repair ? EBazaSfx::Respawn : EBazaSfx::PowerUp);
}

float ABazaPlayer::GetPowerUpRemaining(EBazaPowerUp Type) const
{
	switch (Type)
	{
	case EBazaPowerUp::Shield: return ShieldRemaining;
	case EBazaPowerUp::Speed:  return SpeedRemaining;
	case EBazaPowerUp::Magnet: return MagnetRemaining;
	case EBazaPowerUp::Spread: return SpreadRemaining;
	default:                   return 0.f;
	}
}

void ABazaPlayer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (InvulnerableRemaining > 0.f)
	{
		InvulnerableRemaining -= DeltaSeconds;

		// Blink while the shield is up, then settle back to solid.
		const bool bVisible = FMath::Fmod(InvulnerableRemaining, 0.16f) > 0.08f;
		SetVisualOpacity(InvulnerableRemaining > 0.f && !bVisible ? 0.25f : 1.f);
	}

	for (float* Remaining : { &ShieldRemaining, &SpeedRemaining, &MagnetRemaining, &SpreadRemaining })
	{
		*Remaining = FMath::Max(0.f, *Remaining - DeltaSeconds);
	}

	// The shield ring turns slowly, and flickers through its last two seconds as
	// warning that it is about to go.
	const bool bShieldUp = ShieldRemaining > 0.f && !bDead;
	const bool bFlickerOff = ShieldRemaining < 2.f && FMath::Fmod(ShieldRemaining, 0.2f) < 0.1f;
	ShieldRing->SetVisibility(bShieldUp && !bFlickerOff);

	ShieldSpin += DeltaSeconds * 70.f;
	ShieldRing->SetRelativeRotation(FRotator(ShieldSpin, 0.f, 0.f));

	if (bDead)
	{
		return;
	}

	APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0);
	if (!Controller)
	{
		return;
	}

	// Godot polled the keyboard straight from _process; IsInputKeyDown is the
	// direct equivalent and keeps the port free of input asset dependencies.
	FVector2D InputDir = FVector2D::ZeroVector;

	if (Controller->IsInputKeyDown(EKeys::Up))
	{
		InputDir.Y -= 1.f;
	}
	if (Controller->IsInputKeyDown(EKeys::Down))
	{
		InputDir.Y += 1.f;
	}
	if (Controller->IsInputKeyDown(EKeys::Left))
	{
		InputDir.X -= 1.f;
	}
	if (Controller->IsInputKeyDown(EKeys::Right))
	{
		InputDir.X += 1.f;
	}
	// Right is home. The ship turns to face left only while left is held, so a boss
	// coming back in from that side can be fought, and swings back as soon as it
	// is let go. Latching the last direction instead felt wrong in play: the ship
	// stayed turned round long after the reason for it had gone.
	Facing = (InputDir.X < 0.f) ? -1.f : 1.f;

	// -BazaAutoFire (capture mode only) holds the trigger down, so a screenshot
	// can show what the shots look like.
	const bool bTrigger = Controller->IsInputKeyDown(EKeys::SpaceBar) || bAutoFire;

	if (bTrigger && bCanShot)
	{
		Shoot();
	}

	const float SpeedBoost = SpeedRemaining > 0.f ? 1.5f : 1.f;

	FVector2D Position = GetGamePosition();
	Position += (DeltaSeconds * MoveSpeed * SpeedBoost) * InputDir;

	// Same clamp as the original: the ship's centre stays inside the viewport.
	Position.X = FMath::Clamp(Position.X, 0.f, BazaSettings::ScreenWidth);
	Position.Y = FMath::Clamp(Position.Y, 0.f, BazaSettings::ScreenHeight);

	SetGamePosition(Position);

	// Lean into the turn. Screen-vertical input rolls the ship about its own
	// length; the sign follows the facing so it always leans the same way on screen.
	Bank = FMath::FInterpTo(Bank, -InputDir.Y * MaxBank * Facing, DeltaSeconds, 9.f);

	// Turning round is a half-turn about the screen's vertical axis combined with
	// a half-roll, so the ship swings through facing the camera and comes out the
	// other way with its canopy still towards the viewer rather than its belly.
	FlipProgress = FMath::FInterpConstantTo(FlipProgress, Facing < 0.f ? 1.f : 0.f, DeltaSeconds, 1.f / FlipTime);

	const float Turn = PI * FlipProgress;
	const FQuat Flip = FQuat(FVector::UpVector, Turn) * FQuat(FVector::ForwardVector, Turn);
	const FQuat Lean(FVector::ForwardVector, FMath::DegreesToRadians(Bank));

	SetActorRotation(Flip * Lean);

	// The thruster burns longer and faster while the speed power-up runs.
	ThrusterPhase += DeltaSeconds * 26.f * SpeedBoost;
	const float Flare = (1.f + 0.35f * FMath::Sin(ThrusterPhase)) * (SpeedBoost > 1.f ? 1.7f : 1.f);
	Thruster->SetRelativeScale3D(FVector(
		BazaSettings::Px(5.f),
		BazaSettings::Px(5.f),
		BazaSettings::Px(8.f) * Flare) / 100.f);
}

void ABazaPlayer::Shoot()
{
	ABazaGameMode* Stage = GetStage();
	if (!Stage)
	{
		return;
	}

	const FVector2D Position = GetGamePosition();

	const bool bMagnet = MagnetRemaining > 0.f;

	auto Fire = [&](const FVector2D& From, const FVector2D& Heading)
	{
		if (ABazaShot* Shot = Stage->SpawnAt<ABazaShot>(From))
		{
			Shot->bFiredByPlayer = true;
			Shot->Direction = Heading;
			if (bMagnet)
			{
				Shot->MakeMagnetic();
			}
		}
	};

	const FVector2D Ahead(Facing, 0.f);

	// Two barrels, five pixels above and below the nose, firing whichever way the
	// ship is facing.
	for (const float Offset : { -5.f, 5.f })
	{
		Fire(FVector2D(Position.X + 16.f * Facing, Position.Y + Offset), Ahead);
	}

	// The spread power-up adds a bolt angled out either side.
	if (SpreadRemaining > 0.f)
	{
		for (const float Angle : { -14.f, 14.f })
		{
			Fire(FVector2D(Position.X + 16.f * Facing, Position.Y), Ahead.GetRotated(Angle));
		}
	}

	// One sound for the pair of bolts.
	UBazaSfx::Play(this, EBazaSfx::Shoot);

	++VolleyCount;

	bCanShot = false;

	// player.tscn: reload_timer, wait_time 0.2
	GetWorldTimerManager().SetTimer(
		ReloadTimer, this, &ABazaPlayer::OnReloadTimerTimeout, 0.2f, false);
}

void ABazaPlayer::OnReloadTimerTimeout()
{
	bCanShot = true;
}

void ABazaPlayer::HandleOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	// player.gd's list, plus the gunships and the darks, which the original let
	// the ship fly straight through.
	const bool bIsThreat =
		OtherActor->IsA<ABazaAsteroid>() ||
		OtherActor->IsA<ABazaShot>() ||
		OtherActor->IsA<ABazaCanonBall>() ||
		OtherActor->IsA<ABazaBossBase>() ||
		OtherActor->IsA<ABazaLowEnemy>() ||
		OtherActor->IsA<ABazaDark>();

	if (!bIsThreat || bDead)
	{
		return;
	}

	// The ship's own bolts never hurt it.
	if (const ABazaShot* Shot = Cast<ABazaShot>(OtherActor); Shot && Shot->bFiredByPlayer)
	{
		return;
	}

	// The shield takes the hit instead. Incoming fire is eaten; anything solid
	// flown into still breaks on its own side, as it would anyway.
	if (ShieldRemaining > 0.f)
	{
		if (OtherActor->IsA<ABazaShot>() || OtherActor->IsA<ABazaCanonBall>())
		{
			if (ABazaActor* Projectile = Cast<ABazaActor>(OtherActor))
			{
				SpawnExplosionAt(Projectile->GetGamePosition(), 8.f);
				Projectile->Remove();
			}
		}

		UBazaSfx::Play(this, EBazaSfx::Hit);
		return;
	}

	if (IsInvulnerable())
	{
		return;
	}

	// What killed the ship, for anyone reading the log after a surprising death.
	UE_LOG(LogTemp, Log, TEXT("BazaTiru: player hit by %s"), *OtherActor->GetClass()->GetName());

	SpawnExplosionAt(GetGamePosition(), 20.f);

	ABazaGameMode* Stage = GetStage();
	if (Stage && Stage->bPlayerIsInvulnerable)
	{
		// The original only ever spawned the explosion; its queue_free() and its
		// "destroyed" signal were both commented out.
		FlashHit(0.12f);
		return;
	}

	TakeHit();
}

void ABazaPlayer::TakeHit()
{
	ABazaGameMode* Stage = GetStage();
	if (!Stage)
	{
		return;
	}

	bDead = true;
	SetActorHiddenInGame(true);
	HitZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Losing a life loses whatever power-ups were running.
	ShieldRemaining = 0.f;
	SpeedRemaining = 0.f;
	MagnetRemaining = 0.f;
	SpreadRemaining = 0.f;

	Stage->AddShake(26.f);
	UBazaSfx::Play(this, EBazaSfx::PlayerHit);

	if (!Stage->ConsumeLife())
	{
		// Out of lives: this is the retry screen stage.gd built and never reached.
		Stage->OnPlayerDestroyed();
		return;
	}

	GetWorldTimerManager().SetTimer(
		RespawnTimer, this, &ABazaPlayer::Respawn, RespawnDelay, false);
}

void ABazaPlayer::Respawn()
{
	bDead = false;
	bCanShot = true;
	Bank = 0.f;

	// Come back on the side away from the boss. With the boss holding the left
	// edge, the usual start point would be inside it. The ship always comes back
	// facing right; holding left turns it towards the boss.
	FVector2D SpawnPoint = StartPosition;

	const ABazaGameMode* Stage = GetStage();
	const ABazaBossBase* Boss = Stage ? Stage->GetBoss() : nullptr;

	if (Boss && Boss->GetGamePosition().X < BazaSettings::ScreenWidth * 0.5f)
	{
		SpawnPoint.X = BazaSettings::ScreenWidth - StartPosition.X;
	}

	Facing = 1.f;
	FlipProgress = 0.f;

	SetGamePosition(SpawnPoint);
	SetActorRotation(FQuat::Identity);
	SetActorHiddenInGame(false);
	HitZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	InvulnerableRemaining = InvulnerableTime;

	UBazaSfx::Play(this, EBazaSfx::Respawn);
}
