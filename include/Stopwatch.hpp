#pragma once

#include <chrono>

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
		Stopwatch() noexcept : startTime(Clock::now()) {};
		~Stopwatch() noexcept = default;
		Stopwatch(const Stopwatch& other) = delete;
		Stopwatch(Stopwatch&& other) = delete;
		Stopwatch& operator=(const Stopwatch& other) = delete;
		Stopwatch& operator=(Stopwatch&& other) = delete;

		void	start() noexcept { startTime = now(); }
		void	stop() noexcept { endTime = now(); elapsedTime = endTime - startTime; }
		void	reset() noexcept;

		Duration	elapsed() const noexcept { return elapsedTime; }
		double		elapsed(Unit type) const noexcept;
		
	private:
		Time		startTime{};
		Time		endTime{};
		Duration	elapsedTime{};
		
		Time	now() const noexcept { return Clock::now(); }
		double	ns() const noexcept { return std::chrono::duration<double, std::nano>(elapsedTime).count(); }
		double	us() const noexcept { return std::chrono::duration<double, std::micro>(elapsedTime).count(); }
		double	ms() const noexcept { return std::chrono::duration<double, std::milli>(elapsedTime).count(); }
		double	s() const noexcept { return std::chrono::duration<double>(elapsedTime).count(); }
};

std::ostream&	operator<<(std::ostream& os, const Stopwatch& stopwatch);

}	// namespace vox