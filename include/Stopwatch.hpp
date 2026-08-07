#pragma once

#include <chrono>

#include "Vectors.hpp"


namespace vox {

using Clock = std::chrono::steady_clock;
using Time = Clock::time_point;
using Duration = Clock::duration;

enum class Unit
{
	Nanoseconds,
	Microseconds,
	Milliseconds,
	Seconds
};

class Stopwatch
{
	public:
		Stopwatch() noexcept : startTime(Clock::now()) {}

		void	start() noexcept { startTime = Clock::now(); }
		void	stop() noexcept { endTime = Clock::now(); elapsedTime = endTime - startTime; }
		void	reset() noexcept;

		Duration	getTime() const noexcept { return Clock::now() - startTime; }
		float		getTime(Unit type) const noexcept { return formatTime(Clock::now() - startTime, type); }

		Duration	elapsed() const noexcept { return elapsedTime; }
		float		elapsed(Unit type) const noexcept { return formatTime(elapsedTime, type); }

	private:
		Time		startTime{Clock::now()};
		Time		endTime{};
		Duration	elapsedTime{};
		
		float	formatTime(const Duration& delta, Unit type) const noexcept;
};

std::ostream&	operator<<(std::ostream& os, const Stopwatch& stopwatch);

}	// namespace vox