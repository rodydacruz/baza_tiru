// Basa Tiru - the hit flash. Ported from scripts/explosion.gd and given some
// depth: a core that swells and fades, with shards thrown out of it.

#pragma once

#include "CoreMinimal.h"
#include "BazaActor.h"
#include "BazaExplosion.generated.h"

class UStaticMeshComponent;

UCLASS()
class BAZATIRU_API ABazaExplosion : public ABazaActor
{
	GENERATED_BODY()

public:
	ABazaExplosion();

	virtual void Tick(float DeltaSeconds) override;

	/** Radius in game pixels. A shot landing is small; a boss dying is not. */
	void Configure(float InSizePx);

	static constexpr int32 MaxShards = 8;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Core;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Shards;

	TArray<FVector> ShardVelocities;

	TArray<FRotator> ShardSpins;

	float SizePx = 14.f;

	/** explosion.tscn ran on a 0.1s timer; the shards need a little longer. */
	float Lifetime = 0.28f;

	float Age = 0.f;

	int32 ActiveShards = 0;
};
