#pragma once

#include <string>
#include <cassert>

#include "TypeAliases.hpp"


namespace vox {

std::string formatBytes( size_t );
i32	positiveModulo(i32 value, i32 modulus);

}	// namespace vox