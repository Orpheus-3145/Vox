#include <algorithm>

#include "Vec2ui.hpp"


bool	vec2ui::operator<(const vec2ui& other) const noexcept
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

std::ostream&	operator<<(std::ostream& os, const vec2ui& v)
{
	os << "vec2ui(" << v.x << ", " << v.y << ")";
	return os;
}