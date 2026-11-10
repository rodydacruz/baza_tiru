#include "BazaBossBase.h"

#include "BazaGameMode.h"
#include "BazaPlayer.h"
#include "BazaSfx.h"
#include "BazaShot.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"

ABazaBossBase::ABazaBossBase()
{
	HitZone = CreateHitSphere(TEXT("HitZone"), HitRadiusPx);

	// boss_nemy01.tscn had a ProgressBar rotated upright beside the hull. It is off
	// by default now: pinned to the boss it hangs over the edge of the screen, and
	// the HUD draws a readable one along the bottom instead.
	HealthBar = CreateShape(TEXT("HealthBar"), BazaMesh::Cube, FVector(5.f, 5.f, 52.f), FLinearColor(0.3f, 1.f, 0.4f));
	HealthBar->SetRelativeLocation(FVector(BazaSettings::Px(46.f), 0.f, 0.f));
	HealthBar->SetVisibility(false);
}

void ABazaBossBase::ConfigureBoss(float InTotalHealth, float InHitRadiusPx, float InHealthBarOffsetPx)
{
	TotalHealth = InTotalHealth;
	Health = InTotalHealth;
	HitRadiusPx = InHitRadiusPx;

	HitZone->InitSphereRadius(BazaSettings::Px(InHitRadiusPx));
	HealthBar->SetRelativeLocation(FVector(BazaSettings::Px(InHealthBarOffsetPx), 0.f, 0.f));
}

void ABazaBossBase::BeginPlay()
{
	Super::BeginPlay();

	EnemyId = FMath::Rand();
	Health = TotalHealth;
	HealthBarScale = HealthBar->GetRelativeScale3D();

	OnActorBeginOverlap.AddDynamic(this, &ABazaBossBase::HandleOverlap);

	ArmAttackTimer();

	// The siren runs under the WARNING banner while it sweeps in.
	UBazaSfx::Play(this, EBazaSfx::BossWarning);
}

void ABazaBossBase::ArmAttackTimer()
{
	// Bound as a member function, not a lambda capturing `this`: the timer manager
	// belongs to the game instance and outlives the level, and a lambda timer is
	// not tied to the boss, so after a restart it fired into a boss that no longer
	// existed and crashed the editor. A member binding is dropped with its object.
	GetWorldTimerManager().SetTimer(
		AttackTimer, this, &ABazaBossBase::HandleAttackTimer, AttackInterval, true);
}

void ABazaBossBase::HandleAttackTimer()
{
	// Nothing shoots until the boss has finished arriving, or once it is dying.
	const ABazaGameMode* Stage = GetStage();
	if (Stage && !Stage->IsGameOver() && CurrentPhase != EBazaPhase::Entering
		&& CurrentPhase != EBazaPhase::Defeated)
	{
		OnAttackTick();
	}
}

void ABazaBossBase::SetAttackInterval(float NewInterval)
{
	AttackInterval = FMath::Max(NewInterval, 0.05f);

	// The rate is baked in when a timer is set, so it has to be laid again.
	GetWorldTimerManager().ClearTimer(AttackTimer);
	ArmAttackTimer();
}

void ABazaBossBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (CurrentPhase == EBazaPhase::Defeated)
	{
		RunDefeat(DeltaSeconds);
		return;
	}

	const ABazaGameMode* Stage = GetStage();
	if (!Stage || Stage->IsGameOver())
	{
		return;
	}

	if (CurrentPhase == EBazaPhase::Entering)
	{
		RunEntrance(DeltaSeconds);
		return;
	}

	UpdateMovement(DeltaSeconds);
}

void ABazaBossBase::RunEntrance(float DeltaSeconds)
{
	FVector2D Position = GetGamePosition();

	// A slow sweep in from off-screen, so the arrival reads as an event rather
	// than the boss simply being there one frame.
	Position.X -= EntrySpeed * DeltaSeconds;

	if (Position.X <= EntryTargetX)
	{
		Position.X = EntryTargetX;
		CurrentPhase = EBazaPhase::Appear;
		OnEntranceComplete();
	}

	SetGamePosition(Position);
}

void ABazaBossBase::RunDefeat(float DeltaSeconds)
{
	DefeatElapsed += DeltaSeconds;

	// Come apart in bursts rather than blinking out of existence.
	if (DefeatElapsed >= NextDeathBlast)
	{
		NextDeathBlast = DefeatElapsed + FMath::FRandRange(0.07f, 0.16f);

		const FVector2D Offset(
			FMath::FRandRange(-HitRadiusPx, HitRadiusPx),
			FMath::FRandRange(-HitRadiusPx, HitRadiusPx));

		SpawnExplosionAt(GetGamePosition() + Offset, FMath::FRandRange(12.f, 26.f));
		FlashHit(0.05f);
	}

	// Drifting back and settling as it dies.
	FVector2D Position = GetGamePosition();
	Position.X += 26.f * DeltaSeconds;
	SetGamePosition(Position);

	if (DefeatElapsed < DefeatDuration)
	{
		return;
	}

	SpawnExplosionAt(GetGamePosition(), HitRadiusPx * 1.6f);

	if (ABazaGameMode* Stage = GetStage())
	{
		Stage->AddShake(45.f);
	}

	UBazaSfx::Play(this, EBazaSfx::BossDefeated);

	Destroy();
}

void ABazaBossBase::HandleOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	if (CurrentPhase == EBazaPhase::Defeated || CurrentPhase == EBazaPhase::Entering)
	{
		return;
	}

	if (const ABazaShot* Shot = Cast<ABazaShot>(OtherActor))
	{
		if (!AcceptsShot(Shot))
		{
			return;
		}

		OnScore.Broadcast(HitScore, EnemyId);

		Health -= Damage;
		StartHitAnimation();
		FlashHit();
		UBazaSfx::Play(this, EBazaSfx::Hit);
		SpawnExplosionAt(Shot->GetGamePosition(), 10.f);

		OnDamaged();
		DrawHealth();

		if (Health <= 0.f)
		{
			BeginDefeat();
		}
	}
	else if (const ABazaPlayer* Player = Cast<ABazaPlayer>(OtherActor))
	{
		SpawnExplosionAt(Player->GetGamePosition(), 16.f);
	}
}

void ABazaBossBase::DebugSkipPhase()
{
	switch (CurrentPhase)
	{
	case EBazaPhase::Entering:
		CurrentPhase = EBazaPhase::Appear;
		OnEntranceComplete();
		break;

	case EBazaPhase::Appear:
		CurrentPhase = EBazaPhase::Hide;
		break;

	case EBazaPhase::Hide:
		CurrentPhase = EBazaPhase::AllIn;
		break;

	case EBazaPhase::AllIn:
		BeginDefeat();
		break;

	default:
		break;
	}
}

void ABazaBossBase::StartHitAnimation()
{
	FVector2D Position = GetGamePosition();
	Position.X += 2.f;
	SetGamePosition(Position);
}

void ABazaBossBase::DrawHealth()
{
	const float Fraction = FMath::Clamp(GetHealthFraction(), 0.f, 1.f);

	FVector NewScale = HealthBarScale;
	NewScale.Z *= Fraction;
	HealthBar->SetRelativeScale3D(NewScale);
}

void ABazaBossBase::BeginDefeat()
{
	if (CurrentPhase == EBazaPhase::Defeated)
	{
		return;
	}

	CurrentPhase = EBazaPhase::Defeated;
	Health = 0.f;
	DefeatElapsed = 0.f;
	NextDeathBlast = 0.f;

	GetWorldTimerManager().ClearTimer(AttackTimer);
	HitZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HealthBar->SetVisibility(false);

	// The original just called queue_free() and moved on; putting a boss down
	// should be worth something.
	OnScore.Broadcast(DefeatScore, EnemyId);

	if (ABazaGameMode* Stage = GetStage())
	{
		Stage->OnBossDefeated();
	}
}
