// Copyright EnigmaEngine. All Rights Reserved.

/// @file Matrix.cpp
/// @brief Implementation of FMatrix non-constexpr functions.

#include "Math/Matrix.h"
#include "Math/Quat.h"

namespace
{

/// @brief Compute the determinant of a 3x3 matrix.
float Det3x3(
	float A00, float A01, float A02,
	float A10, float A11, float A12,
	float A20, float A21, float A22)
{
	return A00 * (A11 * A22 - A12 * A21)
		 - A01 * (A10 * A22 - A12 * A20)
		 + A02 * (A10 * A21 - A11 * A20);
}

} // anonymous namespace

namespace Enigma
{

// -----------------------------------------------------------------
// Constants
// -----------------------------------------------------------------

const FMatrix FMatrix::Identity(
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f);

// -----------------------------------------------------------------
// Constructors
// -----------------------------------------------------------------

FMatrix::FMatrix()
{
	// Intentionally uninitialized for performance.
}

// -----------------------------------------------------------------
// Operators
// -----------------------------------------------------------------

FMatrix FMatrix::operator*(const FMatrix& B) const
{
	FMatrix Result;
	for (int Row = 0; Row < 4; ++Row)
	{
		for (int Col = 0; Col < 4; ++Col)
		{
			Result.M[Row][Col] =
				M[Row][0] * B.M[0][Col] +
				M[Row][1] * B.M[1][Col] +
				M[Row][2] * B.M[2][Col] +
				M[Row][3] * B.M[3][Col];
		}
	}
	return Result;
}

// -----------------------------------------------------------------
// Transform operations
// -----------------------------------------------------------------

FVector FMatrix::TransformPosition(const FVector& V) const
{
	// Row-vector convention: [X, Y, Z, 1] * M
	return FVector(
		V.X * M[0][0] + V.Y * M[1][0] + V.Z * M[2][0] + M[3][0],
		V.X * M[0][1] + V.Y * M[1][1] + V.Z * M[2][1] + M[3][1],
		V.X * M[0][2] + V.Y * M[1][2] + V.Z * M[2][2] + M[3][2]);
}

FVector FMatrix::TransformVector(const FVector& V) const
{
	// Row-vector convention: [X, Y, Z, 0] * M (no translation)
	return FVector(
		V.X * M[0][0] + V.Y * M[1][0] + V.Z * M[2][0],
		V.X * M[0][1] + V.Y * M[1][1] + V.Z * M[2][1],
		V.X * M[0][2] + V.Y * M[1][2] + V.Z * M[2][2]);
}

// -----------------------------------------------------------------
// Operations
// -----------------------------------------------------------------

FMatrix FMatrix::GetTransposed() const
{
	return FMatrix(
		M[0][0], M[1][0], M[2][0], M[3][0],
		M[0][1], M[1][1], M[2][1], M[3][1],
		M[0][2], M[1][2], M[2][2], M[3][2],
		M[0][3], M[1][3], M[2][3], M[3][3]);
}

float FMatrix::Determinant() const
{
	return M[0][0] * Det3x3(M[1][1], M[1][2], M[1][3], M[2][1], M[2][2], M[2][3], M[3][1], M[3][2], M[3][3])
		 - M[0][1] * Det3x3(M[1][0], M[1][2], M[1][3], M[2][0], M[2][2], M[2][3], M[3][0], M[3][2], M[3][3])
		 + M[0][2] * Det3x3(M[1][0], M[1][1], M[1][3], M[2][0], M[2][1], M[2][3], M[3][0], M[3][1], M[3][3])
		 - M[0][3] * Det3x3(M[1][0], M[1][1], M[1][2], M[2][0], M[2][1], M[2][2], M[3][0], M[3][1], M[3][2]);
}

FMatrix FMatrix::GetInverse() const
{
	const float Det = Determinant();

	if (FMath::Abs(Det) < FMath::SmallNumber)
	{
		// Singular matrix -- return Identity.
		// TODO: Log warning when logging system is available.
		return Identity;
	}

	const float InvDet = 1.0f / Det;

	// Adjugate method: Inverse = Adj(M) / det(M)
	// Adj[i][j] = (-1)^(i+j) * Minor(j, i)
	FMatrix Result;

	Result.M[0][0] =  Det3x3(M[1][1],M[1][2],M[1][3], M[2][1],M[2][2],M[2][3], M[3][1],M[3][2],M[3][3]) * InvDet;
	Result.M[0][1] = -Det3x3(M[0][1],M[0][2],M[0][3], M[2][1],M[2][2],M[2][3], M[3][1],M[3][2],M[3][3]) * InvDet;
	Result.M[0][2] =  Det3x3(M[0][1],M[0][2],M[0][3], M[1][1],M[1][2],M[1][3], M[3][1],M[3][2],M[3][3]) * InvDet;
	Result.M[0][3] = -Det3x3(M[0][1],M[0][2],M[0][3], M[1][1],M[1][2],M[1][3], M[2][1],M[2][2],M[2][3]) * InvDet;

	Result.M[1][0] = -Det3x3(M[1][0],M[1][2],M[1][3], M[2][0],M[2][2],M[2][3], M[3][0],M[3][2],M[3][3]) * InvDet;
	Result.M[1][1] =  Det3x3(M[0][0],M[0][2],M[0][3], M[2][0],M[2][2],M[2][3], M[3][0],M[3][2],M[3][3]) * InvDet;
	Result.M[1][2] = -Det3x3(M[0][0],M[0][2],M[0][3], M[1][0],M[1][2],M[1][3], M[3][0],M[3][2],M[3][3]) * InvDet;
	Result.M[1][3] =  Det3x3(M[0][0],M[0][2],M[0][3], M[1][0],M[1][2],M[1][3], M[2][0],M[2][2],M[2][3]) * InvDet;

	Result.M[2][0] =  Det3x3(M[1][0],M[1][1],M[1][3], M[2][0],M[2][1],M[2][3], M[3][0],M[3][1],M[3][3]) * InvDet;
	Result.M[2][1] = -Det3x3(M[0][0],M[0][1],M[0][3], M[2][0],M[2][1],M[2][3], M[3][0],M[3][1],M[3][3]) * InvDet;
	Result.M[2][2] =  Det3x3(M[0][0],M[0][1],M[0][3], M[1][0],M[1][1],M[1][3], M[3][0],M[3][1],M[3][3]) * InvDet;
	Result.M[2][3] = -Det3x3(M[0][0],M[0][1],M[0][3], M[1][0],M[1][1],M[1][3], M[2][0],M[2][1],M[2][3]) * InvDet;

	Result.M[3][0] = -Det3x3(M[1][0],M[1][1],M[1][2], M[2][0],M[2][1],M[2][2], M[3][0],M[3][1],M[3][2]) * InvDet;
	Result.M[3][1] =  Det3x3(M[0][0],M[0][1],M[0][2], M[2][0],M[2][1],M[2][2], M[3][0],M[3][1],M[3][2]) * InvDet;
	Result.M[3][2] = -Det3x3(M[0][0],M[0][1],M[0][2], M[1][0],M[1][1],M[1][2], M[3][0],M[3][1],M[3][2]) * InvDet;
	Result.M[3][3] =  Det3x3(M[0][0],M[0][1],M[0][2], M[1][0],M[1][1],M[1][2], M[2][0],M[2][1],M[2][2]) * InvDet;

	return Result;
}

bool FMatrix::Equals(const FMatrix& Other, float Tolerance) const
{
	for (int Row = 0; Row < 4; ++Row)
	{
		for (int Col = 0; Col < 4; ++Col)
		{
			if (FMath::Abs(M[Row][Col] - Other.M[Row][Col]) > Tolerance)
			{
				return false;
			}
		}
	}
	return true;
}

// -----------------------------------------------------------------
// Factory methods
// -----------------------------------------------------------------

FMatrix FMatrix::MakeTranslation(const FVector& T)
{
	return FMatrix(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		T.X,  T.Y,  T.Z,  1.0f);
}

FMatrix FMatrix::MakeRotation(const FQuat& Q)
{
	const float X2 = Q.X + Q.X;
	const float Y2 = Q.Y + Q.Y;
	const float Z2 = Q.Z + Q.Z;

	const float XX = Q.X * X2;
	const float XY = Q.X * Y2;
	const float XZ = Q.X * Z2;
	const float YY = Q.Y * Y2;
	const float YZ = Q.Y * Z2;
	const float ZZ = Q.Z * Z2;
	const float WX = Q.W * X2;
	const float WY = Q.W * Y2;
	const float WZ = Q.W * Z2;

	// Row-vector convention: transpose of the column-vector rotation matrix.
	return FMatrix(
		1.0f - (YY + ZZ),  XY + WZ,            XZ - WY,            0.0f,
		XY - WZ,            1.0f - (XX + ZZ),   YZ + WX,            0.0f,
		XZ + WY,            YZ - WX,            1.0f - (XX + YY),   0.0f,
		0.0f,               0.0f,               0.0f,               1.0f);
}

FMatrix FMatrix::MakeScale(const FVector& S)
{
	return FMatrix(
		S.X,  0.0f, 0.0f, 0.0f,
		0.0f, S.Y,  0.0f, 0.0f,
		0.0f, 0.0f, S.Z,  0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
}

FMatrix FMatrix::MakeScale(float S)
{
	return FMatrix(
		S,    0.0f, 0.0f, 0.0f,
		0.0f, S,    0.0f, 0.0f,
		0.0f, 0.0f, S,    0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
}

} // namespace Enigma
