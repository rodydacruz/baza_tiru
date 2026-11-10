#include "BazaRockMesh.h"

#include "ProceduralMeshComponent.h"

namespace
{
	/** One triangle of the working icosphere, as indices into the vertex array. */
	struct FTri
	{
		int32 A;
		int32 B;
		int32 C;
	};

	/** Key for the midpoint cache, so a shared edge is only split once. */
	uint64 EdgeKey(int32 First, int32 Second)
	{
		const uint64 Low = FMath::Min(First, Second);
		const uint64 High = FMath::Max(First, Second);
		return (Low << 32) | High;
	}

	int32 MidPoint(int32 First, int32 Second, TArray<FVector>& Points, TMap<uint64, int32>& Cache)
	{
		const uint64 Key = EdgeKey(First, Second);
		if (const int32* Existing = Cache.Find(Key))
		{
			return *Existing;
		}

		// Back onto the unit sphere, or subdividing would flatten it out.
		const FVector Mid = ((Points[First] + Points[Second]) * 0.5f).GetSafeNormal();

		const int32 Index = Points.Add(Mid);
		Cache.Add(Key, Index);

		return Index;
	}

	/** The twelve corners and twenty faces of an icosahedron, on the unit sphere. */
	void BuildIcosahedron(TArray<FVector>& Points, TArray<FTri>& Faces)
	{
		const float T = (1.f + FMath::Sqrt(5.f)) * 0.5f;

		const FVector Raw[12] = {
			{ -1.f,  T, 0.f }, {  1.f,  T, 0.f }, { -1.f, -T, 0.f }, {  1.f, -T, 0.f },
			{ 0.f, -1.f,  T }, { 0.f,  1.f,  T }, { 0.f, -1.f, -T }, { 0.f,  1.f, -T },
			{  T, 0.f, -1.f }, {  T, 0.f,  1.f }, { -T, 0.f, -1.f }, { -T, 0.f,  1.f }
		};

		Points.Reset();
		for (const FVector& Point : Raw)
		{
			Points.Add(Point.GetSafeNormal());
		}

		const int32 RawFaces[20][3] = {
			{ 0, 11,  5 }, { 0,  5,  1 }, { 0,  1,  7 }, { 0,  7, 10 }, { 0, 10, 11 },
			{ 1,  5,  9 }, { 5, 11,  4 }, { 11, 10, 2 }, { 10, 7,  6 }, { 7,  1,  8 },
			{ 3,  9,  4 }, { 3,  4,  2 }, { 3,  2,  6 }, { 3,  6,  8 }, { 3,  8,  9 },
			{ 4,  9,  5 }, { 2,  4, 11 }, { 6,  2, 10 }, { 8,  6,  7 }, { 9,  8,  1 }
		};

		Faces.Reset();
		for (const auto& Face : RawFaces)
		{
			Faces.Add({ Face[0], Face[1], Face[2] });
		}
	}
}

void BazaRockMesh::Build(UProceduralMeshComponent* Mesh, const FBazaRockParams& Params)
{
	if (!Mesh)
	{
		return;
	}

	TArray<FVector> Points;
	TArray<FTri> Faces;
	BuildIcosahedron(Points, Faces);

	// Split every face into four, as many times as asked.
	for (int32 Pass = 0; Pass < FMath::Clamp(Params.Subdivisions, 0, 3); ++Pass)
	{
		TMap<uint64, int32> Cache;
		TArray<FTri> Split;
		Split.Reserve(Faces.Num() * 4);

		for (const FTri& Face : Faces)
		{
			const int32 AB = MidPoint(Face.A, Face.B, Points, Cache);
			const int32 BC = MidPoint(Face.B, Face.C, Points, Cache);
			const int32 CA = MidPoint(Face.C, Face.A, Points, Cache);

			Split.Add({ Face.A, AB, CA });
			Split.Add({ Face.B, BC, AB });
			Split.Add({ Face.C, CA, BC });
			Split.Add({ AB, BC, CA });
		}

		Faces = MoveTemp(Split);
	}

	FRandomStream Random(Params.Seed);

	// A few broad swells in random directions. Summing sines of the dot product
	// against each one gives a surface that varies smoothly all the way round,
	// with no seam, which per-vertex noise on its own would not.
	constexpr int32 LobeCount = 4;
	FVector LobeAxis[LobeCount];
	float LobeAmplitude[LobeCount];
	float LobeFrequency[LobeCount];

	for (int32 Index = 0; Index < LobeCount; ++Index)
	{
		LobeAxis[Index] = Random.VRand();
		LobeAmplitude[Index] = Random.FRandRange(0.4f, 1.f) / LobeCount;
		LobeFrequency[Index] = Random.FRandRange(1.2f, 3.2f);
	}

	// Squashed a little differently on each axis, so no two are the same solid.
	const FVector Stretch(
		1.f + Random.FRandRange(-Params.MaxStretch, Params.MaxStretch),
		1.f + Random.FRandRange(-Params.MaxStretch, Params.MaxStretch),
		1.f + Random.FRandRange(-Params.MaxStretch, Params.MaxStretch));

	TArray<FVector> Displaced;
	Displaced.Reserve(Points.Num());

	for (const FVector& Direction : Points)
	{
		float Offset = 0.f;
		for (int32 Index = 0; Index < LobeCount; ++Index)
		{
			Offset += LobeAmplitude[Index]
				* FMath::Sin(LobeFrequency[Index] * FVector::DotProduct(Direction, LobeAxis[Index]) * PI);
		}

		Offset += Random.FRandRange(-Params.Grit, Params.Grit);

		const float Scale = Params.Radius * (1.f + Offset * Params.Lumpiness);
		Displaced.Add(Direction * Scale * Stretch);
	}

	// Flat shading: every triangle gets its own three vertices and one normal, so
	// the rock reads as facets rather than a smooth blob. That suits the look, and
	// it is what makes the silhouette catch the light.
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FProcMeshTangent> Tangents;
	TArray<FLinearColor> Colors;

	const int32 VertexCount = Faces.Num() * 3;
	Vertices.Reserve(VertexCount);
	Triangles.Reserve(VertexCount);
	Normals.Reserve(VertexCount);
	UVs.Reserve(VertexCount);

	for (const FTri& Face : Faces)
	{
		const FVector& A = Displaced[Face.A];
		const FVector& B = Displaced[Face.B];
		const FVector& C = Displaced[Face.C];

		const FVector Normal = FVector::CrossProduct(B - A, C - A).GetSafeNormal();

		const int32 Base = Vertices.Num();
		Vertices.Add(A);
		Vertices.Add(B);
		Vertices.Add(C);

		for (int32 Index = 0; Index < 3; ++Index)
		{
			Normals.Add(Normal);
			Tangents.Add(FProcMeshTangent(1.f, 0.f, 0.f));
		}

		UVs.Add(FVector2D(0.f, 0.f));
		UVs.Add(FVector2D(1.f, 0.f));
		UVs.Add(FVector2D(0.f, 1.f));

		Triangles.Add(Base);
		Triangles.Add(Base + 1);
		Triangles.Add(Base + 2);
	}

	Mesh->CreateMeshSection_LinearColor(
		0, Vertices, Triangles, Normals, UVs, Colors, Tangents, /*bCreateCollision=*/false);
}
