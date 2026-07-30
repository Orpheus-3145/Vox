#pragma once

#include <ostream>
#include <cstdint>
#include <cmath>


using i32 = int32_t;

class vec3i
{
	public:

	union
	{
		int	data[3];
		struct
		{
			int	x;
			int	y;
			int	z;
		};
		struct
		{
			i32	width;
			i32	height;
			i32	depth;
		};
	};

	constexpr vec3i() : x(0), y(0), z(0) {}
	constexpr explicit vec3i(int val) : x(val), y(val), z(val) {}
	constexpr vec3i(int x, int y, int z) : x(x), y(y), z(z) {}
	constexpr vec3i(const vec3i&) noexcept = default;
	constexpr vec3i(vec3i&&) noexcept = default;
	vec3i&	operator=(const vec3i&) noexcept = default;
	vec3i&	operator=(vec3i&&) noexcept = default;
	~vec3i() = default;

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

	int&		operator[](int index) noexcept { return data[index]; }
	const int&	operator[](int index) const noexcept { return data[index]; }
	
	vec3i		clone() const noexcept { return vec3i(x, y, z); }
	float		length() const noexcept { return std::sqrt(x * x + y * y + z * z); }
	uint32_t	length1D() const noexcept { return std::abs(x) + std::abs(y) + std::abs(z); }
	
	static vec3i	zero() noexcept { return vec3i(0.0f, 0.0f, 0.0f); }
	static float	distance(const vec3i& a, const vec3i& b) noexcept { return (b - a).length(); }
	// 1D distance or Manhattan distance
	static uint32_t	distance1D(const vec3i& v1, const vec3i& v2) noexcept {return (v1 - v2).length1D(); }

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
		size_t h1 = hash<uint32_t>{}(v.x);
		size_t h2 = hash<uint32_t>{}(v.y);
		size_t h3 = hash<uint32_t>{}(v.z);
		size_t seed = h1;

		seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}
};

}	// namespace std