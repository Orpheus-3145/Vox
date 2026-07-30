#include <algorithm>

#include "Vec2i.hpp"


bool	vec2i::operator<(const vec2i& other) const noexcept
{
	if (x < other.x)
	{
		return true;
	}
	if (x > other.x)
	{
		return false;	
	}
	return y < other.y;
}

std::ostream&	operator<<(std::ostream& os, const vec2i& v)
{
	os << "vec2i(" << v.x << ", " << v.y << ")";
	return os;
}