#include <algorithm>

#include "Vec2.hpp"
#include "Vec3.hpp"
#include "Vec4.hpp"
#include "Quat.hpp"
#include "Mat4.hpp"


vec3::vec3(const vec4& other) : x(other.x), y(other.y), z(other.z) {}

vec3&	vec3::operator=(const vec3& other)
{
	if (this != &other)
	{
		x = other.x;
		y = other.y;
		z = other.z;
	}
	return *this;
}

bool	vec3::operator<(const vec3& other) const noexcept
{
	if (x < other.x)
	{
		return true;
	}
	if (x > other.x)
	{
		return false;	
	}
	if (y < other.y)
	{
		return true;
	}
	if (y > other.y)
	{
		return false;
	}
	return z < other.z;
}

vec3&	vec3::normalize() noexcept
{
	float len = length();

	if (len != 0.0f)
	{
		*this /= len;
	}
	return *this;
}

vec3&	vec3::fastNormalize() noexcept
{
	float lenSq = lengthSquared();

	if (lenSq != 0.0f)
	{
		float invSqrt = fastInverseSqrt(lenSq);
		*this *= invSqrt;
	}
	return *this;
}

vec3&	vec3::rotate(const mat4& matrix, float angleRadians) noexcept
{
	(void)matrix;
	(void)angleRadians;
	// Rotation implementation using a rotation matrix would go here
	return *this;
}

vec3&	vec3::rotate(const quat& rotation) noexcept
{
	*this = quat::rotated(*this, rotation);
	return *this;
}

vec3&	vec3::rotate(const vec3& axis, float angleRadians) noexcept
{
	quat	rotation(angleRadians, axis);

	return this->rotate(rotation);
}

vec3	vec3::rotated(const quat& rotation) const noexcept
{
	return this->clone().rotate(rotation);
}

/*	Careful, only insert normalized vectors when computing angles	*/

float	vec3::angle(const vec3& a, const vec3& b) noexcept
{
	float	dotProd = vec3::dot(a, b);

	dotProd = std::clamp(dotProd, -1.0f, 1.0f);
	return std::acos(dotProd);
}

vec3	vec3::cross(const vec3& a, const vec3& b) noexcept
{
	return vec3(
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x
	);
}

float	vec3::distanceSquared(const vec3& a, const vec3& b) noexcept
{
	float	deltaX = b.x - a.x;
	float	deltaY = b.y - a.y;
	float	deltaZ = b.z - a.z;

	return deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ;
}

vec3	vec3::lerp(const vec3& a, const vec3& b, float t) noexcept
{
	return vec3(
		a.x + (b.x - a.x) * t,
		a.y + (b.y - a.y) * t,
		a.z + (b.z - a.z) * t
	);
}

std::ostream&	operator<<(std::ostream& os, const vec3& v)
{
	os << "vec3(" << v.x << ", " << v.y << ", " << v.z << ")";
	return os;
}