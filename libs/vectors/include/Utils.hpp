#pragma once

#include <cstdint>


using i8 = int8_t;
using i32 = int32_t;
using i64 = int64_t;
using ui8 = uint8_t;
using ui32 = uint32_t;
using ui64 = uint64_t;

union FloatIntUnion
{
	float	f;
	i32	i;
};

float	fastInverseSqrt(float number) noexcept;
float	degreesToRadians(float degrees) noexcept;
float	radiansToDegrees(float radians) noexcept;
