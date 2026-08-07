#pragma once

#include <ostream>
#include <cmath>

#include "Utils.hpp"


class vec3i
{
	public:

	union
	{
		i32	data[3];
		struct
		{
			i32	x;
			i32	y;
			i32	z;
		};
		struct
		{
			i32	width;
			i32	height;
			i32	depth;
		};
	};

	constexpr vec3i() : x(0), y(0), z(0) {}
	constexpr explicit vec3i(i32 val) : x(val), y(val), z(val) {}
	constexpr vec3i(i32 x, i32 y, i32 z) : x(x), y(y), z(z) {}

	vec3i	operator+(const vec3i& other) const noexcept { return vec3i(x + other.x, y + other.y, z + other.z); }
	vec3i	operator-(const vec3i& other) const noexcept { return vec3i(x - other.x, y - other.y, z - other.z); }
	vec3i&	operator+=(const vec3i& other) noexcept { x += other.x; y += other.y, z += other.z; return *this; }
	vec3i&	operator-=(const vec3i& other) noexcept { x -= other.x; y -= other.y; z -= other.z; return *this; }

	bool	operator==(const vec3i& other) const noexcept { return x == other.x && y == other.y && z == other.z; }
	bool	operator!=(const vec3i& other) const noexcept { return !(*this == other); }

	bool	operator<(const vec3i& other) const noexcept;
	bool	operator<=(const vec3i& other) const noexcept { return *this < other || *this == other; }
	bool	operator>(const vec3i& other) const noexcept { return !(*this <= other); }
	bool	operator>=(const vec3i& other) const noexcept { return !(*this < other); }

	i32&		operator[](i32 index) noexcept { return data[index]; }
	const i32&	operator[](i32 index) const noexcept { return data[index]; }
	
	vec3i		clone() const noexcept { return vec3i(x, y, z); }
	float		length() const noexcept { return std::sqrt(x * x + y * y + z * z); }
	
	static vec3i	zero() noexcept { return vec3i(0.0f, 0.0f, 0.0f); }
	static float	distance(const vec3i& a, const vec3i& b) noexcept { return (b - a).length(); }

	friend vec3i	operator-(const vec3i& v) noexcept { return vec3i(-v.x, -v.y, -v.z); }

	private:
};

std::ostream&	operator<<(std::ostream& os, const vec3i& v);

namespace std {

template<>
struct hash<vec3i>
{
	size_t operator()(const vec3i& v) const noexcept
	{
		size_t h1 = hash<ui32>{}(v.x);
		size_t h2 = hash<ui32>{}(v.y);
		size_t h3 = hash<ui32>{}(v.z);
		size_t seed = h1;

		seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}
};

}	// namespace std