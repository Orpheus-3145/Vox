#pragma once

#include <cstdint>
#include <vector>

#include "Vulkan.hpp"


using i8 = int8_t;
using i32 = int32_t;
using i64 = int64_t;
using ui8 = uint8_t;
using ui32 = uint32_t;
using ui64 = uint64_t;

using VertexVector = std::vector<ve::VulkanModel::Vertex>;
using IndexVector = std::vector<ui32>;
