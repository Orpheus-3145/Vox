#pragma once

#include <ostream>

#include "Utils.hpp"


class vec2ui
{
	public:

	union
	{
		ui32	data[2];
		struct
		{
			ui32	x;
			ui32	y;
		};
	};

	constexpr vec2ui() : x(0), y(0) {}
	constexpr explicit vec2ui(ui32 val) : x(val), y(val) {}
	constexpr vec2ui(ui32 x, ui32 y) : x(x), y(y) {}

	vec2ui	operator+(const vec2ui& other) const noexcept { return vec2ui(x + other.x, y + other.y); }
	vec2ui	operator-(const vec2ui& other) const noexcept { return vec2ui(x - other.x, y - other.y); }
	vec2ui&	operator+=(const vec2ui& other) noexcept { x += other.x; y += other.y; return *this; }
	vec2ui&	operator-=(const vec2ui& other) noexcept { x -= other.x; y -= other.y; return *this; }

	bool	operator==(const vec2ui& other) const noexcept { return x == other.x && y == other.y; }
	bool	operator!=(const vec2ui& other) const noexcept { return !(*this == other); }

	bool	operator<(const vec2ui& other) const noexcept;
	bool	operator<=(const vec2ui& other) const noexcept { return *this < other || *this == other; }
	bool	operator>(const vec2ui& other) const noexcept { return !(*this <= other); }
	bool	operator>=(const vec2ui& other) const noexcept { return !(*this < other); }

	ui32&		operator[](ui32 index) noexcept { return data[index]; }
	const ui32&	operator[](ui32 index) const noexcept { return data[index]; }
	
	vec2ui	clone() const noexcept { return vec2ui(x, y); }
	
	static vec2ui	zero() noexcept { return vec2ui(0.0f, 0.0f); }

	friend vec2ui	operator-(const vec2ui& v) noexcept { return vec2ui(-v.x, -v.y); }

	private:
};

std::ostream&	operator<<(std::ostream& os, const vec2ui& v);

namespace std {

template<>
struct hash<vec2ui>
{
	size_t operator()(const vec2ui& v) const noexcept
	{
		size_t h1 = hash<ui32>{}(v.x);
		size_t h2 = hash<ui32>{}(v.y);
		size_t seed = h1;

		seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}
};

}	// namespace std