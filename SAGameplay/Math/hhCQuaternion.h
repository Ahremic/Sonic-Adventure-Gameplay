#pragma once
#include "hhCMatrix.h"

namespace Hedgehog::Math
{
	class CQuaternion : public Eigen::Quaternionf
	{
	public:
		using Eigen::Quaternionf::Quaternionf;

		//Quaternion::Quaternion(Vector3 vector, float scalar) : X(vector.x()),
		//    Y(vector.y()), Z(vector.z()), W(scalar) {}

		//struct CQuaternion& operator/=(const float rhs)
		//{
		//    *(CVector4*)this /= rhs;
		//    return *this;
		//}
		CQuaternion operator/(const float& rhs) const;
		//CQuaternion operator*(const float& rhs) const
		//{
		//    return *this;
		//}

		/**
		 * The following let you quickly get a direction vector from a quat.
		 */
		CVector Up() const;
		CVector Down() const;
		CVector Left() const;
		CVector Right() const;
		CVector Forward() const;
		CVector Back() const;

		/**
		 * Returns the angle between two quaternions.
		 * The quaternions must be normalized.
		 * @param a: The first quaternion.
		 * @param b: The second quaternion.
		 * @return: A scalar value.
		 */
		static float Angle(const CQuaternion& a, const CQuaternion& b);

		/**
		 * Returns the conjugate of a quaternion.
		 * @param rotation: The quaternion in question.
		 * @return: A new quaternion.
		 */
		static CQuaternion Conjugate(const CQuaternion& rotation);

		/**
		 * Returns the dot product of two quaternions.
		 * @param lhs: The left side of the multiplication.
		 * @param rhs: The right side of the multiplication.
		 * @return: A scalar value.
		 */
		static float Dot(const CQuaternion& lhs, const CQuaternion& rhs);

		/**
		 * Creates a new quaternion from the angle-axis representation of
		 * a rotation.
		 * @param angle: The rotation angle in radians.
		 * @param axis: The vector about which the rotation occurs.
		 * @return: A new quaternion.
		 */
		static CQuaternion FromAngleAxis(float angle, const CVector& axis);

		/**
		 * Create a new quaternion from the euler angle representation of
		 * a rotation. The z, x and y values represent rotations about those
		 * axis in that respective order.
		 * @param rotation: The x, y and z rotations.
		 * @return: A new quaternion.
		 */
		static CQuaternion FromEuler(const CVector& rotation);

		/**
		 * Create a new quaternion from the euler angle representation of
		 * a rotation. The z, x and y values represent rotations about those
		 * axis in that respective order.
		 * @param x: The rotation about the x-axis in radians.
		 * @param y: The rotation about the y-axis in radians.
		 * @param z: The rotation about the z-axis in radians.
		 * @return: A new quaternion.
		 */
		static CQuaternion FromEuler(float x, float y, float z);

		/**
		 * Create a quaternion rotation which rotates "fromVector" to "toVector".
		 * @param fromVector: The vector from which to start the rotation.
		 * @param toVector: The vector at which to end the rotation.
		 * @return: A new quaternion.
		 */
		static CQuaternion FromToRotation(const CVector& fromVector, const CVector& toVector);

		/**
		 * Returns the inverse of a rotation.
		 * @param rotation: The quaternion in question.
		 * @return: A new quaternion.
		 */
		static CQuaternion Inverse(const CQuaternion& rotation);
		CQuaternion Inverse() const;

		/**
		 * Interpolates between a and b by t, which is clamped to the range [0-1].
		 * The result is normalized before being returned.
		 * @param a: The starting rotation.
		 * @param b: The ending rotation.
		 * @return: A new quaternion.
		 */
		static CQuaternion Lerp(const CQuaternion& a, const CQuaternion& b, float t, bool normalize = false);

		/**
		 * Interpolates between a and b by t. This normalizes the result when
		 * complete.
		 * @param a: The starting rotation.
		 * @param b: The ending rotation.
		 * @param t: The interpolation value.
		 * @param normalize: Weather or not to normalize the quaternion (default is true).
		 * @return: A new quaternion.
		 */
		static CQuaternion LerpUnclamped(const CQuaternion& a, const CQuaternion& b,
			float t, bool normalize = false);

		/**
		 * Creates a rotation with the specified forward direction. This is the
		 * same as calling LookRotation with (0, 1, 0) as the upwards vector.
		 * The output is undefined for parallel vectors.
		 * @param forward: The forward direction to look toward.
		 * @return: A new quaternion.
		 */
		static CQuaternion LookRotation(const CVector& forward);

		/**
		 * Creates a rotation with the specified forward and upwards directions.
		 * The output is undefined for parallel vectors.
		 * @param forward: The forward direction to look toward.
		 * @param upwards: The direction to treat as up.
		 * @return: A new quaternion.
		 */
		static CQuaternion LookRotation(const CVector& forward, const CVector& upwards);

		/**
		 * Returns the norm of a quaternion.
		 * @param rotation: The quaternion in question.
		 * @return: A scalar value.
		 */
		static float Norm(const CQuaternion& rotation);

		/**
		 * Returns a quaternion with identical rotation and a norm of one.
		 * @param rotation: The quaternion in question.
		 * @return: A new quaternion.
		 */
		static CQuaternion Normalized(const CQuaternion& rotation);
		CQuaternion Normalized() const;

		/**
		 * Returns a new CQuaternion created by rotating "from" towards "to" by
		 * "maxRadiansDelta". This will not overshoot, and if a negative delta is
		 * applied, it will rotate till completely opposite "to" and then stop.
		 * @param from: The rotation at which to start.
		 * @param to: The rotation at which to end.
		 # @param maxRadiansDelta: The maximum number of radians to rotate.
		 * @return: A new CQuaternion.
		 */
		static CQuaternion RotateTowards(const CQuaternion& from, const CQuaternion& to,
			float maxRadiansDelta);

		/**
		 * Returns a new quaternion interpolated between a and b, usinfg spherical
		 * linear interpolation. The variable t is clamped to the range [0-1]. The
		 * resulting quaternion will be normalized.
		 * @param a: The starting rotation.
		 * @param b: The ending rotation.
		 * @param t: The interpolation value.
		 * @param normalize: Weather or not to normalize the quaternion (default is true).
		 * @return: A new quaternion.
		 */
		static CQuaternion Slerp(const CQuaternion& a, const CQuaternion& b, float t, bool normalize = false);

		/**
		 * Returns a new quaternion interpolated between a and b, usinfg spherical
		 * linear interpolation. The resulting quaternion will be normalized.
		 * @param a: The starting rotation.
		 * @param b: The ending rotation.
		 * @param t: The interpolation value.
		 * @param normalize: Weather or not to normalize the quaternion (default is true).
		 * @return: A new quaternion.
		 */
		static CQuaternion SlerpUnclamped(const CQuaternion& a, const CQuaternion& b,
			float t, bool normalize = false);

		/**
		 * Slerps the current quaternion towards the target, similar to RotateTowards.
		 */
		static CQuaternion SlerpTowards(const CQuaternion& from, const CQuaternion& to, float maxRadiansDelta);

		/**
		 * Outputs the angle axis representation of the provided quaternion.
		 * @param rotation: The input quaternion.
		 * @param angle: The output angle.
		 * @param axis: The output axis.
		 */
		static void ToAngleAxis(const CQuaternion& rotation, float& angle,
			CVector& axis);

		/**
		 * Returns the Euler angle representation of a rotation. The resulting
		 * vector contains the rotations about the z, x and y axis, in that order.
		 * @param rotation: The quaternion to convert.
		 * @return: A new vector.
		 */
		static CVector ToEuler(const CQuaternion& rotation);

		CMatrix44 ToRotationMatrix() const;

		// These are used in Generations as far as I know.
		static CQuaternion FromAxes(const CVector& a1, const CVector& a2, const CVector& a3);
		static CQuaternion* FromAxes(CQuaternion* out, CVector* axisX, CVector* axisY, CVector* axisZ);
	};
}
