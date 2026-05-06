#include "Stopwatch.hpp"
#include "TypeAliases.hpp"

#include <iostream>

namespace vox {

double	Stopwatch::elapsed(Unit type) const noexcept
{
	switch (type)
	{
		case Unit::Nanoseconds:
			return ns();
		case Unit::Microseconds:
			return us();
		case Unit::Milliseconds:
			return ms();
		case Unit::Seconds:
			return s();
		default:
			return 0.0;
	}
}

void	Stopwatch::reset() noexcept
{
	startTime = now();
	endTime = startTime;
	elapsedTime = Duration::zero();
}

std::ostream&	operator<<(std::ostream& os, const Stopwatch& stopwatch)
{
	Duration elapsed = stopwatch.elapsed();
	i64 elapsedTime = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();

	if (elapsedTime < 1000)
	{
		os << "Elapsed time: " << stopwatch.elapsed(Unit::Nanoseconds) << " nanoseconds";
	}
	else if (elapsedTime < 1000000)
	{
		os << "Elapsed time: " << stopwatch.elapsed(Unit::Microseconds) << " microseconds";
	}
	else if (elapsedTime < 1000000000)
	{
		os << "Elapsed time: " << stopwatch.elapsed(Unit::Milliseconds) << " milliseconds";
	}
	else
	{
		os << "Elapsed time: " << stopwatch.elapsed(Unit::Seconds) << " seconds";
	}
	os << std::endl;
	return os;
}

}	// namespace vox