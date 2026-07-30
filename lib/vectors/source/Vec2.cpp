#include <algorithm>

#include "Vec2.hpp"


vec2&	vec2::operator=(const vec2& other)
{
	if (this != &other)
	{
		x = other.x;
		y = other.y;
	}
	return *this;
}

bool	vec2::operator<(const vec2& other) const noexcept
{
	if (x < other.x)
	{
		return true;
	}
	if (x > other.x)
	{
		return false;	
	}
	return y < other.y;
}

vec2&	vec2::normalize() noexcept
{
	float len = length();

	if (len != 0.0f)
	{
		*this /= len;
	}
	return *this;
}

vec2&	vec2::fastNormalize() noexcept
{
	float lenSq = lengthSquared();

	if (lenSq != 0.0f)
	{
		float invSqrt = fastInverseSqrt(lenSq);
		*this *= invSqrt;
	}
	return *this;
}

vec2&	vec2::rotate(float angleRadians) noexcept
{
	float	cosAngle = std::cos(angleRadians);
	float	sinAngle = std::sin(angleRadians);

	x = x * cosAngle - y * sinAngle;
	y = x * sinAngle + y * cosAngle;
	return *this;
}

vec2	vec2::rotated(float angleRadians) const noexcept
{
	float	cosAngle = std::cos(angleRadians);
	float	sinAngle = std::sin(angleRadians);

	return vec2(x * cosAngle - y * sinAngle, x * sinAngle + y * cosAngle);
}

/*	Careful, only insert normalized vectors when computing angles	*/

float	vec2::angle(const vec2& a, const vec2& b) noexcept
{
	float	dotProduct = vec2::dot(a, b);
	
	dotProduct = std::clamp(dotProduct, -1.0f, 1.0f);
	return std::acos(dotProduct);
}

float	vec2::distanceSquared(const vec2& a, const vec2& b) noexcept
{
	float	dx = b.x - a.x;
	float	dy = b.y - a.y;

	return dx * dx + dy * dy;
}

vec2	vec2::lerp(const vec2& a, const vec2& b, float t) noexcept
{
	return vec2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
}

std::ostream&	operator<<(std::ostream& os, const vec2& v)
{
	os << "vec2(" << v.x << ", " << v.y << ")";
	return os;
}