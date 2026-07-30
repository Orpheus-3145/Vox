#pragma once

#include <ostream>

#include "Vec3ui.hpp"


using ui32 = uint32_t;

class vec4ui
{
	public:

	union
	{
		struct
		{
			ui32	x;
			ui32	y;
			ui32	z;
			ui32	w;
		};
		struct
		{
			ui32	width;
			ui32	height;
			ui32	depth;
			ui32	index;
		};
		ui32	data[4];
	};

	vec4ui() : x(0), y(0), z(0), w(0) {}
	vec4ui(ui32 val) : x(val), y(val), z(val), w(val) {}
	vec4ui(ui32 x, ui32 y, ui32 z, ui32 w) : x(x), y(y), z(z), w(w) {}
	vec4ui(const vec3ui& vec3, ui32 w) : x(vec3.width), y(vec3.height), z(vec3.depth), w(w) {}
	vec4ui(const vec4ui& other) : x(other.x), y(other.y), z(other.z), w(other.w) {}
	vec4ui&	operator=(const vec4ui& other);
	~vec4ui() = default;

	vec4ui	operator+(const vec4ui& other) const noexcept { return vec4ui(x + other.x, y + other.y, z + other.z, w + other.w); }
	vec4ui	operator-(const vec4ui& other) const noexcept { return vec4ui(x - other.x, y - other.y, z - other.z, w - other.w); }
	vec4ui	operator*(ui32 scalar) const noexcept { return vec4ui(x * scalar, y * scalar, z * scalar, w * scalar); }
	vec4ui	operator/(ui32 scalar) const noexcept { return vec4ui(x / scalar, y / scalar, z / scalar, w / scalar); }
	vec4ui&	operator+=(const vec4ui& other) noexcept { x += other.x; y += other.y, z += other.z; w += other.w; return *this; }
	vec4ui&	operator-=(const vec4ui& other) noexcept { x -= other.x; y -= other.y; z -= other.z; w -= other.w; return *this; }
	vec4ui&	operator*=(ui32 scalar) noexcept { x *= scalar; y *= scalar; z *= scalar; w *= scalar; return *this; }
	vec4ui&	operator/=(ui32 scalar) noexcept { x /= scalar; y /= scalar; z /= scalar; w /= scalar; return *this; }
	bool	operator==(const vec4ui& other) const noexcept { return x == other.x && y == other.y && z == other.z && w == other.w; }
	bool	operator!=(const vec4ui& other) const noexcept { return !(*this == other); }

	ui32&		operator[](int index) noexcept { return data[index]; }
	const ui32&	operator[](int index) const noexcept { return data[index]; }
};