#pragma once

#include <random>
#include <chrono>
#include <vector>

#include "Vectors.hpp"

namespace ve {

template <typename T, typename... Rest>
void	hashCombine(std::size_t& seed, const T& v, const Rest&... rest)
{
	seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	(hashCombine(seed, rest), ...);
};

float		randomFloat(float min = 0.0f, float max = 1.0f);
int32_t		randomInt(int32_t min = -500, int32_t max = 500);
uint32_t	randomUint(uint32_t min = 0U, uint32_t max = 1000U);
vec3		generateRandomColor();
vec3		generateRandomGreyscale();
vec3		generateSoftGreyscale();

std::vector<char>	readFile(std::string const& filePath);

} // namespace ve