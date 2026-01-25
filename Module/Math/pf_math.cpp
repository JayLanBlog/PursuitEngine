#include "pf_math.h"


namespace pf::math {
	float TriangleArea(float a, float b, float c)
	{
		// Heron's formula:
		float p = (a + b + c) * 0.5f;
		return std::sqrt(p * (p - a) * (p - b) * (p - c));
	}

	float GetPointSegmentDistance(const XMVECTOR& point, const XMVECTOR& segmentA, const XMVECTOR& segmentB)
	{
		// Return minimum distance between line segment vw and point p
		const float l2 = XMVectorGetX(XMVector3LengthSq(segmentB - segmentA));  // i.e. |w-v|^2 -  avoid a sqrt
		if (l2 == 0.0) return Distance(point, segmentA);   // v == w case
												// Consider the line extending the segment, parameterized as v + t (w - v).
												// We find projection of point p onto the line. 
												// It falls where t = [(p-v) . (w-v)] / |w-v|^2
												// We clamp t from [0,1] to handle points outside the segment vw.
		const float t = std::max(0.0f, std::min(1.0f, XMVectorGetX(XMVector3Dot(point - segmentA, segmentB - segmentA)) / l2));
		const XMVECTOR projection = segmentA + t * (segmentB - segmentA);  // Projection falls on the segment
		return Distance(point, projection);
	}

	XMFLOAT3 GetQuadraticBezierPos(const XMFLOAT3& a, const XMFLOAT3& b, const XMFLOAT3& c, float t) {
		float param0, param1, param2;
		param0 = sqr(1 - t);
		param1 = 2 * (1 - t) * t;
		param2 = sqr(t);
		float x = param0 * a.x + param1 * b.x + param2 * c.x;
		float y = param0 * a.y + param1 * b.y + param2 * c.y;
		float z = param0 * a.z + param1 * b.z + param2 * c.z;
		return XMFLOAT3(x, y, z);
	}

	XMFLOAT3 GetQuadraticBezierPos(const XMFLOAT4& a, const XMFLOAT4& b, const XMFLOAT4& c, float t) {
		return GetQuadraticBezierPos(XMFLOAT3(a.x, a.y, a.z), XMFLOAT3(b.x, b.y, b.z), XMFLOAT3(c.x, c.y, c.z), t);
	}
	float GetAngle(XMVECTOR A, XMVECTOR B, XMVECTOR AXIS, float max)
	{
		float angle = XMVectorGetX(XMVector3AngleBetweenVectors(A, B));
		angle = std::min(angle, max);
		if (XMVectorGetX(XMVector3Dot(XMVector3Cross(A, B), AXIS)) < 0)
		{
			angle = XM_2PI - angle;
		}
		return angle;
	}

	float GetAngle(const XMFLOAT2& a, const XMFLOAT2& b)
	{
		float dot = a.x * b.x + a.y * b.y;      // dot product
		float det = a.x * b.y - a.y * b.x;		// determinant
		float angle = atan2f(det, dot);		// atan2(y, x) or atan2(sin, cos)
		if (angle < 0)
		{
			angle += XM_2PI;
		}
		return angle;
	}
	float GetAngleSigned(XMVECTOR A, XMVECTOR B, XMVECTOR AXIS)
	{
		float angle = XMVectorGetX(XMVector3AngleBetweenVectors(A, B));
		if (XMVectorGetX(XMVector3Dot(XMVector3Cross(A, B), AXIS)) < 0)
		{
			angle = -angle;
		}
		return angle;
	}
	XMFLOAT3 QuaternionToRollPitchYaw(const XMFLOAT4& quaternion)
	{
		float roll = atan2f(2 * quaternion.x * quaternion.w - 2 * quaternion.y * quaternion.z, 1 - 2 * quaternion.x * quaternion.x - 2 * quaternion.z * quaternion.z);
		float pitch = atan2f(2 * quaternion.y * quaternion.w - 2 * quaternion.x * quaternion.z, 1 - 2 * quaternion.y * quaternion.y - 2 * quaternion.z * quaternion.z);
		float yaw = asinf(2 * quaternion.x * quaternion.y + 2 * quaternion.z * quaternion.w);

		return XMFLOAT3(roll, pitch, yaw);
	}

}