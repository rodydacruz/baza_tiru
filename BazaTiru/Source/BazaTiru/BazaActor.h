// Basa Tiru - base class for everything that lives on the play plane.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BazaSettings.h"
#include "BazaActor.generated.h"

class ABazaGameMode;
class UBoxComponent;
class UMaterialInstanceDynamic;
class USphereComponent;
class UStaticMeshComponent;

/** Godot: signal score(points, enemy_id) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBazaScoreSignature, int32, Points, int32, EnemyId);

/**
 * Godot's Node2D/Area2D, rebuilt on a flat plane in 3D.
 *
 * Subclasses keep their position in Godot screen pixels (X right, Y down, origin
 * top-left) via GamePosition, exactly like the .gd scripts did, and the actor's
 * real world transform is derived from it. That way every ported line of movement
 * code keeps its original numbers.
 */
UCLASS(Abstract)
class BAZATIRU_API ABazaActor : public AActor
{
	GENERATED_BODY()

public:
	ABazaActor();

	virtual void Tick(float DeltaSeconds) override;

	/** Broadcast by enemies when the player kills them; the game mode listens. */
	UPROPERTY(BlueprintAssignable, Category = "Basa Tiru")
	FBazaScoreSignature OnScore;

	FVector2D GetGamePosition() const { return GamePosition; }
	void SetGamePosition(const FVector2D& NewPosition);

	/** Blows the whole actor out to white for a moment, to sell a hit landing. */
	void FlashHit(float Seconds = 0.08f);

	/** Recolours one registered mesh, for parts that change state. */
	void SetMeshTint(UMeshComponent* Mesh, const FLinearColor& Colour);

	/**
	 * Takes the actor out of play now and deletes it a moment later. Use this
	 * instead of Destroy() inside an overlap handler: both sides of an overlap are
	 * told about it one after the other, and destroying one outright can stop the
	 * other ever hearing about the hit.
	 */
	void Remove();

	bool IsRemoved() const { return bRemoved; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Position in Godot screen pixels: X right, Y down, origin top-left. */
	UPROPERTY(EditAnywhere, Category = "Basa Tiru")
	FVector2D GamePosition = FVector2D::ZeroVector;

	/** Rotation within the play plane, in Godot degrees (positive is clockwise on screen). */
	void SetGameRotation(float Degrees);

	ABazaGameMode* GetStage() const;

	/** Size is the flash radius in game pixels; bigger deaths get bigger blasts. */
	void SpawnExplosionAt(const FVector2D& Where, float SizePx = 14.f);

	/** Fades every mesh towards a colour; used for the respawn blink. */
	void SetVisualOpacity(float Opacity);

	// --- construction helpers; only valid from a subclass constructor ---

	/** Overlap-only collision, the 3D equivalent of a CollisionShape2D under an Area2D. */
	USphereComponent* CreateHitSphere(FName Name, float RadiusPx);
	UBoxComponent* CreateHitBox(FName Name, FVector2D SizePx);

	/**
	 * Stand-in art. SizePx is measured in Godot pixels along
	 * (screen X, depth, screen Y).
	 */
	UStaticMeshComponent* CreateShape(FName Name, const TCHAR* MeshPath, FVector SizePx, FLinearColor Color);

	/**
	 * Brings a mesh the subclass built itself into the tint, flash and fade
	 * system. CreateShape does this for its own meshes.
	 */
	void RegisterTintedMesh(UMeshComponent* Mesh, FLinearColor Color);

private:
	void PushTints();

	/** Registered meshes, turned into dynamic materials in BeginPlay. */
	UPROPERTY()
	TArray<TObjectPtr<UMeshComponent>> TintedMeshes;

	UPROPERTY()
	TArray<FLinearColor> TintColors;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> TintMaterials;

	float FlashRemaining = 0.f;

	float VisualOpacity = 1.f;

	bool bRemoved = false;
};
