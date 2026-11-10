#include "BazaBossNemy01.h"

#include "BazaCanonBall.h"
#include "BazaGameMode.h"
#include "BazaSfx.h"
#include "BazaShipMesh.h"
#include "BazaWarning.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ABazaBossNemy01::ABazaBossNemy01()
{
	// boss_nemy01.tscn: CircleShape2D, radius 36
	ConfigureBoss(/*TotalHealth=*/1000.f, /*HitRadiusPx=*/36.f, /*HealthBarOffsetPx=*/46.f);

	DisplayName = NSLOCTEXT("BasaTiru", "BossNemy01", "BOSS NEMY 01");

	// stage.gd dropped it at ScreenWidth - 20, which with a hull this wide leaves a
	// third of the boss hanging off the right edge. Brought in far enough to sit
	// inside the frame.
	EntryTargetX = BazaSettings::ScreenWidth - 48.f;

	// A generated hull, the same builder the player and the gunships use, turned
	// around to face left and given heavy, slabby proportions.
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

	RegisterTintedMesh(Hull, FLinearColor(0.42f, 0.06f, 0.3f));

	Prow = CreateShape(TEXT("Prow"), BazaMesh::Cone, FVector(30.f, 30.f, 26.f), FLinearColor(0.95f, 0.75f, 0.2f));
	Prow->SetRelativeLocation(FVector(BazaSettings::Px(-40.f), 0.f, 0.f));
	// The boss faces left, so its ram points down the screen's -X.
	Prow->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));

	// A slowly turning eye, sat proud of the hull on the camera side so it can
	// actually be seen. Y is depth here, and the camera is at positive Y.
	Ring = CreateShape(TEXT("Ring"), BazaMesh::Cylinder, FVector(22.f, 22.f, 7.f), FLinearColor(1.f, 0.35f, 0.15f));
	Ring->SetRelativeLocation(FVector(BazaSettings::Px(-4.f), BazaSettings::Px(20.f), 0.f));
	Ring->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
}

void ABazaBossNemy01::BeginPlay()
{
	Super::BeginPlay();

	FBazaShipParams Ship;
	Ship.Length = BazaSettings::Px(86.f);
	Ship.Span = BazaSettings::Px(74.f);
	Ship.Depth = BazaSettings::Px(34.f);
	Ship.WingSweep = 0.18f;
	Ship.NoseLength = 0.45f;
	Ship.WingThickness = 0.3f;
	Ship.bDorsalFin = false;
	Ship.bFacesBackwards = true;

	BazaShipMesh::Build(Hull, Ship);
}

void ABazaBossNemy01::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	RingSpin += DeltaSeconds * 120.f;
	Ring->SetRelativeRotation(FRotator(90.f, 0.f, RingSpin));
}

void ABazaBossNemy01::UpdateMovement(float DeltaSeconds)
{
	ABazaGameMode* Stage = GetStage();
	if (!Stage)
	{
		return;
	}

	FVector2D Position = GetGamePosition();

	switch (CurrentPhase)
	{
	case EBazaPhase::Appear:
	{
		if (Position.X > BazaSettings::BossAttackPosition)
		{
			Position.X -= DeltaSeconds * Speed;
		}

		// Creep towards the player's row, one fixed step per frame in either
		// direction, so it never settles and always jitters around the target.
		const FVector2D PlayerPosition = Stage->GetPlayerPosition();
		const float Diff = Position.Y - PlayerPosition.Y;
		Position.Y += (Diff < 0.f ? 1.f : -1.f) * (DeltaSeconds * Speed);
		break;
	}

	case EBazaPhase::Hide:
	{
		if (Position.X > BazaSettings::BossLeftHidePosition)
		{
			Position.X -= DeltaSeconds * Speed * 5.f;
		}

		if (StrikeCount >= 10)
		{
			CurrentPhase = EBazaPhase::AllIn;
		}
		break;
	}

	case EBazaPhase::AllIn:
	{
		if (!bAllInArrived)
		{
			// The charge back in from the left, as the original had it.
			Position.X += DeltaSeconds * Speed * 5.f;
			SetGameRotation(180.f);

			if (Position.X >= BazaSettings::BossLeftAttackPosition)
			{
				Position.X = BazaSettings::BossLeftAttackPosition;
				bAllInArrived = true;
				SetAttackInterval(0.45f);
			}
			break;
		}

		// boss_nemy_01.gd stopped here: the boss parked at x=60, turned round and
		// did nothing for the rest of the fight. Now it holds the left side facing
		// the player and fights: it works the rows and fires, and every few seconds
		// it flashes, draws back and rams.
		AllInTime += DeltaSeconds;
		AllInMoveTime += DeltaSeconds;

		const float Anchor = BazaSettings::BossLeftAttackPosition;
		const FVector2D PlayerPosition = Stage->GetPlayerPosition();

		switch (AllInMove)
		{
		case EAllInMove::Hold:
		{
			const float Diff = Position.Y - PlayerPosition.Y;
			if (FMath::Abs(Diff) > 2.f)
			{
				Position.Y += (Diff < 0.f ? 1.f : -1.f) * DeltaSeconds * Speed * 1.4f;
			}

			const float WeaveX = Anchor + FMath::Sin(AllInTime * 1.8f) * 12.f;
			Position.X = FMath::FInterpTo(Position.X, WeaveX, DeltaSeconds, 4.f);

			if (AllInMoveTime >= HoldTime)
			{
				// The tell: it goes white and pulls back before it comes.
				AllInMove = EAllInMove::WindUp;
				AllInMoveTime = 0.f;
				FlashHit(WindUpTime);
			}
			break;
		}

		case EAllInMove::WindUp:
		{
			Position.X = FMath::FInterpTo(Position.X, Anchor - 14.f, DeltaSeconds, 6.f);

			if (AllInMoveTime >= WindUpTime)
			{
				// Committed to where the ship was at the moment of the tell, so
				// moving off that line is how the player dodges it.
				DashDirection = (PlayerPosition - Position).GetSafeNormal();
				if (DashDirection.IsNearlyZero())
				{
					DashDirection = FVector2D(1.f, 0.f);
				}

				AllInMove = EAllInMove::Dash;
				AllInMoveTime = 0.f;

				Stage->AddShake(8.f);
				UBazaSfx::Play(this, EBazaSfx::Cannon, 1.2f);
			}
			break;
		}

		case EAllInMove::Dash:
		{
			Position += DashDirection * DashSpeed * DeltaSeconds;

			const bool bSpent = AllInMoveTime >= DashTime
				|| Position.X > BazaSettings::ScreenWidth - 30.f
				|| Position.Y < 20.f
				|| Position.Y > BazaSettings::ScreenHeight - 20.f;

			if (bSpent)
			{
				AllInMove = EAllInMove::Return;
				AllInMoveTime = 0.f;
			}
			break;
		}

		case EAllInMove::Return:
		{
			const FVector2D Home(Anchor, Position.Y);
			Position = FMath::Vector2DInterpConstantTo(Position, Home, DeltaSeconds, Speed * 3.f);

			if (FMath::Abs(Position.X - Anchor) < 2.f)
			{
				AllInMove = EAllInMove::Hold;
				AllInMoveTime = 0.f;
			}
			break;
		}
		}

		Position.Y = FMath::Clamp(Position.Y, 20.f, BazaSettings::ScreenHeight - 20.f);
		break;
	}

	default:
		break;
	}

	SetGamePosition(Position);
}

void ABazaBossNemy01::OnAttackTick()
{
	if (CurrentPhase == EBazaPhase::Appear || CurrentPhase == EBazaPhase::Hide)
	{
		ShootCanonBall();
	}
	else if (CurrentPhase == EBazaPhase::AllIn && bAllInArrived
		&& (AllInMove == EAllInMove::Hold || AllInMove == EAllInMove::Return))
	{
		// Holds its fire while it winds up and rams, so the charge reads clearly.
		// Every third volley throws a fan instead of a single ball.
		++AllInVolleys;

		ShootCanonBall();
		if (AllInVolleys % 3 == 0)
		{
			ShootCanonBall(-18.f);
			ShootCanonBall(18.f);
		}
	}
}

void ABazaBossNemy01::OnDamaged()
{
	// boss_nemy_01.gd tested "health == totalHealth*0.7", which only ever matched
	// because the numbers happened to land on 700 exactly. A threshold crossing
	// does the same thing without depending on that.
	if (CurrentPhase == EBazaPhase::Appear && GetHealthFraction() <= 0.7f)
	{
		CurrentPhase = EBazaPhase::Hide;
	}
}

void ABazaBossNemy01::ShootCanonBall(float SpreadDegrees)
{
	ABazaGameMode* Stage = GetStage();
	if (!Stage || Stage->IsGameOver())
	{
		return;
	}

	const FVector2D Position = GetGamePosition();

	// The muzzle is on whichever side the boss is facing; after the charge it has
	// turned round and faces right.
	const float Muzzle = (CurrentPhase == EBazaPhase::AllIn) ? 20.f : -20.f;

	ABazaCanonBall* Ball = Stage->SpawnAt<ABazaCanonBall>(FVector2D(Position.X + Muzzle, Position.Y));
	if (!Ball)
	{
		return;
	}

	// Aimed at wherever the player is standing right now. Not normalised, on
	// purpose: the original fired faster the further away the player was, and
	// that is what the fight feels like.
	Ball->Velocity = (Stage->GetPlayerPosition() - Position).GetRotated(SpreadDegrees);

	UBazaSfx::Play(this, EBazaSfx::Cannon);

	if (CurrentPhase == EBazaPhase::Hide)
	{
		++StrikeCount;
		Ball->Phase = EBazaPhase::Hide;

		// A marker on the left edge that tracks the shot the player cannot see yet.
		if (ABazaWarning* Warning = Stage->SpawnAt<ABazaWarning>(FVector2D(0.f, Ball->GetGamePosition().Y)))
		{
			Warning->Threat = Ball;
		}
	}
}
