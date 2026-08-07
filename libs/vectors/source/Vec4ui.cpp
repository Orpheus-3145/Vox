#include <cmath>

#include "Vec4ui.hpp"


bool	vec4ui::operator<(const vec4ui& other) const noexcept
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

std::ostream&	operator<<(std::ostream& os, const vec4ui& v)
{
	os << "vec4ui(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")";
	return os;
}