// Basa Tiru - the scrolling backdrop. Ported from scripts/background.gd.

#pragma once

#include "CoreMinimal.h"
#include "BazaActor.h"
#include "BazaBackground.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMeshComponent;

/**
 * The original nebula from sprites/background.png, scrolling at the same speed and
 * wrapping the same way it always did, with a parallax starfield in front of it so
 * the backdrop has some depth now that there is depth to have.
 */
UCLASS()
class BAZATIRU_API ABazaBackground : public ABazaActor
{
	GENERATED_BODY()

public:
	ABazaBackground();

	virtual void Tick(float DeltaSeconds) override;

	/**
	 * Everything back here has to be scaled up to cover the same screen area it
	 * would at the play plane, so the stage hands us how far away the camera is.
	 */
	void FitToCamera(float InCameraDistance);

	/** background.gd: var scroll_speed = 30.0 */
	UPROPERTY(EditAnywhere, Category = "Basa Tiru")
	float ScrollSpeed = 30.f;

	/** Set to zero for just the nebula, with no 3D stars in front of it. */
	UPROPERTY(EditAnywhere, Category = "Basa Tiru")
	int32 StarCount = 70;

protected:
	virtual void BeginPlay() override;

	/** Places a point given in game pixels at Depth units behind the play plane. */
	FVector PlaceAtDepth(const FVector2D& GamePos, float Depth) const;

private:
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Backdrop;

	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> Stars;

	/** background.gd tracked this as the sprite's own position.x. */
	float ScrollX = 0.f;

	float CameraDistance = 3431.f;

	/** Star position in game pixels, plus how far back it sits. */
	TArray<FVector> StarPositions;

	TArray<float> StarParallax;

	TArray<FVector> StarScales;
};
