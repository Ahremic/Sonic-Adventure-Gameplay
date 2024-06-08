#include "hhCVector.h"
#include "hhCVector4.h"
#include "hhCMatrix.h"

namespace Hedgehog::Math
{

	CQuaternion CQuaternion::operator/(const float& rhs) const
	{
		CVector4 This = *(CVector4*)this;
		This /= rhs;
		return *(CQuaternion*)&This;
	}

	CQuaternion CQuaternion::FromAxes(const CVector& a1, const CVector& a2, const CVector& a3)
	{
		CQuaternion This = CQuaternion::Identity();

		const CVector AxisX = a1.normalizedSafe();
		const CVector AxisY = a2.normalizedSafe();
		CVector AxisZ = CVector::Cross(AxisX, AxisY);
		const float axisZ_length = (float)AxisZ.Length();

		AxisZ = AxisZ.normalizedSafe();
		if (fabs(axisZ_length) < 0.000199999994947575)
			AxisZ = a3.normalizedSafe();

		const float dot = std::clamp((float)CVector::Dot(AxisX, AxisY), -1.0f, 1.0f);
		const double halfDot = dot * 0.5;
		const float axisScale = sqrtf(0.5 - halfDot);

		This.x() = AxisZ.x() * axisScale;
		This.y() = AxisZ.y() * axisScale;
		This.z() = AxisZ.z() * axisScale;
		This.w() = sqrtf(0.5 + halfDot);
		return This;

		//BB_FUNCTION_PTR(CQuaternion*, __cdecl, func, 0x006F1950, CQuaternion* _This, const CVector& A1, const CVector& A2, const CVector& A3);
		//func(&This, a1, a2, a3);
		//return This;
	}

	CQuaternion* CQuaternion::FromAxes(CQuaternion* out, CVector* axisX, CVector* axisY, CVector* axisZ)
	{
		const CVector AxisX = axisX->normalizedSafe();
		const CVector AxisY = axisY->normalizedSafe();
		CVector AxisZ = CVector::Cross(AxisX, AxisY);
		const float axisZ_length = (float)AxisZ.Length();

		AxisZ = AxisZ.normalizedSafe();
		if (fabs(axisZ_length) < 0.000199999994947575)
			AxisZ = axisZ->normalizedSafe();

		const float dot = std::clamp((float)CVector::Dot(AxisX, AxisY), -1.0f, 1.0f);
		const double halfDot = dot * 0.5;
		const float axisScale = sqrtf(0.5 - halfDot);

		out->x() = AxisZ.x() * axisScale;
		out->y() = AxisZ.y() * axisScale;
		out->z() = AxisZ.z() * axisScale;
		out->w() = sqrtf(0.5 + halfDot);
		return out;
	}

	CVector CQuaternion::Up() const
	{
		return *this * CVector::Up();
	}

	CVector CQuaternion::Down() const
	{
		return *this * CVector::Down();
	}

	CVector CQuaternion::Left() const
	{
		return *this * CVector::Left();
	}

	CVector CQuaternion::Right() const
	{
		return *this * CVector::Right();
	}

	CVector CQuaternion::Forward() const
	{
		return *this * CVector::Forward();
	}

	CVector CQuaternion::Back() const
	{
		return *this * CVector::Backward();
	}

	CMatrix44 CQuaternion::ToRotationMatrix() const
	{
		CMatrix44 matrix = CMatrix44::Identity();
		BB_FUNCTION_PTR(void*, __cdecl, func, 0x009BEF20, const CMatrix44 & mat, const CQuaternion * quat);
		func(matrix, this);
		return matrix;
	}

	float CQuaternion::Angle(const CQuaternion& a, const CQuaternion& b)
	{
		float dot = Dot(a, b);
		return acosf(fminf(fabs(dot), 1)) * 2;
	}

	CQuaternion CQuaternion::Conjugate(const CQuaternion& rotation)
	{
		return CQuaternion(-rotation.x(), -rotation.y(), -rotation.z(), rotation.w());
	}

	float CQuaternion::Dot(const CQuaternion& lhs, const CQuaternion& rhs)
	{
		return lhs.x() * rhs.x() + lhs.y() * rhs.y() + lhs.z() * rhs.z() + lhs.w() * rhs.w();
	}

	CQuaternion CQuaternion::FromAngleAxis(float angle, const CVector& axis)
	{
		float m = sqrt(axis.x() * axis.x() + axis.y() * axis.y() + axis.z() * axis.z());
		float s = sinf(angle / 2.0f) / m;
		const CQuaternion q(
			cosf(angle / 2.0f),
			axis.x() * s,
			axis.y() * s,
			axis.z() * s
		);
		return q;
	}

	CQuaternion CQuaternion::FromEuler(const CVector& rotation)
	{
		return FromEuler(rotation.x(), rotation.y(), rotation.z());
	}

	CQuaternion CQuaternion::FromEuler(float x, float y, float z)
	{
		float cx = cosf(x * 0.5f);
		float cy = cosf(y * 0.5f);
		float cz = cosf(z * 0.5f);
		float sx = sinf(x * 0.5f);
		float sy = sinf(y * 0.5f);
		float sz = sinf(z * 0.5f);
		CQuaternion q;
		q.x() = cx * sy * sz + cy * cz * sx;
		q.y() = cx * cz * sy - cy * sx * sz;
		q.z() = cx * cy * sz - cz * sx * sy;
		q.w() = sx * sy * sz + cx * cy * cz;
		return q;
	}

	CQuaternion CQuaternion::FromToRotation(const CVector& fromVector, const CVector& toVector)
	{
		float dot = CVector::Dot(fromVector, toVector);
		float k = sqrt(CVector::SqrMagnitude(fromVector) *
			CVector::SqrMagnitude(toVector));
		if (fabs(dot / k + 1) < 0.00001)
		{
			CVector ortho = CVector::Orthogonal(fromVector);
			auto vNorm = CVector::Normalized(ortho);
			return CQuaternion(vNorm.x(), vNorm.y(), vNorm.z(), 0);
		}
		CVector cross = CVector::Cross(fromVector, toVector);
		return Normalized(CQuaternion(cross.x(), cross.y(), cross.z(), dot + k));
	}

	CQuaternion CQuaternion::Inverse(const CQuaternion& rotation)
	{
		//float n = Norm(rotation);
		//return Conjugate(rotation) / (n * n);
		return rotation.inverse();
	}

	CQuaternion CQuaternion::Inverse() const
	{
		//float n = Norm(*this);
		//return Conjugate(*this) / (n * n);
		return this->inverse();
	}

	CQuaternion CQuaternion::Lerp(const CQuaternion& a, const CQuaternion& b, float t, bool normalize)
	{
		if (t < 0)
			return Normalized(a);
		else
			if (t > 1)
				return Normalized(b);
		return LerpUnclamped(a, b, t, normalize);
	}

	CQuaternion CQuaternion::LerpUnclamped(const CQuaternion& a, const CQuaternion& b, float t, bool normalize)
	{
		CVector4 tempVec;
		if (Dot(a, b) >= 0)
			tempVec = (*(CVector4*)&a * (1 - t)) + (*(CVector4*)&b * t);
		else
			tempVec = (*(CVector4*)&a * (1 - t)) - (*(CVector4*)&b * t);
		if (!normalize)
			return *(CQuaternion*)&tempVec;
		return Normalized(*(CQuaternion*)&tempVec);
		//return *(CQuaternion*)&tempVec;
	}

	CQuaternion CQuaternion::LookRotation(const CVector& forward)
	{
		return LookRotation(forward, CVector(0, 1, 0));
	}

	CQuaternion CQuaternion::LookRotation(const CVector& forward_, const CVector& upwards_)
	{
		// Normalize inputs
		CVector forward = CVector::Normalized(forward_);
		CVector upwards = CVector::Normalized(upwards_);
		// Don't allow zero vectors
		if (CVector::SqrMagnitude(forward) < FLT_EPSILON || CVector::SqrMagnitude(upwards) < FLT_EPSILON)
			return CQuaternion::Identity();
		// Handle alignment with up direction
		if (1 - fabs(CVector::Dot(forward, upwards)) < FLT_EPSILON)
			return FromToRotation(CVector::Forward(), forward);
		// Get orthogonal vectors
		CVector right = CVector::Normalized(CVector::Cross(upwards, forward));
		upwards = CVector::Cross(forward, right);
		// Calculate rotation
		CQuaternion CQuaternion;
		float radicand = right.x() + upwards.y() + forward.z();
		if (radicand > 0)
		{
			CQuaternion.w() = sqrt(1.0f + radicand) * 0.5f;
			float recip = 1.0f / (4.0f * CQuaternion.w());
			CQuaternion.x() = (upwards.z() - forward.y()) * recip;
			CQuaternion.y() = (forward.x() - right.z()) * recip;
			CQuaternion.z() = (right.y() - upwards.x()) * recip;
		}
		else if (right.x() >= upwards.y() && right.x() >= forward.z())
		{
			CQuaternion.x() = sqrt(1.0f + right.x() - upwards.y() - forward.z()) * 0.5f;
			float recip = 1.0f / (4.0f * CQuaternion.x());
			CQuaternion.w() = (upwards.z() - forward.y()) * recip;
			CQuaternion.z() = (forward.x() + right.z()) * recip;
			CQuaternion.y() = (right.y() + upwards.x()) * recip;
		}
		else if (upwards.y() > forward.z())
		{
			CQuaternion.y() = sqrt(1.0f - right.x() + upwards.y() - forward.z()) * 0.5f;
			float recip = 1.0f / (4.0f * CQuaternion.y());
			CQuaternion.z() = (upwards.z() + forward.y()) * recip;
			CQuaternion.w() = (forward.x() - right.z()) * recip;
			CQuaternion.x() = (right.y() + upwards.x()) * recip;
		}
		else
		{
			CQuaternion.z() = sqrt(1.0f - right.x() - upwards.y() + forward.z()) * 0.5f;
			float recip = 1.0f / (4.0f * CQuaternion.z());
			CQuaternion.y() = (upwards.z() + forward.y()) * recip;
			CQuaternion.x() = (forward.x() + right.z()) * recip;
			CQuaternion.w() = (right.y() - upwards.x()) * recip;
		}
		return CQuaternion;
	}

	float CQuaternion::Norm(const CQuaternion& rotation)
	{
		return sqrt(rotation.x() * rotation.x() +
			rotation.y() * rotation.y() +
			rotation.z() * rotation.z() +
			rotation.w() * rotation.w());
	}

	CQuaternion CQuaternion::Normalized(const CQuaternion& rotation)
	{
		return rotation / Norm(rotation);
	}

	CQuaternion CQuaternion::Normalized() const
	{
		auto rotation = *this;
		return rotation / Norm(rotation);
	}

	CQuaternion CQuaternion::RotateTowards(const CQuaternion& from, const CQuaternion& to,
		float maxRadiansDelta)
	{
		float angle = CQuaternion::Angle(from, to);
		if (angle == 0)
			return to;
		const float radiansDelta = fmaxf(maxRadiansDelta, angle - (float)M_PI);
		float t = fminf(1, radiansDelta / angle);
		return CQuaternion::SlerpUnclamped(from, to, t);
	}

	CQuaternion CQuaternion::Slerp(const CQuaternion& a, const CQuaternion& b, float t, bool normalize)
	{
		if (t < 0)
			return Normalized(a);
		else
			if (t > 1)
				return Normalized(b);
		return SlerpUnclamped(a, b, t, normalize);
	}

	CQuaternion CQuaternion::SlerpUnclamped(const CQuaternion& a, const CQuaternion& b, float t, bool normalize)
	{
		float n1;
		float n2;
		float n3 = Dot(a, b);
		bool flag = false;
		if (n3 < 0)
		{
			flag = true;
			n3 = -n3;
		}
		if (n3 > 0.999999)
		{
			n2 = 1 - t;
			n1 = flag ? -t : t;
		}
		else
		{
			float n4 = acosf(n3);
			float n5 = 1 / sinf(n4);
			n2 = sinf((1 - t) * n4) * n5;
			n1 = flag ? -sinf(t * n4) * n5 : sinf(t * n4) * n5;
		}
		CQuaternion CQuaternion;
		CQuaternion.x() = (n2 * a.x()) + (n1 * b.x());
		CQuaternion.y() = (n2 * a.y()) + (n1 * b.y());
		CQuaternion.z() = (n2 * a.z()) + (n1 * b.z());
		CQuaternion.w() = (n2 * a.w()) + (n1 * b.w());
		if (normalize)
			return Normalized(CQuaternion);
		return CQuaternion;
	}

	CQuaternion CQuaternion::SlerpTowards(const CQuaternion& from, const CQuaternion& to, float maxRadiansDelta)
	{
		float angle = Angle(from, to);
		if (angle == 0)
			return to;
		maxRadiansDelta = fmaxf(maxRadiansDelta, angle - (float)M_PI);
		float t = fminf(1, maxRadiansDelta / angle);
		return Slerp(from, to, t);
	}

	void CQuaternion::ToAngleAxis(const CQuaternion& rotation, float& angle, CVector& axis)
	{
		CQuaternion rot = rotation.w() > 1 ? Normalized(rotation) : rotation;
		//if (rot.w() > 1)
		//    rot = Normalized(rot);
		angle = 2 * acosf(rot.w());
		float s = sqrt(1 - rot.w() * rot.w());
		if (s < 0.00001)
		{
			axis.x() = 1;
			axis.y() = 0;
			axis.z() = 0;
		}
		else
		{
			axis.x() = rot.x() / s;
			axis.y() = rot.y() / s;
			axis.z() = rot.z() / s;
		}
	}

	CVector CQuaternion::ToEuler(const CQuaternion& rotation)
	{
		float sqw = rotation.w() * rotation.w();
		float sqx = rotation.x() * rotation.x();
		float sqy = rotation.y() * rotation.y();
		float sqz = rotation.z() * rotation.z();
		// If normalized is one, otherwise is correction factor
		float unit = sqx + sqy + sqz + sqw;
		float test = rotation.x() * rotation.w() - rotation.y() * rotation.z();
		CVector v;
		// sinfgularity at north pole
		if (test > 0.4995f * unit)
		{
			v.y() = 2 * atan2f(rotation.y(), rotation.x());
			v.x() = (float)M_PI_2;
			v.z() = 0;
			return v;
		}
		// sinfgularity at south pole
		if (test < -0.4995f * unit)
		{
			v.y() = -2 * atan2f(rotation.y(), rotation.x());
			v.x() = -(float)M_PI_2;
			v.z() = 0;
			return v;
		}
		// Yaw
		v.y() = atan2f(2 * rotation.w() * rotation.y() + 2 * rotation.z() * rotation.x(),
			1 - 2 * (rotation.x() * rotation.x() + rotation.y() * rotation.y()));
		// Pitch
		v.x() = asinf(2 * (rotation.w() * rotation.x() - rotation.y() * rotation.z()));
		// Roll
		v.z() = atan2f(2 * rotation.w() * rotation.z() + 2 * rotation.x() * rotation.y(),
			1 - 2 * (rotation.z() * rotation.z() + rotation.x() * rotation.x()));
		return v;
	}

}
