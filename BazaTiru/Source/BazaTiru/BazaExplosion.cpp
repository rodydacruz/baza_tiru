#include "BazaExplosion.h"

#include "Components/StaticMeshComponent.h"

ABazaExplosion::ABazaExplosion()
{
	Core = CreateShape(TEXT("Core"), BazaMesh::Sphere, FVector(1.f, 1.f, 1.f), FLinearColor(1.f, 0.88f, 0.35f));

	// Built up front and switched on as needed, so a burst never has to allocate.
	Shards.Reserve(MaxShards);
	for (int32 Index = 0; Index < MaxShards; ++Index)
	{
		UStaticMeshComponent* Shard = CreateShape(
			*FString::Printf(TEXT("Shard%d"), Index),
			BazaMesh::Cube,
			FVector(1.f, 1.f, 1.f),
			FLinearColor(1.f, 0.45f, 0.12f));

		Shard->SetVisibility(false);
		Shards.Add(Shard);
	}
}

void ABazaExplosion::Configure(float InSizePx)
{
	SizePx = FMath::Max(InSizePx, 2.f);

	// Bigger blasts hang around longer and throw more pieces.
	const float SizeAlpha = FMath::Clamp((SizePx - 8.f) / (60.f - 8.f), 0.f, 1.f);
	Lifetime = FMath::Lerp(0.18f, 0.7f, SizeAlpha);
	ActiveShards = FMath::Clamp(FMath::RoundToInt(SizePx * 0.22f), 3, MaxShards);

	SetLifeSpan(Lifetime);

	// Configure lands after BeginPlay, so the shards have to be armed here.
	for (int32 Index = 0; Index < Shards.Num(); ++Index)
	{
		const bool bActive = Index < ActiveShards;
		Shards[Index]->SetVisibility(bActive);

		if (!bActive || !ShardVelocities.IsValidIndex(Index))
		{
			continue;
		}

		const float Speed = BazaSettings::Px(SizePx) * FMath::FRandRange(1.6f, 3.4f);
		const float Angle = FMath::DegreesToRadians(360.f * Index / ActiveShards)
			+ FMath::FRandRange(-0.4f, 0.4f);

		// Thrown within the play plane, with a little depth so they read as 3D.
		ShardVelocities[Index] = FVector(
			FMath::Cos(Angle) * Speed,
			FMath::FRandRange(-0.35f, 0.35f) * Speed,
			FMath::Sin(Angle) * Speed);

		const float ShardSize = BazaSettings::Px(SizePx) * FMath::FRandRange(0.1f, 0.22f);
		Shards[Index]->SetRelativeScale3D(FVector(ShardSize) / 100.f);
		Shards[Index]->SetRelativeLocation(FVector::ZeroVector);
	}
}

void ABazaExplosion::BeginPlay()
{
	Super::BeginPlay();

	ShardVelocities.SetNum(MaxShards);
	ShardSpins.SetNum(MaxShards);

	for (int32 Index = 0; Index < MaxShards; ++Index)
	{
		ShardSpins[Index] = FRotator(
			FMath::FRandRange(-720.f, 720.f),
			FMath::FRandRange(-720.f, 720.f),
			FMath::FRandRange(-720.f, 720.f));
	}

	Configure(SizePx);
}

void ABazaExplosion::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Age += DeltaSeconds;

	const float Alpha = FMath::Clamp(Age / Lifetime, 0.f, 1.f);

	// The core swells fast and dims as it goes, the way the sprite used to pop.
	const float CoreSize = BazaSettings::Px(SizePx) * FMath::InterpEaseOut(0.5f, 1.9f, Alpha, 2.f);
	Core->SetRelativeScale3D(FVector(CoreSize) / 100.f);

	SetVisualOpacity(1.f - Alpha * Alpha);

	for (int32 Index = 0; Index < ActiveShards; ++Index)
	{
		if (!Shards.IsValidIndex(Index))
		{
			continue;
		}

		// Slowing as they go, so the burst settles instead of flying off forever.
		const float Drag = FMath::Lerp(1.f, 0.25f, Alpha);

		Shards[Index]->AddLocalOffset(ShardVelocities[Index] * Drag * DeltaSeconds);
		Shards[Index]->AddLocalRotation(ShardSpins[Index] * DeltaSeconds);
	}
}
