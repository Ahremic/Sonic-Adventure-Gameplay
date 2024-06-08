#pragma once
#include "hhCMatrix.h"

namespace Hedgehog::Math
{
	void CMatrix::CreateFromAxis(CMatrix* This, CVector* xAxis, CVector* yAxis, CVector* zAxis)
	{
		BB_FUNCTION_PTR(void, __thiscall, func, 0x006F0EA0,
			CMatrix * _This,
			CVector * x, CVector * y, CVector * z);

		// UNDONE: May not be necessary, worth verifying?
		//*This = CMatrix::Identity();
		func(This, xAxis, yAxis, zAxis);
	}

	CMatrix CMatrix::CreateFromAxis(const CVector& xAxis, const CVector& yAxis, const CVector& zAxis)
	{
		BB_FUNCTION_PTR(void, __thiscall, func, 0x006F0EA0,
			CMatrix * _This,
			const CVector & x, const CVector & y, const CVector & z);

		CMatrix result = CMatrix::Identity();
		func(&result, xAxis, yAxis, zAxis);
		return result;
	}

	CMatrix* CMatrix::Transpose(void* This, void* Result)
	{
		BB_FUNCTION_PTR(void*, __thiscall, func, 0x009BECC0, void* _This, void* z);
		return (CMatrix*)func(This, Result);
	}

	CVector CMatrix::GetVectorFromRow(int row) const
	{
		BB_FUNCTION_PTR(CVector*, __thiscall, func, 0x006F1530, const CMatrix * This, const CVector & _result, int _row);

		CVector result(0, 0, 0);
		func(this, result, row);
		return result;
	}

	CVector CMatrix::GetVectorFromColumn(int column) const
	{
		BB_FUNCTION_PTR(CVector*, __thiscall, func, 0x006F14E0, const CMatrix * This, const CVector & _result, int _column);

		CVector result(0, 0, 0);
		func(this, result, column);
		return result;
	}

	CVector* CMatrix::TransformNormal(CMatrix44* This, CVector* result, CVector* vec)
	{
		BB_FUNCTION_PTR(CVector*, __thiscall, func, 0x009BE800, CMatrix44 * _This, CVector * _result, CVector * _vec);
		return func(This, result, vec);
	}

	Eigen::Matrix3f CMatrix::GetRotationMatrix()
	{
		float* data = (float*)this;
		float out[9] = { data[0], data[1], data[2],
						 data[4], data[5], data[6],
						 data[8], data[9], data[10] };

		return *(Eigen::Matrix3f*)&out;
	}

	void CMatrix44::CreateFromAxis(CMatrix44* This, const CVector* xAxis, const CVector* yAxis, const CVector* zAxis)
	{
		BB_FUNCTION_PTR(void, __thiscall, func, 0x006F0EA0,
			CMatrix44 * _This,
			const CVector * x, const CVector * y, const CVector * z);

		*This = CMatrix44::Identity();
		func(This, xAxis, yAxis, zAxis);
	}

	CMatrix44 CMatrix44::CreateFromAxis(const CVector& xAxis, const CVector& yAxis, const CVector& zAxis)
	{
		CMatrix44 mat = {};
		CreateFromAxis(&mat, &xAxis, &yAxis, &zAxis);
		return mat;
	}

	CMatrix44 CMatrix44::CreateFromAxis(CVector* xAxis, CVector* yAxis, CVector* zAxis)
	{
		CMatrix44* mat = nullptr;
		CreateFromAxis(mat, xAxis, yAxis, zAxis);
		return *mat;
	}

	void CMatrix44::SetFromAxis(CVector* xAxis, CVector* yAxis, CVector* zAxis)
	{
		CreateFromAxis(this, xAxis, yAxis, zAxis);
	}

	float CMatrix44::GetFloatFromMatrix(int row, int column)
	{
		return *(float*)(((uint32_t)this) + 4 * column + row);
	}

	CVector CMatrix44::GetVectorFromRow(int row) const
	{
		//CMatrix44* This = (CMatrix44*)this;
		//float z = This->GetFloatFromMatrix(row, 2);
		//float y = This->GetFloatFromMatrix(row, 1);
		//float x = This->GetFloatFromMatrix(row, 0);
		//return CVector(x, y, z);

		BB_FUNCTION_PTR(CVector*, __thiscall, func, 0x006F1530, const CMatrix44 * This, const CVector & result, int _row);
		CVector result(0, 0, 0);
		func(this, result, row);
		return result;
	}

	CVector CMatrix44::GetVectorFromColumn(int row) const
	{
		BB_FUNCTION_PTR(CVector*, __thiscall, func, 0x006F14E0, const CMatrix44 * This, const CVector & _result, int _column);
		CVector result(0, 0, 0);
		func(this, result, row);
		return result;
	}

	CVector CMatrix44::TransformVector(const CVector& vector) const
	{
		CVector* vectorRows = (CVector*)this;
		return vector.x() * vectorRows[0]
		     + vector.y() * vectorRows[1]
		     + vector.z() * vectorRows[2];

		//BB_FUNCTION_PTR(CVector*, __thiscall, func, 0x009BE800, const CMatrix44 * This, const CVector& result, const CVector& vec);
		//CVector result(0, 0, 0);
		//func(this, result, vector);
		//return result;
	}

	void CMatrix44::RotateYaw(float angle)
	{
		BB_FUNCTION_PTR(void, __thiscall, func, 0x009BE460, CMatrix44 * This, const float f);
		func(this, angle);
	}

	void CMatrix44::RotatePitch(float angle)
	{
		BB_FUNCTION_PTR(void, __thiscall, func, 0x009BE420, CMatrix44 * This, const float f);
		func(this, angle);
	}

}