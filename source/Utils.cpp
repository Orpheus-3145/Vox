#include <sstream>
#include <iomanip>
#include <cassert>

#include "Utils.hpp"


namespace vox {

std::string formatBytes( size_t bytes ) {
	constexpr size_t KB = 1024UL;
	constexpr size_t MB = 1024UL * KB;
	constexpr size_t GB = 1024UL * MB;

	std::ostringstream oss;
	oss << std::fixed << std::setprecision(3);

	if (bytes >= GB)
		oss << static_cast<double>(bytes) / GB << " GiB";
	else if (bytes >= MB)
		oss << static_cast<double>(bytes) / MB << " MiB";
	else if (bytes >= KB)
		oss << static_cast<double>(bytes) / KB << " KiB";
	else
		oss << bytes << " B";

	return oss.str();
}

i32	positiveModulo(i32 value, i32 modulus)
{
	assert(modulus > 0 && "modulus must be positive");
	value = value % modulus;
	if (value < 0)
	{
		return value + modulus;
	}
	return value;
}

}	// namespace vox