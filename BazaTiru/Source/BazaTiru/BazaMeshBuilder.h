// Basa Tiru - a small flat-shaded mesh builder.
//
// Everything built here is faceted on purpose: each triangle carries its own
// vertices and its own normal, so edges stay hard and the silhouette catches the
// light. That is the low-poly read, and it survives being rendered at 320x180.

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"

struct FBazaMeshBuilder
{
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FProcMeshTangent> Tangents;

	/** Winding is clockwise seen from the front, which is where the normal points. */
	void AddTri(const FVector& A, const FVector& B, const FVector& C)
	{
		const FVector Normal = FVector::CrossProduct(B - A, C - A).GetSafeNormal();

		// A degenerate triangle has no normal and would render as a black sliver.
		if (Normal.IsNearlyZero())
		{
			return;
		}

		const int32 Base = Vertices.Num();

		Vertices.Add(A);
		Vertices.Add(B);
		Vertices.Add(C);

		UVs.Add(FVector2D(0.f, 0.f));
		UVs.Add(FVector2D(1.f, 0.f));
		UVs.Add(FVector2D(0.f, 1.f));

		for (int32 Index = 0; Index < 3; ++Index)
		{
			Normals.Add(Normal);
			Tangents.Add(FProcMeshTangent(1.f, 0.f, 0.f));
		}

		Triangles.Add(Base);
		Triangles.Add(Base + 1);
		Triangles.Add(Base + 2);
	}

	void AddQuad(const FVector& A, const FVector& B, const FVector& C, const FVector& D)
	{
		AddTri(A, B, C);
		AddTri(A, C, D);
	}

	/**
	 * Adds a triangle wound so its normal points away from Inside, which saves
	 * having to reason about winding for every face of a convex hull.
	 */
	void AddTriFacingOut(const FVector& A, const FVector& B, const FVector& C, const FVector& Inside)
	{
		const FVector Centroid = (A + B + C) / 3.f;
		const FVector Normal = FVector::CrossProduct(B - A, C - A);

		if (FVector::DotProduct(Normal, Centroid - Inside) < 0.f)
		{
			AddTri(A, C, B);
		}
		else
		{
			AddTri(A, B, C);
		}
	}

	/** As above, but the normal is made to point along Facing. */
	void AddTriFacing(const FVector& A, const FVector& B, const FVector& C, const FVector& Facing)
	{
		const FVector Normal = FVector::CrossProduct(B - A, C - A);

		if (FVector::DotProduct(Normal, Facing) < 0.f)
		{
			AddTri(A, C, B);
		}
		else
		{
			AddTri(A, B, C);
		}
	}

	/**
	 * Extrudes a flat outline into a solid slab along Axis. Wings, fins and blades
	 * are all this shape. The outline must be convex; winding does not matter,
	 * since every face is oriented as it is added.
	 */
	void AddSlab(const TArray<FVector>& Outline, const FVector& Axis, float HalfThickness)
	{
		const int32 Count = Outline.Num();
		if (Count < 3)
		{
			return;
		}

		const FVector Direction = Axis.GetSafeNormal();
		const FVector Offset = Direction * HalfThickness;

		FVector Centre = FVector::ZeroVector;
		for (const FVector& Point : Outline)
		{
			Centre += Point;
		}
		Centre /= Count;

		// Both faces, fanned from the first point, each turned to face outward.
		for (int32 Index = 1; Index < Count - 1; ++Index)
		{
			AddTriFacing(Outline[0] + Offset, Outline[Index] + Offset, Outline[Index + 1] + Offset, Direction);
			AddTriFacing(Outline[0] - Offset, Outline[Index] - Offset, Outline[Index + 1] - Offset, -Direction);
		}

		// And the rim joining them, so the slab is closed from every angle.
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FVector& Current = Outline[Index];
			const FVector& Next = Outline[(Index + 1) % Count];

			const FVector EdgeOut = ((Current + Next) * 0.5f - Centre).GetSafeNormal();

			AddTriFacing(Current + Offset, Next + Offset, Next - Offset, EdgeOut);
			AddTriFacing(Current + Offset, Next - Offset, Current - Offset, EdgeOut);
		}
	}

	/** A capped prism along Axis, for engine pods and gun barrels. */
	void AddPrism(const FVector& Centre, const FVector& Axis, float Length, float Radius, int32 Sides)
	{
		Sides = FMath::Max(Sides, 3);

		const FVector Forward = Axis.GetSafeNormal();
		FVector Right = FVector::CrossProduct(Forward, FVector::UpVector);
		if (Right.IsNearlyZero())
		{
			Right = FVector::CrossProduct(Forward, FVector::ForwardVector);
		}
		Right = Right.GetSafeNormal();

		const FVector Up = FVector::CrossProduct(Right, Forward).GetSafeNormal();

		const FVector Front = Centre + Forward * (Length * 0.5f);
		const FVector Back = Centre - Forward * (Length * 0.5f);

		TArray<FVector> Ring;
		Ring.Reserve(Sides);

		for (int32 Index = 0; Index < Sides; ++Index)
		{
			const float Angle = (2.f * PI * Index) / Sides;
			Ring.Add((Right * FMath::Cos(Angle) + Up * FMath::Sin(Angle)) * Radius);
		}

		for (int32 Index = 0; Index < Sides; ++Index)
		{
			const FVector& A = Ring[Index];
			const FVector& B = Ring[(Index + 1) % Sides];

			AddQuad(Front + A, Front + B, Back + B, Back + A);
			AddTri(Front, Front + A, Front + B);
			AddTri(Back, Back + B, Back + A);
		}
	}

	void Commit(UProceduralMeshComponent* Mesh, int32 Section = 0) const
	{
		if (!Mesh)
		{
			return;
		}

		const TArray<FLinearColor> NoColours;

		Mesh->CreateMeshSection_LinearColor(
			Section, Vertices, Triangles, Normals, UVs, NoColours, Tangents,
			/*bCreateCollision=*/false);
	}
};
