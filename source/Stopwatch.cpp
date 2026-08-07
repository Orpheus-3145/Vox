#include <iostream>

#include "Stopwatch.hpp"


namespace vox {

float	Stopwatch::formatTime(const Duration& delta, Unit type) const noexcept
{
	switch (type)
	{
		case Unit::Nanoseconds:
			return std::chrono::duration<float,std::nano>(delta).count();
		case Unit::Microseconds:
			return std::chrono::duration<float,std::micro>(delta).count();
		case Unit::Milliseconds:
			return std::chrono::duration<float,std::milli>(delta).count();
		case Unit::Seconds:
			return std::chrono::duration<float>(delta).count();
		default:
			return 0.0;
	}
}

void	Stopwatch::reset() noexcept
{
	startTime = Clock::now();
	endTime = startTime;
	elapsedTime = Duration::zero();
}

std::ostream&	operator<<(std::ostream& os, const Stopwatch& stopwatch)
{
	Duration elapsed = stopwatch.elapsed();
	size_t elapsedTime = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();

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