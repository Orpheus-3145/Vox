#pragma once

#include <cmath>
#include <ostream>


float	fastInverseSqrt(float number) noexcept;
float	radiansToDegrees(float radians) noexcept;

class vec2
{
	public:
	
	union
	{
		float	data[2];
		struct
		{
			float	x;
			float	y;
		};
	};

	constexpr vec2() : x(0.0f), y(0.0f) {}
	constexpr explicit vec2(float val) : x(val), y(val) {}
	constexpr vec2(float x, float y) : x(x), y(y) {}
	constexpr vec2(const vec2& other) : x(other.x), y(other.y) {}
	vec2&	operator=(const vec2& other);
	~vec2() = default;

	vec2	operator+(const vec2& other) const noexcept { return vec2(x + other.x, y + other.y); }
	vec2	operator-(const vec2& other) const noexcept { return vec2(x - other.x, y - other.y); }
	vec2	operator*(float scalar) const noexcept { return vec2(x * scalar, y * scalar); }
	vec2	operator/(float scalar) const noexcept { return vec2(x / scalar, y / scalar); }
	vec2&	operator+=(const vec2& other) noexcept { x += other.x; y += other.y; return *this; }
	vec2&	operator-=(const vec2& other) noexcept { x -= other.x; y -= other.y; return *this; }
	vec2&	operator*=(float scalar) noexcept { x *= scalar; y *= scalar; return *this; }
	vec2&	operator/=(float scalar) noexcept { x /= scalar; y /= scalar; return *this; }

	bool	operator==(const vec2& other) const noexcept { return x == other.x && y == other.y; }
	bool	operator!=(const vec2& other) const noexcept { return !(*this == other); }
	bool	operator<(const vec2& other) const noexcept;
	bool	operator<=(const vec2& other) const noexcept { return *this < other || *this == other; }
	bool	operator>(const vec2& other) const noexcept { return !(*this <= other); }
	bool	operator>=(const vec2& other) const noexcept { return !(*this < other); }

	float&	operator[](int index) noexcept { return data[index]; }
	vec2	clone() const noexcept { return vec2(x, y); }
	float	length() const noexcept { return std::sqrt(x * x + y * y); }
	vec2&	normalize() noexcept;
	vec2&	fastNormalize() noexcept;
	vec2	normalized() const noexcept { return this->clone().normalize(); }
	vec2	fastNormalized() const noexcept { return this->clone().fastNormalize(); }
	vec2&	rotate(float angleRadians) noexcept;
	vec2	rotated(float angleRadians) const noexcept;

	vec2&	invert() noexcept { *this *= -1.0f; return *this; }
	vec2	reflect(const vec2& normal) const noexcept { return *this - 2.0f * vec2::dot(*this, normal) * normal; }
	float	lengthSquared() const noexcept { return x * x + y * y; }

	static float	angle(const vec2& a, const vec2& b) noexcept;
	static float	angleRadians(const vec2& a, const vec2& b) noexcept { return vec2::angle(a, b); }
	static float	angleDegrees(const vec2& a, const vec2& b) noexcept { return radiansToDegrees(vec2::angle(a, b)); }
	static float	distance(const vec2& a, const vec2& b) noexcept { return (b - a).length(); }

	static float	dot(const vec2& a, const vec2& b) noexcept { return a.x * b.x + a.y * b.y; }
	static float	distanceSquared(const vec2& a, const vec2& b) noexcept;
	static vec2		lerp(const vec2& a, const vec2& b, float t) noexcept;
	
	static vec2	zero() noexcept { return vec2(0.0f, 0.0f); }
	static vec2	one() noexcept { return vec2(1.0f, 1.0f); }
	static vec2	up() noexcept { return vec2(0.0f, 1.0f); }
	static vec2	right() noexcept { return vec2(1.0f, 0.0f); }

	friend vec2	operator*(float scalar, const vec2& v) noexcept { return vec2(v.x * scalar, v.y * scalar); }

	private:
};

std::ostream&	operator<<(std::ostream& os, const vec2& v);

namespace std {

template<>
struct hash<vec2>
{
	size_t operator()(const vec2& v) const noexcept
	{
		size_t h1 = hash<float>{}(v.x);
		size_t h2 = hash<float>{}(v.y);
		size_t seed = h1;

		seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}
};

}	// namespace std