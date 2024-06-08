#include "..\hhMath.h"

namespace Hedgehog::Math
{
//#define _VECTOR_DEBUG
#ifdef _VECTOR_DEBUG

	inline bool _IsVectorNaN(const CVector& vec)
	{
		bool result = false;

		float* dbg = (float*)&vec;

		result = isnan(dbg[0]) ? true : result;
		result = isnan(dbg[1]) ? true : result;
		result = isnan(dbg[2]) ? true : result;
		result = isnan(dbg[3]) ? true : result;

		return result;
	}
	inline bool _IsVectorNaN(const CVector* vec)
	{
		bool result = false;

		float* dbg = (float*)vec;

		result = isnan(dbg[0]) ? true : result;
		result = isnan(dbg[1]) ? true : result;
		result = isnan(dbg[2]) ? true : result;
		result = isnan(dbg[3]) ? true : result;

		return result;
	}

#define NANVEC(vec) \
	if (_IsVectorNaN(vec)) { std::stringstream stream; stream << "Vector is NAN at line: " << __LINE__ << "\n Please attach the debugger and check what caused this."; MessageBoxA(nullptr, stream.str().c_str(), "VECTOR NAN", MB_OK); }
#define NANFLOAT(flt) \
	if (isnan(flt)) { std::stringstream stream; stream << "Float is NAN at line: " << __LINE__ << "\n Please attach the debugger and check what caused this."; MessageBoxA(nullptr, stream.str().c_str(), "FLOAT NAN", MB_OK); }
#else
#define NANVEC(vec)
#define NANFLOAT(vec)
#endif

	CVector CVector::One()
	{
		return CVector(1, 1, 1);
	}

	CVector CVector::Right()
	{
		return CVector(-1, 0, 0);
	}

	CVector CVector::Left()
	{
		return CVector(1, 0, 0);
	}

	CVector CVector::Up()
	{
		return CVector(0, 1, 0);
	}

	CVector CVector::Down()
	{
		return CVector(0, -1, 0);
	}

	CVector CVector::Forward()
	{
		return CVector(0, 0, 1);
	}

	CVector CVector::Backward()
	{
		return CVector(0, 0, -1);
	}

	CVector* CVector::Lerp(CVector* out, CVector* VectorA, CVector* VectorB, float tValue)
	{
#ifdef _USE_INGAME_MATH
		NANVEC(out);
		NANVEC(VectorA);
		NANVEC(VectorB);

		BB_FUNCTION_PTR(CVector*, __thiscall, func, 0x006F0330, CVector * _out, CVector * _VectorA, CVector * _VectorB, float _tValue);
		CVector* result = func(out, VectorA, VectorB, tValue);

		NANVEC(out);
		NANVEC(VectorA);
		NANVEC(VectorB);

		return result;
#else
		*out = CVector::Lerp(*VectorA, *VectorB, tValue);
		return out;
#endif
	}

	CVector CVector::Orthogonal(const CVector& v)
	{
		const CVector out = v.z() < v.x() ? CVector(v.y(), -v.x(), 0) : CVector(0, -v.z(), v.y());

		NANVEC(out);
		NANVEC(v);

		return out;
	}

	double CVector::Length(CVector* This)
	{
		NANVEC(This);

		BB_FUNCTION_PTR(double, __thiscall, func, 0x009BF710, CVector * _This);
		double out = func(This);

		NANFLOAT(out);

		return out;
	}

	float CVector::Length(const CVector& This)
	{
		NANVEC(This);
		return This.norm();
	}

	double CVector::Length() const
	{
#ifdef _USE_INGAME_MATH
		NANVEC(this);
		BB_FUNCTION_PTR(double, __thiscall, func, 0x009BF710, const CVector * _This);
		double out = func(this);
		NANVEC(this);
		NANFLOAT(out);
		return out;
#else
		return this->norm();
#endif
	}

	// Two-point length
	float CVector::Length(const CVector& A, const CVector& B)
	{
		return (A - B).norm();
	}
	float CVector::Distance(const CVector& A, const CVector& B)
	{
		return Length(A, B);
	}

	CVector CVector::Project(const CVector& a, const CVector& b)
	{
		NANVEC(a);
		NANVEC(b);
		float m = Magnitude(b);
		NANFLOAT(m);
		return b * (Dot(a, b) / (m * m));
	}

	CVector CVector::Reject(const CVector& a, const CVector& b)
	{
		return a - Project(a, b);
	}

	CVector CVector::ProjectOnPlane(const CVector& vector, const CVector& planeNormal)
	{
		return Reject(vector, planeNormal);
	}

	CVector CVector::ProjectOnPlane(const CVector& planeNormal) const
	{
		return Reject(*this, planeNormal);
	}

	double CVector::Magnitude(const CVector& This)
	{
		return This.norm();
	}

	double CVector::Magnitude() const
	{
		return this->Length();
	}

	CVector* CVector::SetZero(CVector* This)
	{
		*This = CVector::Zero();
		return This;
		//BB_FUNCTION_PTR(CVector, __thiscall, func, 0x009BFB10, CVector* _This);
		//return func(This);
	}

	void CVector::SetZero()
	{
#ifdef _USE_INGAME_MATH
		NANVEC(this);
		BB_FUNCTION_PTR(CVector, __thiscall, func, 0x009BFB10, const CVector * _This);
		func(this);
		NANVEC(this);
#else
		*this = CVector::Zero();
#endif
	}

	void CVector::Normalize(CVector* This)
	{
		//BB_FUNCTION_PTR(void, __thiscall, func, 0x009BF970, CVector* _This);
		//func(This);
		*This = This->normalizedSafe();
	}

	CVector CVector::Normalize(const CVector& This)
	{
		//BB_FUNCTION_PTR(void, __thiscall, func, 0x009BF970, CVector* _This);
		//func(This);
		return This.normalizedSafe();
	}

	void CVector::Normalize()
	{
		//BB_FUNCTION_PTR(void, __thiscall, func, 0x009BF970, const CVector* _This);
		//func(this);
		*this = this->normalizedSafe();
	}

	CVector* CVector::Normalized(CVector* This, CVector* result)
	{
		//BB_FUNCTION_PTR(CVector*, __thiscall, func, 0x009BF7E0, CVector* _This, CVector* _result);
		//return func(This, result);
		*result = This->normalizedSafe();
		return result;
	}

	CVector* CVector::Normalized(CVector* result) const
	{
#ifdef _USE_INGAME_MATH
		NANVEC(this);
		NANVEC(result);
		BB_FUNCTION_PTR(CVector*, __thiscall, func, 0x009BF7E0, const CVector * _This, CVector * result);
		CVector* out = func(this, result);
		NANVEC(this);
		NANVEC(result);
		NANVEC(out);
		return out;
#else
		*result = this->normalizedSafe();
		return result;
#endif
	}

	CVector CVector::Normalized(const CVector& This)
	{
		return This.normalizedSafe();
	}

	CVector* CVector::Add(CVector* result, CVector* value)
	{
		*result += *value;
		return result;
	}

	CVector* CVector::Divide(CVector* result, CVector* value, float scalar)
	{
		//BB_FUNCTION_PTR(CVector*, __cdecl, func, 0x004030E0, CVector* _result, CVector* _value, float _scalar);
		//return func(result, value, scalar);

		if (scalar < FLT_EPSILON)
		{
			*result = CVector::Zero();
			return result;
		}

		NANVEC(result);
		NANVEC(value);
		NANFLOAT(scalar);

		*result = *value / scalar;

		NANVEC(result);
		NANVEC(value);

		return result;
	}

	CVector CVector::Divide(const CVector& value, float scalar)
	{
		if (scalar < FLT_EPSILON)
		{
			return Zero();
		}

		NANVEC(value);
		NANFLOAT(scalar);

		const CVector out = value / scalar;
		NANVEC(out);
		return out;
	}

	CVector* CVector::Multiply(CVector* result, CVector* value, float scalar)
	{
		//BB_FUNCTION_PTR(CVector*, __cdecl, func, 0x009BFB90, CVector* _result, CVector* _value, float _scalar);
		//return func(result, value, scalar);

		*result = *value * scalar;
		return result;
	}

	CVector* CVector::Multiply(CVector* result, float scalar, CVector* value)
	{
		//BB_FUNCTION_PTR(CVector*, __cdecl, func, 0x00404A70, CVector* _result, float _scalar, CVector* _value);
		//return func(result, scalar, value);
		*result = *value * scalar;
		return result;
	}

	CVector* CVector::Multiply(CVector* vector, float scalar)
	{
		//BB_FUNCTION_PTR(CVector*, __thiscall, func, 0x009BFB40, CVector* _vector, float _scalar);
		//return func(vector, scalar);
		*vector *= scalar;
		return vector;
	}

	double CVector::Dot(const CVector* This, const CVector* value)
	{
#if _USE_INGAME_MATH
		NANVEC(This);
		NANVEC(value);
		BB_FUNCTION_PTR(double, __thiscall, func, 0x009BF650, const CVector * _This, const CVector * _value);
		double out = func(This, value);
		NANVEC(This);
		NANVEC(value);
		NANFLOAT(out);
		return out;
#else
		return This->dot(*value);
#endif
	}

	double CVector::Dot(const CVector& This, const CVector& value)
	{
		/*
		BB_FUNCTION_PTR(double, __thiscall, func, 0x009BF650, const CVector & _This, const CVector & _value);
		double out = func(This, value);
		NANVEC(This);
		NANVEC(value);
		NANFLOAT(out);
		return out;
		*/

		return This.dot(value);
	}

	double CVector::Dot(CVector* value) const
	{
#if _USE_INGAME_MATH
		BB_FUNCTION_PTR(double, __thiscall, func, 0x009BF650, const CVector * _This, CVector * value);
		double out = func(this, value);
		NANVEC(this);
		NANVEC(value);
		NANFLOAT(out);
		return out;
#else
		return this->dot(*value);
#endif
	}

	double CVector::LengthSqr(CVector* This)
	{
#if _USE_INGAME_MATH
		BB_FUNCTION_PTR(double, __thiscall, func, 0x004030A0, CVector * _This);
		return func(This);
#else
		return This->squaredNorm();
#endif
	}
	double CVector::LengthSqr(const CVector& This)
	{
		return This.squaredNorm();
	}

	double CVector::LengthSqr() const
	{
#if _USE_INGAME_MATH
		BB_FUNCTION_PTR(double, __thiscall, func, 0x004030A0, const CVector * _This);
		return func(this);
#else
		return this->squaredNorm();
#endif
	}

	double CVector::SqrMagnitude(const CVector& This)
	{
		return This.squaredNorm();
	}

	double CVector::SqrMagnitude() const
	{
		return this->squaredNorm();
	}

	CVector* CVector::Cross(const CVector* This, CVector* result, const CVector* value)
	{
#ifdef _USE_INGAME_MATH
		NANVEC(This);
		NANVEC(result);
		NANVEC(value);
		BB_FUNCTION_PTR(CVector*, __thiscall, func, 0x009BF550, const CVector * _This, CVector * _result, const CVector * _value);
		CVector* out = func(This, result, value);
		NANVEC(This);
		NANVEC(result);
		NANVEC(value);
		NANVEC(out);
		return out;
#else
		CVector a = *This;
		CVector b = *value;
		*result = Cross(a, b);
		return result;
#endif
	}

	CVector CVector::Cross(const CVector& lhs, const CVector& rhs)
	{
		float x = lhs.y() * rhs.z() - lhs.z() * rhs.y();
		float y = lhs.z() * rhs.x() - lhs.x() * rhs.z();
		float z = lhs.x() * rhs.y() - lhs.y() * rhs.x();
		return CVector(x, y, z);
	}

	CVector CVector::Cross(const CVector& b)
	{
#ifdef _USE_INGAME_MATH
		NANVEC(this);
		NANVEC(b);
		BB_FUNCTION_PTR(CVector*, __thiscall, func, 0x009BF550, const CVector * _This, CVector * _result, CVector const* _value);
		CVector junk;
		const CVector out = *func(this, &junk, &b);

		NANVEC(junk);
		NANVEC(this);
		NANVEC(b);
		NANVEC(out);
		return out;
#else
		return CVector::Cross(*this, b);
#endif
	}

	CVector CVector::SlerpUnclamped(const CVector& a, const CVector& b, const float t)
	{
		float magA = Magnitude(a);
		float magB = Magnitude(b);

		// HACK: Fix potential NAN stuff.
		if (magA < FLT_EPSILON)
			return b;
		if (magB < FLT_EPSILON)
			return a;

		CVector _a = a / magA;
		CVector _b = b / magB;
		float dot = Dot(_a, _b);
		dot = fmaxf(dot, -1.0);
		dot = fminf(dot, 1.0);
		float theta = acosf(dot) * t;
		CVector relativeVec = Normalized(_b - _a * dot);
		CVector newVec = _a * cosf(theta) + relativeVec * sinf(theta);
		return newVec * (magA + (magB - magA) * t);
	}

	CVector CVector::Slerp(const CVector& a, const CVector& b, const float t)
	{
		if (t < 0)
			return a;
		else
			if (t > 1)
				return b;
		return SlerpUnclamped(a, b, t);
	}

	float CVector::Angle(const CVector& a, const CVector& b)
	{
		float v = Dot(a, b) / (Magnitude(a) * Magnitude(b));
		v = fmaxf(v, -1.0);
		v = fminf(v, 1.0);
		return acosf(v);
	}

	float CVector::SignedAngle(const CVector& a, const CVector& b, const CVector& axis)
	{
		auto cross = CVector::Cross(a, b);
		auto x = CVector::Dot(axis, cross);
		auto y = CVector::Dot(a, b);

		return atan2(x, y);
	}

	CVector CVector::RotateTowards(const CVector& from, const CVector& to, float maxRadiansDelta)
	{
		float angle = CVector::Angle(from, to);
		if (angle == 0)
			return to;
		maxRadiansDelta = fmaxf(maxRadiansDelta, angle - (float)M_PI);
		float t = fminf(1, maxRadiansDelta / angle);
		return CVector::SlerpUnclamped(from, to, t);
	}

	CVector CVector::LerpTowards(const CVector& from, const CVector& to, float maxRadiansDelta)
	{
		float angle = CVector::Angle(from, to);
		if (angle == 0)
			return to;
		maxRadiansDelta = fmaxf(maxRadiansDelta, angle - (float)M_PI);
		float t = fminf(1, maxRadiansDelta / angle);
		return CVector::LerpUnclamped(from, to, t);
	}

	CVector CVector::SlerpTowards(const CVector& from, const CVector& to, float maxRadiansDelta)
	{
		float angle = CVector::Angle(from, to);
		if (angle == 0)
			return to;
		maxRadiansDelta = fmaxf(maxRadiansDelta, angle - (float)M_PI);
		float t = fminf(1, maxRadiansDelta / angle);
		return CVector::LerpUnclamped(from, to, t);
	}

	CVector CVector::Scale(const CVector& a, const CVector& b)
	{
		return CVector(a.x() * b.x(), a.y() * b.y(), a.z() * b.z());
	}

	CVector CVector::Scale(const CVector& b) const
	{
		return CVector(this->x() * b.x(), this->y() * b.y(), this->z() * b.z());
	}

	CVector CVector::normalizedSafe() const
	{
		CVector This = *this;
		CVector result = This.LengthSqr() > (DBL_EPSILON + DBL_EPSILON + DBL_EPSILON)
					   ? This.normalized()
					   : CVector(0, 0, 0);
		return result;
	}

	CVector CVector::LerpUnclamped(const CVector& a, const CVector& b, const float t)
	{
		CVector result = (b - a) * t + a;
		return result;
	}

	CVector CVector::Lerp(const CVector& a, const CVector& b, const float t)
	{
		if (t < 0)
			return a;
		if (t > 1)
			return b;
		return LerpUnclamped(a, b, t);
	}

	CVector CVector::Reflect(const CVector& vector, const CVector& planeNormal)
	{
		return vector - Project(vector, planeNormal) * 2.0f;
	}


	CVector CVector::Reflect(const CVector& planeNormal) const
	{
		return Reflect(*this, planeNormal);
	}

}
