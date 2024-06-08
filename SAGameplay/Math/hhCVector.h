#pragma once
namespace Hedgehog::Math
{
	class CVector : public Eigen::AlignedVector3<float>
	{
	public:
		using AlignedVector3<float>::AlignedVector3;

		static CVector One();
		static CVector Right();
		static CVector Left();
		static CVector Up();
		static CVector Down();
		static CVector Forward();
		static CVector Backward();

		static CVector LerpUnclamped(const CVector& a, const CVector& b, const float t);

		static CVector Lerp(const CVector& a, const CVector& b, const float t);

		static CVector* Lerp(CVector* out, CVector* VectorA, CVector* VectorB, float tValue);

		static CVector Orthogonal(const CVector& v);

		static double Length(CVector* This);

		static float Length(const CVector& This);

		static float Length(const CVector& A, const CVector& B);

		static float Distance(const CVector& A, const CVector& B);

		double Length() const;

		static CVector Project(const CVector& a, const CVector& b);

		static CVector Reject(const CVector& a, const CVector& b);

		/**
	 * Returns a vector reflected off the plane orthogonal to the normal.
	 * The input vector is pointed inward, at the plane, and the return vector
	 * is pointed outward from the plane, like a beam of light hitting and then
	 * reflecting off a mirror.
	 * @param vector: The vector traveling inward at the plane.
	 * @param planeNormal: The normal of the plane off of which to reflect.
	 * @return: A new vector pointing outward from the plane.
	 */
		static CVector Reflect(const CVector& vector, const CVector& planeNormal);
		CVector Reflect(const CVector& planeNormal) const;

		static CVector ProjectOnPlane(const CVector& vector, const CVector& planeNormal);

		CVector ProjectOnPlane(const CVector& planeNormal) const;

		static double Magnitude(const CVector& This);

		double Magnitude() const;

		static CVector* SetZero(CVector* This);

		void SetZero();

		static void Normalize(CVector* This);

		static CVector Normalize(const CVector& This);

		void Normalize();

		static CVector* Normalized(CVector* This, CVector* result);

		CVector* Normalized(CVector* result) const;

		static CVector Normalized(const CVector& This);

		static CVector* Add(CVector* result, CVector* value);

		static CVector* Divide(CVector* result, CVector* value, float scalar);

		static CVector Divide(const CVector& value, float scalar);

		static CVector* Multiply(CVector* result, CVector* value, float scalar);

		static CVector* Multiply(CVector* result, float scalar, CVector* value);

		static CVector* Multiply(CVector* vector, float scalar);

		static double Dot(const CVector* This, const CVector* value);

		static double Dot(const CVector& This, const CVector& value);

		double Dot(CVector* value) const;

		static double LengthSqr(CVector* This);

		static double LengthSqr(const CVector& This);

		double LengthSqr() const;

		static double SqrMagnitude(const CVector& This);

		double SqrMagnitude() const;

		static CVector* Cross(const CVector* This, CVector* result, const CVector* value);

		static CVector Cross(const CVector& lhs, const CVector& rhs);

		CVector Cross(const CVector& b);

		static CVector SlerpUnclamped(const CVector& a, const CVector& b, const float t);

		static CVector Slerp(const CVector& a, const CVector& b, const float t);

		static float Angle(const CVector& a, const CVector& b);

		static float SignedAngle(const CVector& a, const CVector& b, const CVector& axis);

		static CVector RotateTowards(const CVector& from, const CVector& to,
			float maxRadiansDelta);

		static CVector LerpTowards(const CVector& from, const CVector& to,
			float maxRadiansDelta);

		static CVector SlerpTowards(const CVector& from, const CVector& to,
			float maxRadiansDelta);

		static CVector Scale(const CVector& a, const CVector& b);

		CVector Scale(const CVector& b) const;

		CVector normalizedSafe() const;
	};
}