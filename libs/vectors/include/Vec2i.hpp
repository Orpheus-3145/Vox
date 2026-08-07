#pragma once

#include <ostream>
#include <cmath>

#include "Utils.hpp"


class vec2i
{
	public:

	union
	{
		i32	data[2];
		struct
		{
			i32	x;
			i32	y;
		};
		struct
		{
			i32 width;
			i32 depth;
		};
	};

	constexpr vec2i() : x(0), y(0) {}
	constexpr explicit vec2i(i32 val) : x(val), y(val) {}
	constexpr vec2i(i32 x, i32 y) : x(x), y(y) {}

	vec2i	operator+(const vec2i& other) const noexcept { return vec2i(x + other.x, y + other.y); }
	vec2i	operator-(const vec2i& other) const noexcept { return vec2i(x - other.x, y - other.y); }
	vec2i&	operator+=(const vec2i& other) noexcept { x += other.x; y += other.y; return *this; }
	vec2i&	operator-=(const vec2i& other) noexcept { x -= other.x; y -= other.y; return *this; }

	bool	operator==(const vec2i& other) const noexcept { return x == other.x && y == other.y; }
	bool	operator!=(const vec2i& other) const noexcept { return !(*this == other); }

	bool	operator<(const vec2i& other) const noexcept;
	bool	operator<=(const vec2i& other) const noexcept { return *this < other || *this == other; }
	bool	operator>(const vec2i& other) const noexcept { return !(*this <= other); }
	bool	operator>=(const vec2i& other) const noexcept { return !(*this < other); }

	i32&		operator[](i32 index) noexcept { return data[index]; }
	const i32&	operator[](i32 index) const noexcept { return data[index]; }
	
	vec2i		clone() const noexcept { return vec2i(x, y); }
	float		length() const noexcept { return std::sqrt(x * x + y * y); }
	
	static float	distance(const vec2i& v1, const vec2i& v2) noexcept {return (v1 - v2).length(); }
	
	static vec2i	zero() noexcept { return vec2i(0.0f, 0.0f); }

	friend vec2i	operator-(const vec2i& v) noexcept { return vec2i(-v.x, -v.y); }

	private:
};

std::ostream&	operator<<(std::ostream& os, const vec2i& v);

namespace std {

template<>
struct hash<vec2i>
{
	size_t operator()(const vec2i& v) const noexcept
	{
		size_t h1 = hash<i32>{}(v.x);
		size_t h2 = hash<i32>{}(v.y);
		size_t seed = h1;

		seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}
};

}	// namespace std