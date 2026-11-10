#include "BazaBackground.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** sprites/background.png is 640x180: two screen widths, made to loop. */
	constexpr float BackdropWidth = 640.f;
	constexpr float BackdropHeight = 180.f;

	/** background.tscn placed the sprite at (0, -2), uncentred. */
	constexpr float BackdropTop = -2.f;

	/** How far behind the play plane each layer sits, in Unreal units. */
	constexpr float BackdropDepth = 1500.f;
	constexpr float NearStarDepth = 200.f;
	constexpr float FarStarDepth = 1100.f;
}

ABazaBackground::ABazaBackground()
{
	Backdrop = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Backdrop"));
	Backdrop->SetupAttachment(RootComponent);
	Backdrop->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Backdrop->SetCastShadow(false);

	// Engine planes lie in XY facing +Z; rolling back a quarter turn stands it up
	// in the play plane with its face towards the camera.
	Backdrop->SetRelativeRotation(FRotator(0.f, 0.f, -90.f));

	ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneAsset(BazaMesh::Plane);
	if (PlaneAsset.Succeeded())
	{
		Backdrop->SetStaticMesh(PlaneAsset.Object);
	}

	ConstructorHelpers::FObjectFinder<UMaterialInterface> BackdropMaterial(BazaArt::BackdropMaterial);
	if (BackdropMaterial.Succeeded())
	{
		Backdrop->SetMaterial(0, BackdropMaterial.Object);
	}

	Stars = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Stars"));
	Stars->SetupAttachment(RootComponent);
	Stars->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Stars->SetCastShadow(false);

	ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(BazaMesh::Cube);
	if (CubeAsset.Succeeded())
	{
		Stars->SetStaticMesh(CubeAsset.Object);
	}

	ConstructorHelpers::FObjectFinder<UMaterialInterface> StarMaterial(BazaMesh::Material);
	if (StarMaterial.Succeeded())
	{
		Stars->SetMaterial(0, StarMaterial.Object);
	}
}

void ABazaBackground::FitToCamera(float InCameraDistance)
{
	CameraDistance = FMath::Max(InCameraDistance, 1.f);
}

void ABazaBackground::BeginPlay()
{
	Super::BeginPlay();

	if (UMaterialInstanceDynamic* Dynamic = Backdrop->CreateAndSetMaterialInstanceDynamic(0))
	{
		if (UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, BazaArt::BackgroundTexture))
		{
			Dynamic->SetTextureParameterValue(TEXT("Tex"), Texture);
		}
	}

	StarPositions.Reserve(StarCount);
	StarParallax.Reserve(StarCount);
	StarScales.Reserve(StarCount);

	for (int32 Index = 0; Index < StarCount; ++Index)
	{
		const float Depth = FMath::FRandRange(NearStarDepth, FarStarDepth);
		const float Alpha = (Depth - NearStarDepth) / (FarStarDepth - NearStarDepth);

		StarPositions.Add(FVector(
			FMath::FRandRange(0.f, BazaSettings::ScreenWidth),
			FMath::FRandRange(-20.f, BazaSettings::ScreenHeight + 20.f),
			Depth));

		// Near stars sweep past faster than the nebula, distant ones slower.
		StarParallax.Add(FMath::Lerp(2.2f, 0.7f, Alpha));

		const float SizePx = FMath::Lerp(1.6f, 0.7f, Alpha);
		const FVector Scale = FVector(BazaSettings::Px(SizePx)) / 100.f;
		StarScales.Add(Scale);

		FTransform Transform;
		Transform.SetScale3D(Scale);
		Stars->AddInstance(Transform);
	}
}

FVector ABazaBackground::PlaceAtDepth(const FVector2D& GamePos, float Depth) const
{
	// Further from the camera means a smaller slice of the world fills the frame,
	// so a point back here has to be pushed out from the centre to stay put on
	// screen. Same factor scales the mesh, so it covers what it is meant to cover.
	const FVector2D Centre(BazaSettings::ScreenWidth * 0.5f, BazaSettings::ScreenHeight * 0.5f);
	const float Perspective = (CameraDistance + Depth) / CameraDistance;

	FVector Location = BazaSettings::ToWorld(Centre + (GamePos - Centre) * Perspective);
	Location.Y = -Depth;

	return Location;
}

void ABazaBackground::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// background.gd, unchanged: drift left, and jump back one screen width. The
	// image is two screens wide, so the seam never shows.
	ScrollX -= ScrollSpeed * DeltaSeconds;
	if (ScrollX <= -BazaSettings::ScreenWidth)
	{
		ScrollX += BazaSettings::ScreenWidth;
	}

	const float Perspective = (CameraDistance + BackdropDepth) / CameraDistance;

	const FVector2D BackdropCentre(
		ScrollX + BackdropWidth * 0.5f,
		BackdropTop + BackdropHeight * 0.5f);

	Backdrop->SetWorldLocation(PlaceAtDepth(BackdropCentre, BackdropDepth));
	Backdrop->SetRelativeScale3D(FVector(
		BazaSettings::Px(BackdropWidth) / 100.f * Perspective,
		BazaSettings::Px(BackdropHeight) / 100.f * Perspective,
		1.f));

	for (int32 Index = 0; Index < StarPositions.Num(); ++Index)
	{
		FVector& Star = StarPositions[Index];

		Star.X -= ScrollSpeed * StarParallax[Index] * DeltaSeconds;
		if (Star.X <= 0.f)
		{
			Star.X += BazaSettings::ScreenWidth;
		}

		const float Depth = Star.Z;
		const float StarPerspective = (CameraDistance + Depth) / CameraDistance;

		FTransform Transform;
		Transform.SetLocation(PlaceAtDepth(FVector2D(Star.X, Star.Y), Depth));
		Transform.SetScale3D(StarScales[Index] * StarPerspective);

		Stars->UpdateInstanceTransform(Index, Transform, /*bWorldSpace=*/true, /*bMarkRenderStateDirty=*/false);
	}

	if (StarPositions.Num() > 0)
	{
		Stars->MarkRenderStateDirty();
	}
}
