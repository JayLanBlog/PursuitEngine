#pragma once

#include "Core/core_include.h"


#include <cmath>

#include <algorithm>

#include <limits>

#ifndef CKED_CMAKE_BUILD
#define _XM_F16C_INTRINSICS_
#define _XM_FMA3_INTRINSICS_
#endif

namespace pf::math {
	inline constexpr XMFLOAT4X4 IDENTITY_MATRIX = XMFLOAT4X4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

	inline constexpr float PI = 3.1415926;

	inline bool float_equal(float f1, float f2) {
		return (std::abs(f1 - f2) <= std::numeric_limits<float>::epsilon() * std::max(std::abs(f1), std::abs(f2)));
	}

	constexpr float saturate(float x)
	{
		return ::saturate(x);
	}
	
	constexpr float InverseLerp(float value1, float value2, float pos)
	{
		return ::inverse_lerp(value1, value2, pos);
	}

	constexpr float Lerp(float value1, float value2, float amount)
	{
		return ::lerp(value1, value2, amount);
	}

	constexpr float Clamp(float val, float min, float max)
	{
		return std::min(max, std::max(min, val));
	}

	constexpr float SmoothStep(float value1, float value2, float amount)
	{
		amount = Clamp((amount - value1) / (value2 - value1), 0.0f, 1.0f);
		return amount * amount * amount * (amount * (amount * 6 - 15) + 10);
	}

	constexpr uint32_t GetNextPowerOfTwo(uint32_t x)
	{
		--x;
		x |= x >> 1;
		x |= x >> 2;
		x |= x >> 4;
		x |= x >> 8;
		x |= x >> 16;
		return ++x;
	}

	constexpr uint64_t GetNextPowerOfTwo(uint64_t x)
	{
		--x;
		x |= x >> 1;
		x |= x >> 2;
		x |= x >> 4;
		x |= x >> 8;
		x |= x >> 16;
		x |= x >> 32u;
		return ++x;
	}

	// a, b, c: trangle side lengths
	float TriangleArea(float a, float b, float c);

	constexpr float SphereSurfaceArea(float radius)
	{
		return 4 * PI * radius * radius;
	}
	constexpr float SphereVolume(float radius)
	{
		return 4.0f / 3.0f * PI * radius * radius * radius;
	}

	constexpr float RadiansToDegrees(float radians) { return radians / PI * 180.0f; }
	constexpr float DegreesToRadians(float degrees) { return degrees / 180.0f * PI; }

	constexpr uint32_t pack_unorm16x2(float x, float y)
	{
		return uint32_t(saturate(x) * 65535.0f) | (uint32_t(saturate(y) * 65535.0f) << 16u);
	}
	
	inline float Distance(XMVECTOR v1, XMVECTOR v2)
	{
		return XMVectorGetX(XMVector3Length(XMVectorSubtract(v1, v2)));
	}
	inline float DistanceSquared(XMVECTOR v1, XMVECTOR v2)
	{
		return XMVectorGetX(XMVector3LengthSq(XMVectorSubtract(v1, v2)));
	}


	inline float DistanceEstimated(const XMVECTOR& v1, const XMVECTOR& v2)
	{
		XMVECTOR vectorSub = XMVectorSubtract(v1, v2);
		XMVECTOR length = XMVector3LengthEst(vectorSub);

		float Distance = 0.0f;
		XMStoreFloat(&Distance, length);
		return Distance;
	}
	inline float Dot(const XMFLOAT2& v1, const XMFLOAT2& v2)
	{
		XMVECTOR vector1 = XMLoadFloat2(&v1);
		XMVECTOR vector2 = XMLoadFloat2(&v2);
		return XMVectorGetX(XMVector2Dot(vector1, vector2));
	}
	inline float Dot(const XMFLOAT3& v1, const XMFLOAT3& v2)
	{
		XMVECTOR vector1 = XMLoadFloat3(&v1);
		XMVECTOR vector2 = XMLoadFloat3(&v2);
		return XMVectorGetX(XMVector3Dot(vector1, vector2));
	}
	inline float Distance(const XMFLOAT2& v1, const XMFLOAT2& v2)
	{
		XMVECTOR vector1 = XMLoadFloat2(&v1);
		XMVECTOR vector2 = XMLoadFloat2(&v2);
		return XMVectorGetX(XMVector2Length(vector2 - vector1));
	}
	inline float Distance(const XMFLOAT3& v1, const XMFLOAT3& v2)
	{
		XMVECTOR vector1 = XMLoadFloat3(&v1);
		XMVECTOR vector2 = XMLoadFloat3(&v2);
		return Distance(vector1, vector2);
	}
	inline float DistanceSquared(const XMFLOAT2& v1, const XMFLOAT2& v2)
	{
		XMVECTOR vector1 = XMLoadFloat2(&v1);
		XMVECTOR vector2 = XMLoadFloat2(&v2);
		return XMVectorGetX(XMVector2LengthSq(vector2 - vector1));
	}
	inline float DistanceSquared(const XMFLOAT3& v1, const XMFLOAT3& v2)
	{
		XMVECTOR vector1 = XMLoadFloat3(&v1);
		XMVECTOR vector2 = XMLoadFloat3(&v2);
		return DistanceSquared(vector1, vector2);
	}
	inline float DistanceSquared(const XMVECTOR& v1, const XMFLOAT3& v2)
	{
		XMVECTOR vector2 = XMLoadFloat3(&v2);
		return DistanceSquared(v1, vector2);
	}
	inline float DistanceSquared(const XMFLOAT3& v1, const XMVECTOR& v2)
	{
		XMVECTOR vector1 = XMLoadFloat3(&v1);
		return DistanceSquared(vector1, v2);
	}
	inline float DistanceEstimated(const XMFLOAT2& v1, const XMFLOAT2& v2)
	{
		XMVECTOR vector1 = XMLoadFloat2(&v1);
		XMVECTOR vector2 = XMLoadFloat2(&v2);
		return XMVectorGetX(XMVector2LengthEst(vector2 - vector1));
	}
	inline float DistanceEstimated(const XMFLOAT3& v1, const XMFLOAT3& v2)
	{
		XMVECTOR vector1 = XMLoadFloat3(&v1);
		XMVECTOR vector2 = XMLoadFloat3(&v2);
		return DistanceEstimated(vector1, vector2);
	}
	inline XMVECTOR ClosestPointOnLine(const XMVECTOR& A, const XMVECTOR& B, const XMVECTOR& Point)
	{
		XMVECTOR AB = B - A;
		XMVECTOR T = XMVector3Dot(Point - A, AB) / XMVector3Dot(AB, AB);
		return A + T * AB;
	}
	inline XMVECTOR ClosestPointOnLineSegment(const XMVECTOR& A, const XMVECTOR& B, const XMVECTOR& Point)
	{
		XMVECTOR AB = B - A;
		XMVECTOR T = XMVector3Dot(Point - A, AB) / XMVector3Dot(AB, AB);
		return A + XMVectorSaturate(T) * AB;
	}
	constexpr XMFLOAT3 getVectorHalfWayPoint(const XMFLOAT3& a, const XMFLOAT3& b)
	{
		return XMFLOAT3((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f, (a.z + b.z) * 0.5f);
	}
	inline XMVECTOR InverseLerp(XMVECTOR value1, XMVECTOR value2, XMVECTOR pos)
	{
		return (pos - value1) / (value2 - value1);
	} 

	inline XMUINT2 pack_half4(float x, float y, float z, float w)
	{
		return XMUINT2(
			(uint32_t)XMConvertFloatToHalf(x) | ((uint32_t)XMConvertFloatToHalf(y) << 16u),
			(uint32_t)XMConvertFloatToHalf(z) | ((uint32_t)XMConvertFloatToHalf(w) << 16u)
		);
	}

	inline XMUINT2 pack_half4(const XMFLOAT4& value)
	{
		return pack_half4(value.x, value.y, value.z, value.w);
	}


	inline uint32_t pack_half2(float x, float y)
	{
		return (uint32_t)XMConvertFloatToHalf(x) | ((uint32_t)XMConvertFloatToHalf(y) << 16u);
	}

	inline uint32_t pack_half2(const XMFLOAT2& value)
	{
		return pack_half2(value.x, value.y);
	}

	constexpr XMFLOAT2 Max(const XMFLOAT2& a, const XMFLOAT2& b) {
		return XMFLOAT2(std::max(a.x, b.x), std::max(a.y, b.y));
	}
	constexpr XMFLOAT3 Max(const XMFLOAT3& a, const XMFLOAT3& b) {
		return XMFLOAT3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
	}
	constexpr XMFLOAT4 Max(const XMFLOAT4& a, const XMFLOAT4& b) {
		return XMFLOAT4(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z), std::max(a.w, b.w));
	}
	constexpr XMFLOAT2 Min(const XMFLOAT2& a, const XMFLOAT2& b) {
		return XMFLOAT2(std::min(a.x, b.x), std::min(a.y, b.y));
	}
	constexpr XMFLOAT3 Min(const XMFLOAT3& a, const XMFLOAT3& b) {
		return XMFLOAT3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
	}
	constexpr XMFLOAT4 Min(const XMFLOAT4& a, const XMFLOAT4& b) {
		return XMFLOAT4(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z), std::min(a.w, b.w));
	}
	constexpr XMFLOAT2 Abs(const XMFLOAT2& a) {
		return XMFLOAT2(std::abs(a.x), std::abs(a.y));
	}
	constexpr XMFLOAT3 Abs(const XMFLOAT3& a) {
		return XMFLOAT3(std::abs(a.x), std::abs(a.y), std::abs(a.z));
	}
	constexpr XMFLOAT4 Abs(const XMFLOAT4& a) {
		return XMFLOAT4(std::abs(a.x), std::abs(a.y), std::abs(a.z), std::abs(a.w));
	}

	float GetPointSegmentDistance(const XMVECTOR& point, const XMVECTOR& segmentA, const XMVECTOR& segmentB);


	inline float GetPlanePointDistance(const XMVECTOR& planeOrigin, const XMVECTOR& planeNormal, const XMVECTOR& point)
	{
		return XMVectorGetX(XMVector3Dot(planeNormal, point - planeOrigin));
	} 

	constexpr bool Collision2D(const XMFLOAT2& hitBox1Pos, const XMFLOAT2& hitBox1Siz, const XMFLOAT2& hitBox2Pos, const XMFLOAT2& hitBox2Siz)
	{
		if (hitBox1Siz.x <= 0 || hitBox1Siz.y <= 0 || hitBox2Siz.x <= 0 || hitBox2Siz.y <= 0)
			return false;

		if (hitBox1Pos.x + hitBox1Siz.x < hitBox2Pos.x)
			return false;
		else if (hitBox1Pos.x > hitBox2Pos.x + hitBox2Siz.x)
			return false;
		else if (hitBox1Pos.y + hitBox1Siz.y < hitBox2Pos.y)
			return false;
		else if (hitBox1Pos.y > hitBox2Pos.y + hitBox2Siz.y)
			return false;

		return true;
	}



	XMFLOAT3 GetQuadraticBezierPos(const XMFLOAT3& a, const XMFLOAT3& b, const XMFLOAT3& c, float t);
	XMFLOAT3 GetQuadraticBezierPos(const XMFLOAT4& a, const XMFLOAT4& b, const XMFLOAT4& c, float t);
	inline XMVECTOR GetQuadraticBezierPos(const XMVECTOR& a, const XMVECTOR& b, const XMVECTOR& c, float t)
	{
		// XMVECTOR optimized version
		const float param0 = sqr(1 - t);
		const float param1 = 2 * (1 - t) * t;
		const float param2 = sqr(t);
		const XMVECTOR param = XMVectorSet(param0, param1, param2, 1);
		const XMMATRIX M = XMMATRIX(a, b, c, XMVectorSet(0, 0, 0, 1));
		return XMVector3TransformNormal(param, M);
	}
}