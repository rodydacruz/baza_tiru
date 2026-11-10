#include "BazaPowerUp.h"

#include "BazaMeshBuilder.h"
#include "BazaPlayer.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

FLinearColor BazaPowerUps::ColourOf(EBazaPowerUp Type)
{
	switch (Type)
	{
	case EBazaPowerUp::Shield: return FLinearColor(0.2f, 0.75f, 1.f);
	case EBazaPowerUp::Speed:  return FLinearColor(1.f, 0.82f, 0.12f);
	case EBazaPowerUp::Repair: return FLinearColor(0.3f, 0.92f, 0.4f);
	case EBazaPowerUp::Magnet: return FLinearColor(0.92f, 0.14f, 0.2f);
	case EBazaPowerUp::Spread: return FLinearColor(1.f, 0.48f, 0.1f);
	default:                   return FLinearColor::White;
	}
}

const TCHAR* BazaPowerUps::LabelOf(EBazaPowerUp Type)
{
	switch (Type)
	{
	case EBazaPowerUp::Shield: return TEXT("SHIELD");
	case EBazaPowerUp::Speed:  return TEXT("SPEED x2");
	case EBazaPowerUp::Repair: return TEXT("REPAIR");
	case EBazaPowerUp::Magnet: return TEXT("MAGNET");
	case EBazaPowerUp::Spread: return TEXT("SPREAD");
	default:                   return TEXT("?");
	}
}

float BazaPowerUps::DurationOf(EBazaPowerUp Type)
{
	switch (Type)
	{
	case EBazaPowerUp::Shield: return 10.f;
	case EBazaPowerUp::Speed:  return 10.f;
	case EBazaPowerUp::Magnet: return 12.f;
	case EBazaPowerUp::Spread: return 12.f;
	default:                   return 0.f;
	}
}

namespace
{
	// Pickup shapes are laid out in the screen plane - X right, Z up - with their
	// thickness along Y, which points at the camera. Sizes are in Unreal units;
	// a pickup is about 140 across, fourteen game pixels.
	const FVector Depth(0.f, 1.f, 0.f);

	FVector At(float X, float Z, float Y = 0.f)
	{
		return FVector(X, Y, Z);
	}

	/** A hexagon inside a hexagon, the inner one standing proud: a shield crest. */
	void BuildShield(FBazaMeshBuilder& Builder)
	{
		TArray<FVector> Outer;
		TArray<FVector> Inner;

		for (int32 Index = 0; Index < 6; ++Index)
		{
			const float Angle = FMath::DegreesToRadians(60.f * Index + 30.f);
			Outer.Add(At(FMath::Cos(Angle) * 66.f, FMath::Sin(Angle) * 66.f));
			Inner.Add(At(FMath::Cos(Angle) * 36.f, FMath::Sin(Angle) * 36.f, 16.f));
		}

		Builder.AddSlab(Outer, Depth, 10.f);
		Builder.AddSlab(Inner, Depth, 9.f);
	}

	/** Two chevrons, >>, each made of an upper and a lower arm. */
	void BuildSpeed(FBazaMeshBuilder& Builder)
	{
		for (const float C : { -24.f, 26.f })
		{
			Builder.AddSlab({ At(C - 30.f, 56.f), At(C - 8.f, 56.f), At(C + 28.f, 0.f), At(C + 6.f, 0.f) }, Depth, 10.f);
			Builder.AddSlab({ At(C + 6.f, 0.f), At(C + 28.f, 0.f), At(C - 8.f, -56.f), At(C - 30.f, -56.f) }, Depth, 10.f);
		}
	}

	/** A hub with an axle and eight teeth: the repair gear. */
	void BuildGear(FBazaMeshBuilder& Builder)
	{
		Builder.AddPrism(FVector::ZeroVector, Depth, 22.f, 44.f, 12);
		Builder.AddPrism(At(0.f, 0.f, 8.f), Depth, 30.f, 16.f, 8);

		for (int32 Index = 0; Index < 8; ++Index)
		{
			const float Angle = FMath::DegreesToRadians(45.f * Index);
			const FVector Out(FMath::Cos(Angle), 0.f, FMath::Sin(Angle));
			const FVector Side(-FMath::Sin(Angle), 0.f, FMath::Cos(Angle));

			// Tapered teeth, wider at the root, so it reads as a gear and not a star.
			Builder.AddSlab({
				Out * 38.f + Side * 12.f,
				Out * 64.f + Side * 8.f,
				Out * 64.f - Side * 8.f,
				Out * 38.f - Side * 12.f
			}, Depth, 10.f);
		}
	}

	/** A horseshoe magnet, opening upward. The pole tips are separate meshes. */
	void BuildMagnet(FBazaMeshBuilder& Builder)
	{
		Builder.AddSlab({ At(-50.f, -22.f), At(-24.f, -22.f), At(-24.f, 44.f), At(-50.f, 44.f) }, Depth, 12.f);
		Builder.AddSlab({ At(24.f, -22.f), At(50.f, -22.f), At(50.f, 44.f), At(24.f, 44.f) }, Depth, 12.f);

		// The bend, cut at the corners so it is not just a box.
		Builder.AddSlab({
			At(-50.f, -22.f), At(-38.f, -50.f), At(38.f, -50.f), At(50.f, -22.f)
		}, Depth, 12.f);
	}

	/** Three arrowheads fanned out: the spread shot. */
	void BuildSpread(FBazaMeshBuilder& Builder)
	{
		for (const float Degrees : { -32.f, 0.f, 32.f })
		{
			const float Angle = FMath::DegreesToRadians(Degrees);
			const FVector Forward(FMath::Cos(Angle), 0.f, FMath::Sin(Angle));
			const FVector Side(-FMath::Sin(Angle), 0.f, FMath::Cos(Angle));
			const FVector Base = Forward * 6.f;

			Builder.AddSlab({
				Base + Forward * 66.f,
				Base - Forward * 8.f + Side * 26.f,
				Base - Forward * 8.f - Side * 26.f
			}, Depth, 13.f);
		}
	}
}

ABazaPowerUp::ABazaPowerUp()
{
	// Generous, so a near miss still counts.
	HitZone = CreateHitSphere(TEXT("HitZone"), 10.f);

	Body = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(RootComponent);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetCastShadow(false);
	Body->bUseAsyncCooking = false;

	ConstructorHelpers::FObjectFinder<UMaterialInterface> BodyMaterial(BazaMesh::Material);
	if (BodyMaterial.Succeeded())
	{
		Body->SetMaterial(0, BodyMaterial.Object);
	}

	RegisterTintedMesh(Body, FLinearColor::White);

	TipLeft = CreateShape(TEXT("TipLeft"), BazaMesh::Cube, FVector(2.8f, 2.6f, 1.6f), FLinearColor(0.9f, 0.92f, 0.95f));
	TipLeft->SetRelativeLocation(FVector(-37.f, 0.f, 52.f));
	TipLeft->SetVisibility(false);

	TipRight = CreateShape(TEXT("TipRight"), BazaMesh::Cube, FVector(2.8f, 2.6f, 1.6f), FLinearColor(0.9f, 0.92f, 0.95f));
	TipRight->SetRelativeLocation(FVector(37.f, 0.f, 52.f));
	TipRight->SetVisibility(false);
}

void ABazaPowerUp::BeginPlay()
{
	Super::BeginPlay();

	BaseY = GetGamePosition().Y;
	Age = FMath::FRandRange(0.f, 6.f);

	OnActorBeginOverlap.AddDynamic(this, &ABazaPowerUp::HandleOverlap);

	// Anything spawned without a Configure still gets a shape.
	Configure(Type);
}

void ABazaPowerUp::Configure(EBazaPowerUp InType)
{
	Type = InType;

	FBazaMeshBuilder Builder;

	switch (Type)
	{
	case EBazaPowerUp::Shield: BuildShield(Builder); break;
	case EBazaPowerUp::Speed:  BuildSpeed(Builder);  break;
	case EBazaPowerUp::Repair: BuildGear(Builder);   break;
	case EBazaPowerUp::Magnet: BuildMagnet(Builder); break;
	case EBazaPowerUp::Spread: BuildSpread(Builder); break;
	default: break;
	}

	Builder.Commit(Body);
	SetMeshTint(Body, BazaPowerUps::ColourOf(Type));

	const bool bMagnet = Type == EBazaPowerUp::Magnet;
	TipLeft->SetVisibility(bMagnet);
	TipRight->SetVisibility(bMagnet);
}

void ABazaPowerUp::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Age += DeltaSeconds;

	FVector2D Position = GetGamePosition();
	Position.X -= DriftSpeed * DeltaSeconds;
	Position.Y = BaseY + FMath::Sin(Age * 3.f) * 6.f;
	SetGamePosition(Position);

	// Swinging to and fro about the screen's vertical rather than spinning flat,
	// so the thickness of the shape shows and it never turns edge-on for long.
	SetActorRotation(FRotator(0.f, FMath::Sin(Age * 2.1f) * 38.f, 0.f));

	// A slow pulse, so it reads as something to pick up rather than scenery. The
	// base scale is up on the modelled size: at 320x180 a fourteen-pixel pickup
	// was hard to tell apart from a speck.
	const float Pulse = 1.f + 0.1f * FMath::Sin(Age * 7.f);
	SetActorScale3D(FVector(Pulse * 1.4f));

	if (Position.X < -20.f)
	{
		Destroy();
	}
}

void ABazaPowerUp::HandleOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	ABazaPlayer* Player = Cast<ABazaPlayer>(OtherActor);
	if (!Player || bCollected || Player->IsHidden())
	{
		return;
	}

	bCollected = true;
	Player->ApplyPowerUp(Type);
	Remove();
}
