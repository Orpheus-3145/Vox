#include <cmath>

#include "Vec4.hpp"


bool	vec4::operator<(const vec4& other) const noexcept
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
	if (z < other.z)
	{
		return true;
	}
	if (z > other.z)
	{
		return false;
	}
	return w < other.w;
}

vec4&	vec4::normalize() noexcept
{
	float len = length();

	if (len != 0.0f)
	{
		*this /= len;
	}
	return *this;
}


std::ostream&	operator<<(std::ostream& os, const vec4& v)
{
	os << "vec4(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")";
	return os;
}
