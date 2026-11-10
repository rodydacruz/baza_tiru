// Basa Tiru - BossNemy02, "Spelhu" (the mirror). New for the Unreal version.
//
// Spelhu does not chase. It is the ship's echo: it records where the player
// went and when they fired, and plays it all back a second or two later,
// mirrored so it faces the ship from the right. The fight is about leading it -
// shoot at the row it is about to arrive on, and be somewhere else by the time
// your own volley comes back at you.

#pragma once

#include "CoreMinimal.h"
#include "BazaBossBase.h"
#include "BazaBossNemy02.generated.h"

class UStaticMeshComponent;

UCLASS()
class BAZATIRU_API ABazaBossNemy02 : public ABazaBossBase
{
	GENERATED_BODY()

public:
	ABazaBossNemy02();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void UpdateMovement(float DeltaSeconds) override;
	virtual void OnDamaged() override;

	/** Notes where the ship is this frame, and whether it just fired. */
	void RecordPlayer();

	/** Plays back every recorded frame that is now old enough, firing where the ship fired. */
	void ReplayEcho();

	/** True when a recorded volley is about to come back, so the eye can warn. */
	bool IsVolleyImminent() const;

	/** The ship's volley, turned round: two bolts back along the row. */
	void FireEcho(float PlayerFacing);

	/** How far behind the ship it runs, in seconds. Shrinks as it takes damage. */
	float GetEchoDelay() const;

	/** Where the boss stands for a recorded ship position. */
	FVector2D MirrorOf(const FVector2D& PlayerPosition) const;

private:
	struct FEchoSample
	{
		float Time = 0.f;
		FVector2D Position = FVector2D::ZeroVector;
		float Facing = 1.f;
		bool bFired = false;
	};

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Pane;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> FrameTop;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> FrameBottom;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Eye;

	/** Oldest first; trimmed as each sample is played back. */
	TArray<FEchoSample> Recording;

	/** The recorded ship position it is currently replaying. */
	FVector2D EchoTarget = FVector2D::ZeroVector;

	bool bHasEchoTarget = false;

	int32 LastVolleyCount = 0;

	/** Faster than the ship, so it keeps pace with the replay instead of lagging it. */
	float Speed = 240.f;

	/** Cached so the eye is only recoloured when the state actually changes. */
	bool bWasWarning = false;

	float EyePulse = 0.f;
};
