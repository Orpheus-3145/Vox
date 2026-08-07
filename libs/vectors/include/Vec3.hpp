#pragma once

#include <algorithm>
#include <ostream>
#include <cmath>

#include "Vec2.hpp"
#include "Utils.hpp"


class vec2;
class mat4;
class quat;

class vec3
{
	public:

	union
	{
		float	data[3];
		struct
		{
			float	x;
			float	y;
			float	z;
		};
		struct
		{
			float	r;
			float	g;
			float	b;
		};
	};

	constexpr vec3() : x(0.0f), y(0.0f), z(0.0f) {}
	constexpr explicit vec3(float val) : x(val), y(val), z(val) {}
	constexpr vec3(float x, float y, float z) : x(x), y(y), z(z) {}
	constexpr vec3(i32 x, i32 y, i32 z) : x(static_cast<float>(x)), y(static_cast<float>(y)), z(static_cast<float>(z)) {}
	constexpr explicit vec3(vec2 const& v2, float z = 0.0f) : x(v2.x), y(v2.y), z(z) {}

	vec3	operator+(const vec3& other) const noexcept { return vec3(x + other.x, y + other.y, z + other.z); }
	vec3	operator-(const vec3& other) const noexcept { return vec3(x - other.x, y - other.y, z - other.z); }
	vec3	operator*(float scalar) const noexcept { return vec3(x * scalar, y * scalar, z * scalar); }
	vec3	operator/(float scalar) const noexcept { return vec3(x / scalar, y / scalar, z / scalar); }
	vec3&	operator+=(const vec3& other) noexcept { x += other.x; y += other.y, z += other.z; return *this; }
	vec3&	operator-=(const vec3& other) noexcept { x -= other.x; y -= other.y; z -= other.z; return *this; }
	vec3&	operator*=(float scalar) noexcept { x *= scalar; y *= scalar; z *= scalar; return *this; }
	vec3&	operator/=(float scalar) noexcept { x /= scalar; y /= scalar; z /= scalar; return *this; }

	bool	operator==(const vec3& other) const noexcept { return x == other.x && y == other.y && z == other.z; }
	bool	operator!=(const vec3& other) const noexcept { return !(*this == other); }

	bool	operator<(const vec3& other) const noexcept;
	bool	operator<=(const vec3& other) const noexcept { return *this < other || *this == other; }
	bool	operator>(const vec3& other) const noexcept { return !(*this <= other); }
	bool	operator>=(const vec3& other) const noexcept { return !(*this < other); }

	float&			operator[](i32 index) noexcept { return data[index]; }
	const float&	operator[](i32 index) const noexcept { return data[index]; }

	float	length() const noexcept { return std::sqrt(x * x + y * y + z * z); }
	float	lengthSquared() const noexcept { return x * x + y * y + z * z; }
	vec3&	normalize() noexcept;
	vec3	normalized() const noexcept { return this->clone().normalize(); }
	vec3&	fastNormalize() noexcept;
	vec3	fastNormalized() const noexcept { return this->clone().fastNormalize(); }
	vec3&	rotate(const mat4& matrix, float angleRadians) noexcept;
	vec3&	rotate(const quat& rotation) noexcept;
	vec3&	rotate(const vec3& axis, float angleRadians) noexcept;
	vec3	rotated(const mat4& matrix, float angleRadians) const noexcept { return this->clone().rotate(matrix, angleRadians); }
	vec3	rotated(const quat& rotation) const noexcept;
	
	vec3	clone() const noexcept { return vec3(x, y, z); }
	vec3&	invert() noexcept { *this *= -1.0f; return *this; }
	vec3	inverted() const noexcept { return this->clone().invert(); }
	vec3	reflect(const vec3& normal) const noexcept { return *this - 2.0f * vec3::dot(*this, normal) * normal; }

	static float	angle(const vec3& a, const vec3& b) noexcept;
	static float	angleRadians(const vec3& a, const vec3& b) noexcept { return vec3::angle(a, b); }
	static float	angleDegrees(const vec3& a, const vec3& b) noexcept { return radiansToDegrees(vec3::angle(a, b)); }
	static float	distance(const vec3& a, const vec3& b) noexcept { return (b - a).length(); }

	static float	dot(const vec3& a, const vec3& b) noexcept { return a.x * b.x + a.y * b.y + a.z * b.z; }
	static vec3		cross(const vec3& a, const vec3& b) noexcept;
	static float	distanceSquared(const vec3& a, const vec3& b) noexcept;
	static vec3		lerp(const vec3& a, const vec3& b, float t) noexcept;
	
	static constexpr vec3	zero() noexcept { return vec3(0.0f, 0.0f, 0.0f); }
	static constexpr vec3	one() noexcept { return vec3(1.0f, 1.0f, 1.0f); }
	static constexpr vec3	up() noexcept { return vec3(0.0f, 1.0f, 0.0f); }
	static constexpr vec3	down() noexcept { return vec3(0.0f, -1.0f, 0.0f); }
	static constexpr vec3	left() noexcept { return vec3(-1.0f, 0.0f, 0.0f); }
	static constexpr vec3	right() noexcept { return vec3(1.0f, 0.0f, 0.0f); }
	static constexpr vec3	forward() noexcept { return vec3(0.0f, 0.0f, 1.0f); }
	static constexpr vec3	backward() noexcept { return vec3(0.0f, 0.0f, -1.0f); }

	friend vec3	operator*(float scalar, const vec3& v) noexcept { return vec3(v.x * scalar, v.y * scalar, v.z * scalar); }
	friend vec3 operator/(float scalar, const vec3& v) noexcept { return vec3(scalar / v.x, scalar / v.y, scalar / v.z); }
	friend vec3	operator-(const vec3& v) noexcept { return vec3(-v.x, -v.y, -v.z); }

	private:
};

std::ostream&	operator<<(std::ostream& os, const vec3& v);

namespace std {

template<>
struct hash<vec3>
{
	size_t operator()(const vec3& v) const noexcept
	{
		size_t h1 = hash<float>{}(v.x);
		size_t h2 = hash<float>{}(v.y);
		size_t h3 = hash<float>{}(v.z);
		size_t seed = h1;

		seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}
};

}	// namespace std