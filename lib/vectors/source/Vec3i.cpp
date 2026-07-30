#include <algorithm>

#include "Vec3i.hpp"


bool	vec3i::operator<(const vec3i& other) const noexcept
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
	return z < other.z;
}

std::ostream&	operator<<(std::ostream& os, const vec3i& v)
{
	os << "vec3i(" << v.x << ", " << v.y << ", " << v.z << ")";
	return os;
}