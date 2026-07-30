#pragma once

#include <ostream>
#include <cstdint>


using ui32 = uint32_t;

class vec3ui
{
	public:

	union
	{
		ui32	data[3];
		struct
		{
			ui32	x;
			ui32	y;
			ui32	z;
		};
		struct
		{
			ui32	width;
			ui32	height;
			ui32	depth;
		};
	};

	constexpr vec3ui() : x(0), y(0), z(0) {}
	constexpr explicit vec3ui(ui32 val) : x(val), y(val), z(val) {}
	constexpr vec3ui(ui32 x, ui32 y, ui32 z) : x(x), y(y), z(z) {}
	constexpr vec3ui(const vec3ui&) noexcept = default;
	constexpr vec3ui(vec3ui&&) noexcept = default;
	vec3ui&	operator=(const vec3ui&) noexcept = default;
	vec3ui&	operator=(vec3ui&&) noexcept = default;
	~vec3ui() = default;

	vec3ui	operator+(const vec3ui& other) const noexcept { return vec3ui(x + other.x, y + other.y, z + other.z); }
	vec3ui	operator-(const vec3ui& other) const noexcept { return vec3ui(x - other.x, y - other.y, z - other.z); }
	vec3ui&	operator+=(const vec3ui& other) noexcept { x += other.x; y += other.y, z += other.z; return *this; }
	vec3ui&	operator-=(const vec3ui& other) noexcept { x -= other.x; y -= other.y; z -= other.z; return *this; }

	bool	operator==(const vec3ui& other) const noexcept { return x == other.x && y == other.y && z == other.z; }
	bool	operator!=(const vec3ui& other) const noexcept { return !(*this == other); }
	bool	operator<(const vec3ui& other) const noexcept;
	bool	operator<=(const vec3ui& other) const noexcept { return *this < other || *this == other; }
	bool	operator>(const vec3ui& other) const noexcept { return !(*this <= other); }
	bool	operator>=(const vec3ui& other) const noexcept { return !(*this < other); }

	ui32&		operator[](ui32 index) noexcept { return data[index]; }
	const ui32&	operator[](ui32 index) const noexcept { return data[index]; }
	
	vec3ui	clone() const noexcept { return vec3ui(x, y, z); }
	
	static vec3ui	zero() noexcept { return vec3ui(0.0f, 0.0f, 0.0f); }

	friend vec3ui	operator-(const vec3ui& v) noexcept { return vec3ui(-v.x, -v.y, -v.z); }

	private:
};

std::ostream&	operator<<(std::ostream& os, const vec3ui& v);

namespace std {

template<>
struct hash<vec3ui>
{
	size_t operator()(const vec3ui& v) const noexcept
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