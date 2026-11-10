// Basa Tiru - procedural low-poly ship generator.
//
// One parametrised hull serves the player and the gunships: a faceted fuselage
// with a raised canopy and a keel, swept wings, a dorsal fin and engine pods.
// Change the proportions and the same builder gives a different ship.

#pragma once

#include "CoreMinimal.h"

class UProceduralMeshComponent;

struct FBazaShipParams
{
	/** Nose to tail, in Unreal units. Everything else is relative to it. */
	float Length = 150.f;

	/** Wingtip to wingtip. */
	float Span = 130.f;

	/** Canopy peak to keel. This is the axis pointing at the camera. */
	float Depth = 55.f;

	/** How far back the wingtips sit, 0 straight out to 1 fully swept. */
	float WingSweep = 0.55f;

	/** 0 for a stub nose, 1 for a long dart. */
	float NoseLength = 1.f;

	/** Wing thickness as a fraction of Depth. */
	float WingThickness = 0.12f;

	bool bDorsalFin = true;

	bool bEnginePods = true;

	/** Mirrors the hull so the ship faces down -X instead of +X. */
	bool bFacesBackwards = false;
};

namespace BazaShipMesh
{
	/** Replaces section 0 of Mesh with a freshly built ship. */
	BAZATIRU_API void Build(UProceduralMeshComponent* Mesh, const FBazaShipParams& Params);
}
