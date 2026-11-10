#include "BazaShipMesh.h"

#include "BazaMeshBuilder.h"
#include "ProceduralMeshComponent.h"

void BazaShipMesh::Build(UProceduralMeshComponent* Mesh, const FBazaShipParams& Params)
{
	if (!Mesh)
	{
		return;
	}

	FBazaMeshBuilder Builder;

	// Laid out in normalised ship space and scaled on the way out:
	//   X  nose (+1) to tail (-1)
	//   Y  keel (-1) to canopy (+1) - this axis points at the camera
	//   Z  wingspan, left (-1) to right (+1)
	const float HalfLength = Params.Length * 0.5f;
	const float HalfSpan = Params.Span * 0.5f;
	const float HalfDepth = Params.Depth * 0.5f;
	const float Mirror = Params.bFacesBackwards ? -1.f : 1.f;

	auto P = [=](float X, float Y, float Z)
	{
		return FVector(X * HalfLength * Mirror, Y * HalfDepth, Z * HalfSpan);
	};

	// Hull. A spine and a keel down the centreline, with the widest point of the
	// fuselage at the shoulders where the wings meet it.
	const FVector Nose = P(0.55f + 0.45f * Params.NoseLength, 0.f, 0.f);
	const FVector SpineFront = P(0.26f, 1.f, 0.f);
	const FVector SpineBack = P(-0.58f, 0.34f, 0.f);
	const FVector KeelFront = P(0.2f, -0.78f, 0.f);
	const FVector KeelBack = P(-0.58f, -0.24f, 0.f);

	const FVector ShoulderL = P(0.16f, 0.f, -0.3f);
	const FVector ShoulderR = P(0.16f, 0.f, 0.3f);
	const FVector TailL = P(-0.82f, 0.f, -0.34f);
	const FVector TailR = P(-0.82f, 0.f, 0.34f);

	// Every hull face is turned outward from the middle of the fuselage, so none
	// of this has to be wound by hand.
	const FVector Inside = P(-0.1f, 0.f, 0.f);

	// Upper hull, both sides.
	Builder.AddTriFacingOut(Nose, SpineFront, ShoulderL, Inside);
	Builder.AddTriFacingOut(SpineFront, SpineBack, TailL, Inside);
	Builder.AddTriFacingOut(SpineFront, TailL, ShoulderL, Inside);

	Builder.AddTriFacingOut(Nose, SpineFront, ShoulderR, Inside);
	Builder.AddTriFacingOut(SpineFront, SpineBack, TailR, Inside);
	Builder.AddTriFacingOut(SpineFront, TailR, ShoulderR, Inside);

	// Lower hull.
	Builder.AddTriFacingOut(Nose, KeelFront, ShoulderL, Inside);
	Builder.AddTriFacingOut(KeelFront, KeelBack, TailL, Inside);
	Builder.AddTriFacingOut(KeelFront, TailL, ShoulderL, Inside);

	Builder.AddTriFacingOut(Nose, KeelFront, ShoulderR, Inside);
	Builder.AddTriFacingOut(KeelFront, KeelBack, TailR, Inside);
	Builder.AddTriFacingOut(KeelFront, TailR, ShoulderR, Inside);

	// Tail plate, closing the back.
	Builder.AddTriFacingOut(SpineBack, TailL, KeelBack, Inside);
	Builder.AddTriFacingOut(SpineBack, KeelBack, TailR, Inside);

	// Wings. Flat slabs lying in the hull's plane, swept back from the shoulder.
	const float TipX = FMath::Lerp(0.05f, -0.62f, Params.WingSweep);
	const float WingHalfThickness = HalfDepth * Params.WingThickness;

	for (const float Side : { -1.f, 1.f })
	{
		const TArray<FVector> Wing = {
			P(0.18f, 0.f, 0.26f * Side),
			P(TipX, 0.f, 1.f * Side),
			P(TipX - 0.3f, 0.f, 0.94f * Side),
			P(-0.84f, 0.f, 0.3f * Side)
		};

		Builder.AddSlab(Wing, FVector(0.f, 1.f, 0.f), WingHalfThickness);
	}

	// A fin standing up out of the spine, which is what stops the ship reading as
	// a flat cut-out when it banks.
	if (Params.bDorsalFin)
	{
		const TArray<FVector> Fin = {
			P(-0.24f, 0.42f, 0.f),
			P(-0.52f, 1.15f, 0.f),
			P(-0.84f, 1.05f, 0.f),
			P(-0.8f, 0.2f, 0.f)
		};

		Builder.AddSlab(Fin, FVector(0.f, 0.f, 1.f), HalfSpan * 0.05f);
	}

	// Engine pods tucked under the wing roots.
	if (Params.bEnginePods)
	{
		for (const float Side : { -1.f, 1.f })
		{
			Builder.AddPrism(
				P(-0.55f, -0.12f, 0.36f * Side),
				FVector(Mirror, 0.f, 0.f),
				HalfLength * 0.46f,
				HalfDepth * 0.3f,
				6);
		}
	}

	Builder.Commit(Mesh);
}
