// Basa Tiru - procedural rock generator.
//
// The asteroids were a cube each, which reads as a crate rather than a rock. This
// builds a lumpy, faceted solid instead: an icosphere pushed about by a handful of
// smooth lobes and then flat-shaded, so every asteroid is plainly the same kind of
// thing and plainly not the same one.

#pragma once

#include "CoreMinimal.h"

class UProceduralMeshComponent;

/** Knobs on the shape. The defaults make a believable small asteroid. */
struct FBazaRockParams
{
	/** Same seed, same rock, every run. */
	int32 Seed = 0;

	/** Mean radius, in Unreal units. */
	float Radius = 100.f;

	/** 0 gives 20 faces, 1 gives 80, 2 gives 320. One is plenty at this scale. */
	int32 Subdivisions = 1;

	/** How far the surface strays from a sphere, as a fraction of the radius. */
	float Lumpiness = 0.28f;

	/** How far each rock is allowed to be stretched or squashed per axis. */
	float MaxStretch = 0.3f;

	/** Per-vertex roughening on top of the smooth lobes. */
	float Grit = 0.05f;
};

namespace BazaRockMesh
{
	/** Replaces section 0 of Mesh with a freshly generated rock. */
	BAZATIRU_API void Build(UProceduralMeshComponent* Mesh, const FBazaRockParams& Params);
}
