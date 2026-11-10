#include "BazaBossNemy03.h"

#include "BazaCanonBall.h"
#include "BazaGameMode.h"
#include "BazaLowEnemy.h"
#include "BazaSfx.h"
#include "BazaShot.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"

namespace
{
	/** Where things sit on the hull's face, as offsets from its centre row. */
	constexpr float TurretOffsets[] = { -84.f, -34.f, 34.f, 84.f };
	constexpr float HangarOffsets[] = { -58.f, 58.f };

	/** Taller than the screen, so there is no getting round it. */
	constexpr float HullHeight = 256.f;

	/** Rows either side of the middle line the ram stops short of: the free half. */
	constexpr float LaneEdgeTop = 84.f;
	constexpr float LaneEdgeBottom = 96.f;

	constexpr float CoreRadius = 14.f;
	constexpr float TurretHealthMax = 80.f;

	/** Health fractions at which it escalates. */
	constexpr float HangarThreshold = 0.66f;
	constexpr float FrenzyThreshold = 0.33f;

	const FLinearColor HullColour(0.14f, 0.15f, 0.2f);
	const FLinearColor ArmourColour(0.3f, 0.32f, 0.4f);
	const FLinearColor TrimColour(0.45f, 0.47f, 0.55f);
	const FLinearColor TurretColour(0.5f, 0.5f, 0.56f);
	const FLinearColor WreckColour(0.06f, 0.06f, 0.07f);

	/** A point on the hull, in game pixels: across, down the screen, and towards the camera. */
	FVector HullPoint(float X, float Y, float Depth = 0.f)
	{
		return FVector(BazaSettings::Px(X), BazaSettings::Px(Depth), BazaSettings::Px(-Y));
	}
}

ABazaBossNemy03::ABazaBossNemy03()
{
	// Only the core counts against its health; the hit zone the base gives every
	// boss is moved onto it.
	ConfigureBoss(/*TotalHealth=*/2600.f, /*HitRadiusPx=*/CoreRadius, /*HealthBarOffsetPx=*/0.f);
	HitZone->SetRelativeLocation(HullPoint(-HullHalfWidth - 4.f, 0.f));

	DisplayName = NSLOCTEXT("BasaTiru", "BossNemy03", "BARKU MAI");

	// It creeps on rather than sweeping in, and parks with its face a little over
	// three quarters of the way across.
	EntryTargetX = BazaSettings::ScreenWidth - 30.f;
	EntrySpeed = 22.f;
	AttackInterval = 0.15f;
	DefeatScore = 1000;
	DefeatDuration = 3.f;

	// The whole hull is solid: flying into it costs a life, and it stops bolts.
	HullZone = CreateHitBox(TEXT("HullZone"), FVector2D(HullHalfWidth * 2.f - 6.f, HullHeight));

	Hull = CreateShape(TEXT("Hull"), BazaMesh::Cube, FVector(HullHalfWidth * 2.f, 70.f, HullHeight), HullColour);

	Armour = CreateShape(TEXT("Armour"), BazaMesh::Cube, FVector(8.f, 60.f, HullHeight - 6.f), ArmourColour);
	Armour->SetRelativeLocation(HullPoint(-HullHalfWidth - 1.f, 0.f));

	// A painted band down the flank, and rows of lit windows, so the hull reads
	// as a ship and not a wall.
	UStaticMeshComponent* Band = CreateShape(TEXT("Band"), BazaMesh::Cube, FVector(6.f, 1.f, HullHeight), FLinearColor(0.55f, 0.1f, 0.1f));
	Band->SetRelativeLocation(HullPoint(28.f, 0.f, 36.f));

	int32 WindowIndex = 0;
	for (const float WindowX : { -18.f, 8.f })
	{
		for (float WindowY = -120.f; WindowY <= 120.f; WindowY += 20.f)
		{
			UStaticMeshComponent* Window = CreateShape(
				FName(*FString::Printf(TEXT("Window%d"), WindowIndex++)),
				BazaMesh::Cube, FVector(4.f, 1.f, 3.f), FLinearColor(1.f, 0.8f, 0.4f));
			Window->SetRelativeLocation(HullPoint(WindowX, WindowY, 36.f));
		}
	}

	int32 RibIndex = 0;
	for (const float RibY : { -110.f, -71.f, -45.f, 45.f, 71.f, 110.f })
	{
		UStaticMeshComponent* Rib = CreateShape(
			FName(*FString::Printf(TEXT("Rib%d"), RibIndex++)),
			BazaMesh::Cube, FVector(18.f, 50.f, 4.f), TrimColour);
		Rib->SetRelativeLocation(HullPoint(-HullHalfWidth - 5.f, RibY));
		Ribs.Add(Rib);
	}

	// The core: the one soft spot, set in a frame, behind a pair of shutters.
	UStaticMeshComponent* CoreFrame = CreateShape(TEXT("CoreFrame"), BazaMesh::Cylinder, FVector(40.f, 40.f, 6.f), TrimColour);
	CoreFrame->SetRelativeLocation(HullPoint(-HullHalfWidth - 2.f, 0.f, 2.f));
	// Cylinders stand up Z; rolled over, the disc faces the camera.
	CoreFrame->SetRelativeRotation(FRotator(0.f, 0.f, 90.f));

	Core = CreateShape(TEXT("Core"), BazaMesh::Sphere, FVector(CoreRadius * 2.f), FLinearColor(1.f, 0.35f, 0.1f));
	Core->SetRelativeLocation(HullPoint(-HullHalfWidth - 3.f, 0.f, 6.f));

	ShutterTop = CreateShape(TEXT("ShutterTop"), BazaMesh::Cube, FVector(6.f, 34.f, 12.f), ArmourColour);
	ShutterBottom = CreateShape(TEXT("ShutterBottom"), BazaMesh::Cube, FVector(6.f, 34.f, 12.f), ArmourColour);

	for (int32 Index = 0; Index < TurretCount; ++Index)
	{
		const float Offset = TurretOffsets[Index];

		UStaticMeshComponent* Base = CreateShape(
			FName(*FString::Printf(TEXT("TurretBase%d"), Index)),
			BazaMesh::Cube, FVector(12.f, 26.f, 10.f), TurretColour);
		Base->SetRelativeLocation(HullPoint(-HullHalfWidth - 6.f, Offset));
		TurretBases.Add(Base);

		UStaticMeshComponent* Barrel = CreateShape(
			FName(*FString::Printf(TEXT("TurretBarrel%d"), Index)),
			BazaMesh::Cylinder, FVector(4.f, 4.f, 14.f), FLinearColor(0.2f, 0.2f, 0.24f));
		Barrel->SetRelativeLocation(HullPoint(-HullHalfWidth - 16.f, Offset));
		// Laid along the screen's X, pointing out of the face.
		Barrel->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
		TurretBarrels.Add(Barrel);

		TurretHealth[Index] = TurretHealthMax;
	}

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(HangarOffsets); ++Index)
	{
		UStaticMeshComponent* Bay = CreateShape(
			FName(*FString::Printf(TEXT("Hangar%d"), Index)),
			BazaMesh::Cube, FVector(6.f, 40.f, 16.f), FLinearColor(0.03f, 0.03f, 0.05f));
		Bay->SetRelativeLocation(HullPoint(-HullHalfWidth - 2.f, HangarOffsets[Index]));

		UStaticMeshComponent* Light = CreateShape(
			FName(*FString::Printf(TEXT("HangarLight%d"), Index)),
			BazaMesh::Sphere, FVector(5.f), FLinearColor(1.f, 0.6f, 0.1f));
		Light->SetRelativeLocation(HullPoint(-HullHalfWidth - 6.f, HangarOffsets[Index] - 11.f, 12.f));
		Light->SetVisibility(false);
		HangarLights.Add(Light);
	}

	// The danger stripes stay put in the world while the ship moves, so they are
	// placed absolutely rather than riding on the hull.
	for (int32 Index = 0; Index < 4; ++Index)
	{
		UStaticMeshComponent* Stripe = CreateShape(
			FName(*FString::Printf(TEXT("LaneStripe%d"), Index)),
			BazaMesh::Cube, FVector(BazaSettings::ScreenWidth, 1.f, 2.f), FLinearColor(1.f, 0.12f, 0.1f));
		Stripe->SetUsingAbsoluteLocation(true);
		Stripe->SetVisibility(false);
		LaneStripes.Add(Stripe);
	}

	SetCoreOpen(true);
	CoreOpenness = 1.f;
}

void ABazaBossNemy03::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsActorBeingDestroyed())
	{
		return;
	}

	ABazaGameMode* Stage = GetStage();
	if (!Stage)
	{
		return;
	}

	if (GetPhase() == EBazaPhase::Defeated)
	{
		// The base only switches the core off; the hull has to go quiet as well,
		// or the wreck would still kill the ship.
		if (HullZone->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
		{
			HullZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			ShowLane(false);
		}

		// Something this size comes apart all along its length, not just round
		// the core.
		DeathBlastCooldown -= DeltaSeconds;
		if (DeathBlastCooldown <= 0.f)
		{
			DeathBlastCooldown = 0.09f;

			const FVector2D Where(
				GetGamePosition().X + FMath::FRandRange(-HullHalfWidth, HullHalfWidth),
				FMath::Clamp(GetGamePosition().Y + FMath::FRandRange(-100.f, 100.f), 0.f, BazaSettings::ScreenHeight));

			SpawnExplosionAt(Where, FMath::FRandRange(14.f, 34.f));
			Stage->AddShake(6.f);
		}
		return;
	}

	if (Stage->IsGameOver())
	{
		return;
	}

	Blink += DeltaSeconds;

	if (GetPhase() == EBazaPhase::Entering)
	{
		// The whole screen rumbles as it creeps on, and it settles onto the
		// middle row whatever height it came in at.
		RumbleCooldown -= DeltaSeconds;
		if (RumbleCooldown <= 0.f)
		{
			RumbleCooldown = 0.25f;
			Stage->AddShake(3.f);
		}

		FVector2D Position = GetGamePosition();
		Position.Y = FMath::FInterpConstantTo(Position.Y, BazaSettings::ScreenHeight * 0.5f, DeltaSeconds, 60.f);
		SetGamePosition(Position);
	}

	// The shutters slide rather than snap, so the player can see the core
	// closing and knows not to waste the shots.
	CoreOpenness = FMath::FInterpConstantTo(CoreOpenness, bCoreOpen ? 1.f : 0.f, DeltaSeconds, 4.f);
	const float ShutterY = FMath::Lerp(6.f, 21.f, CoreOpenness);
	ShutterTop->SetRelativeLocation(HullPoint(-HullHalfWidth - 16.f, -ShutterY, 10.f));
	ShutterBottom->SetRelativeLocation(HullPoint(-HullHalfWidth - 16.f, ShutterY, 10.f));

	const float Pulse = 1.f + 0.08f * FMath::Sin(Blink * (GetPhase() == EBazaPhase::AllIn ? 12.f : 6.f));
	Core->SetRelativeScale3D(FVector(BazaSettings::Px(CoreRadius * 2.f * Pulse)) / 100.f);

	// The stripes flash while it lines up, then go as it commits.
	if (Charge == EChargeStep::Aim)
	{
		ShowLane(FMath::Fmod(Blink, 0.16f) < 0.09f);
	}

	// The hangar lights flash in the second before gunships come out.
	const bool bLaunchSoon = GetPhase() != EBazaPhase::Appear && Charge == EChargeStep::None && FighterCooldown < 0.9f;
	for (UStaticMeshComponent* Light : HangarLights)
	{
		Light->SetVisibility(bLaunchSoon && FMath::Fmod(Blink, 0.2f) < 0.1f);
	}
}

void ABazaBossNemy03::UpdateMovement(float DeltaSeconds)
{
	FVector2D Position = GetGamePosition();

	if (Charge != EChargeStep::None)
	{
		UpdateCharge(DeltaSeconds, Position);
		SetGamePosition(Position);
		return;
	}

	// A slow heave up and down, so even the core will not hold still to be shot.
	const bool bFrenzy = CurrentPhase == EBazaPhase::AllIn;
	BobTime += DeltaSeconds;

	const float BobY = BazaSettings::ScreenHeight * 0.5f
		+ FMath::Sin(BobTime * (bFrenzy ? 0.9f : 0.6f)) * (bFrenzy ? 30.f : 22.f);

	Position.X = FMath::FInterpConstantTo(Position.X, EntryTargetX, DeltaSeconds, 60.f);
	Position.Y = FMath::FInterpConstantTo(Position.Y, BobY, DeltaSeconds, 60.f);
	SetGamePosition(Position);

	ChargeCooldown -= DeltaSeconds;
	if (ChargeCooldown <= 0.f)
	{
		// Mostly goes for the half the ship is in, so sitting still is never safe.
		const ABazaGameMode* Stage = GetStage();
		const bool bPlayerTop = Stage && Stage->GetPlayerPosition().Y < BazaSettings::ScreenHeight * 0.5f;
		BeginAim(FMath::FRand() < 0.7f ? bPlayerTop : !bPlayerTop);
		return;
	}

	if (CurrentPhase != EBazaPhase::Appear)
	{
		FighterCooldown -= DeltaSeconds;
		if (FighterCooldown <= 0.f)
		{
			FighterCooldown = bFrenzy ? 6.f : 7.5f;
			LaunchFighters();
		}
	}
}

void ABazaBossNemy03::BeginAim(bool bTop)
{
	Charge = EChargeStep::Aim;
	StepTime = 0.f;
	bChargeTop = bTop;

	SetCoreOpen(false);

	// Stripes across the half that is about to be rammed, from the left edge up to
	// the ship's face.
	const float LaneTop = bTop ? 0.f : LaneEdgeBottom;
	const float LaneBottom = bTop ? LaneEdgeTop : BazaSettings::ScreenHeight;

	for (int32 Index = 0; Index < LaneStripes.Num(); ++Index)
	{
		const float Row = FMath::Lerp(LaneTop, LaneBottom, (Index + 0.5f) / LaneStripes.Num());

		// Pushed back a little, so shots and ships pass in front of them.
		LaneStripes[Index]->SetWorldLocation(
			BazaSettings::ToWorld(FVector2D(BazaSettings::ScreenWidth * 0.5f, Row)) + FVector(0.f, BazaSettings::Px(-12.f), 0.f));
	}

	UBazaSfx::Play(this, EBazaSfx::BossWarning, 0.6f);
}

void ABazaBossNemy03::UpdateCharge(float DeltaSeconds, FVector2D& Position)
{
	const bool bFrenzy = CurrentPhase == EBazaPhase::AllIn;
	StepTime += DeltaSeconds;

	// Lined up so one edge of the hull sits just short of the middle line: the
	// other half of the screen is always left open.
	const float HalfHeight = HullHeight * 0.5f;
	const float CoverY = bChargeTop ? LaneEdgeTop - HalfHeight : LaneEdgeBottom + HalfHeight;

	switch (Charge)
	{
	case EChargeStep::Aim:
	{
		// Draws back a touch and shudders: the wind-up.
		Position.Y = FMath::FInterpConstantTo(Position.Y, CoverY, DeltaSeconds, 260.f);
		Position.X = FMath::FInterpConstantTo(Position.X, EntryTargetX + 14.f, DeltaSeconds, 40.f);

		if (ABazaGameMode* Stage = GetStage())
		{
			Stage->AddShake(0.6f);
		}

		const float AimTime = bFrenzy ? 0.9f : 1.2f;
		if (StepTime >= AimTime && FMath::IsNearlyEqual(Position.Y, CoverY, 0.5f))
		{
			Charge = EChargeStep::Ram;
			StepTime = 0.f;
			ShowLane(false);

			if (ABazaGameMode* Stage = GetStage())
			{
				Stage->AddShake(20.f);
			}
			UBazaSfx::Play(this, EBazaSfx::Cannon);
		}
		break;
	}

	case EChargeStep::Ram:
	{
		// All the way to the far edge.
		Position.X -= (bFrenzy ? 480.f : 420.f) * DeltaSeconds;

		const float StopX = HullHalfWidth - 10.f;
		if (Position.X <= StopX)
		{
			Position.X = StopX;
			Charge = EChargeStep::Hold;
			StepTime = 0.f;

			if (ABazaGameMode* Stage = GetStage())
			{
				Stage->AddShake(12.f);
			}
		}
		break;
	}

	case EChargeStep::Hold:
		if (StepTime >= 0.35f)
		{
			Charge = EChargeStep::Recover;
			StepTime = 0.f;
		}
		break;

	case EChargeStep::Recover:
	{
		// Straight back first, and only then back to the middle: moving up or down
		// while it is still out across the screen would close the open half.
		Position.X = FMath::FInterpConstantTo(Position.X, EntryTargetX, DeltaSeconds, 170.f);

		if (Position.X < EntryTargetX - 0.5f)
		{
			break;
		}

		// The last phase sometimes goes straight back in down the other half.
		if (bFrenzy && !bChainCharge && FMath::FRand() < 0.45f)
		{
			bChainCharge = true;
			BeginAim(!bChargeTop);
			break;
		}

		const float MiddleY = BazaSettings::ScreenHeight * 0.5f;
		Position.Y = FMath::FInterpConstantTo(Position.Y, MiddleY, DeltaSeconds, 120.f);

		if (FMath::IsNearlyEqual(Position.Y, MiddleY, 0.5f))
		{
			Charge = EChargeStep::None;
			bChainCharge = false;
			BobTime = 0.f;
			SetCoreOpen(true);

			ChargeCooldown =
				CurrentPhase == EBazaPhase::AllIn ? 3.8f :
				CurrentPhase == EBazaPhase::Hide ? 5.f : 6.5f;
		}
		break;
	}

	default:
		break;
	}
}

void ABazaBossNemy03::ShowLane(bool bVisible)
{
	for (UStaticMeshComponent* Stripe : LaneStripes)
	{
		Stripe->SetVisibility(bVisible);
	}
}

void ABazaBossNemy03::SetCoreOpen(bool bOpen)
{
	bCoreOpen = bOpen;
}

void ABazaBossNemy03::OnAttackTick()
{
	// It holds fire through a ram, so the ram is the thing to watch.
	if (Charge != EChargeStep::None)
	{
		return;
	}

	++AttackTicks;

	switch (CurrentPhase)
	{
	case EBazaPhase::Appear:
		if (AttackTicks % 3 == 0) { FireFromTurret(); }
		if (AttackTicks % 22 == 0) { FireCurtain(); }
		break;

	case EBazaPhase::Hide:
		if (AttackTicks % 2 == 0) { FireFromTurret(); }
		if (AttackTicks % 18 == 0) { FireCurtain(); }
		break;

	case EBazaPhase::AllIn:
		if (AttackTicks % 2 == 0) { FireSpiral(); }
		if (AttackTicks % 4 == 0) { FireFromTurret(); }
		if (AttackTicks % 26 == 0) { FireCurtain(); }
		break;

	default:
		break;
	}
}

void ABazaBossNemy03::OnDamaged()
{
	const float Fraction = GetHealthFraction();

	ABazaGameMode* Stage = GetStage();

	if (CurrentPhase == EBazaPhase::Appear && Fraction <= HangarThreshold)
	{
		// The hangars open.
		CurrentPhase = EBazaPhase::Hide;
		FighterCooldown = 1.f;
		FlashHit(0.4f);
		if (Stage)
		{
			Stage->AddShake(25.f);
		}
	}
	else if (CurrentPhase == EBazaPhase::Hide && Fraction <= FrenzyThreshold)
	{
		// And it stops holding anything back.
		CurrentPhase = EBazaPhase::AllIn;
		ChargeCooldown = FMath::Min(ChargeCooldown, 1.5f);
		FlashHit(0.4f);
		if (Stage)
		{
			Stage->AddShake(25.f);
		}
	}
}

bool ABazaBossNemy03::AcceptsShot(const ABazaShot* Shot)
{
	// Its own gunships' fire is no threat to it.
	if (!Shot->bFiredByPlayer)
	{
		return false;
	}

	// Which part of the face a bolt meets is all down to its row: everything on
	// the face is in front of the armour.
	const float Row = Shot->GetGamePosition().Y - GetGamePosition().Y;

	if (bCoreOpen && CoreOpenness > 0.5f && FMath::Abs(Row) < CoreRadius - 1.f)
	{
		return true;
	}

	for (int32 Index = 0; Index < TurretCount; ++Index)
	{
		if (TurretHealth[Index] <= 0.f || FMath::Abs(Row - TurretOffsets[Index]) >= 7.f)
		{
			continue;
		}

		TurretHealth[Index] -= Damage;
		SpawnExplosionAt(Shot->GetGamePosition(), 6.f);
		UBazaSfx::Play(this, EBazaSfx::Hit, 0.6f);

		if (TurretHealth[Index] <= 0.f)
		{
			// Shot off: blackened, barrel gone, and it never fires again.
			SetMeshTint(TurretBases[Index], WreckColour);
			TurretBarrels[Index]->SetVisibility(false);
			SpawnExplosionAt(FVector2D(FaceX() - 6.f, RowOf(TurretOffsets[Index])), 24.f);
			OnScore.Broadcast(50, EnemyId);

			if (ABazaGameMode* Stage = GetStage())
			{
				Stage->AddShake(10.f);
			}
		}

		return false;
	}

	// Armour: a spark, and nothing more.
	SpawnExplosionAt(Shot->GetGamePosition(), 4.f);
	return false;
}

void ABazaBossNemy03::FireBall(const FVector2D& From, const FVector2D& Direction, float Speed)
{
	ABazaGameMode* Stage = GetStage();
	if (!Stage)
	{
		return;
	}

	// Always a set speed. Nemy01's aim uses the raw distance as the velocity, which
	// leaves a ball fired at close range hanging in the air.
	if (ABazaCanonBall* Ball = Stage->SpawnAt<ABazaCanonBall>(From))
	{
		Ball->Velocity = Direction.GetSafeNormal() * Speed;
	}
}

void ABazaBossNemy03::FireFromTurret()
{
	const ABazaGameMode* Stage = GetStage();
	if (!Stage)
	{
		return;
	}

	// The next turret round that is still standing and actually on screen.
	for (int32 Tries = 0; Tries < TurretCount; ++Tries)
	{
		const int32 Index = NextTurret;
		NextTurret = (NextTurret + 1) % TurretCount;

		const float Row = RowOf(TurretOffsets[Index]);
		if (TurretHealth[Index] <= 0.f || Row < 4.f || Row > BazaSettings::ScreenHeight - 4.f)
		{
			continue;
		}

		const FVector2D Muzzle(FaceX() - 22.f, Row);
		const float Speed =
			CurrentPhase == EBazaPhase::AllIn ? 135.f :
			CurrentPhase == EBazaPhase::Hide ? 125.f : 115.f;

		FireBall(Muzzle, Stage->GetPlayerPosition() - Muzzle, Speed);
		UBazaSfx::Play(this, EBazaSfx::Cannon, 0.7f);
		return;
	}
}

void ABazaBossNemy03::FireCurtain()
{
	// A wall of balls down the whole height with one gap in it, somewhere the
	// ship will have to get to.
	const float GapY = FMath::FRandRange(30.f, BazaSettings::ScreenHeight - 30.f);

	for (float Row = 8.f; Row <= BazaSettings::ScreenHeight - 8.f; Row += 14.f)
	{
		if (FMath::Abs(Row - GapY) < 24.f)
		{
			continue;
		}

		FireBall(FVector2D(FaceX() - 12.f, Row), FVector2D(-1.f, 0.f), 80.f);
	}

	UBazaSfx::Play(this, EBazaSfx::Cannon);
}

void ABazaBossNemy03::FireSpiral()
{
	if (!bCoreOpen)
	{
		return;
	}

	// Three streams out of the core, sweeping up and down the screen together.
	SpiralAngle += 23.f;
	const float Sweep = 180.f + 55.f * FMath::Sin(FMath::DegreesToRadians(SpiralAngle));

	for (int32 Arm = -1; Arm <= 1; ++Arm)
	{
		const float Angle = FMath::DegreesToRadians(Sweep + Arm * 28.f);
		FireBall(FVector2D(FaceX() - 18.f, GetGamePosition().Y),
			FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)), 105.f);
	}
}

void ABazaBossNemy03::LaunchFighters()
{
	ABazaGameMode* Stage = GetStage();
	UWorld* World = GetWorld();
	if (!Stage || !World)
	{
		return;
	}

	// Never more than a handful out at once.
	int32 Flying = 0;
	for (TActorIterator<ABazaLowEnemy> It(World); It; ++It)
	{
		if (!It->IsRemoved())
		{
			++Flying;
		}
	}

	for (const float Offset : HangarOffsets)
	{
		const float Row = RowOf(Offset);
		if (Flying >= 4 || Row < 16.f || Row > BazaSettings::ScreenHeight - 16.f)
		{
			continue;
		}

		Stage->SpawnAt<ABazaLowEnemy>(FVector2D(FaceX() - 14.f, Row));
		++Flying;
	}

	Stage->AddShake(4.f);
}
