#include "VulkanUtils.hpp"

#include <fstream>
#include <string>
#include <cassert>


namespace ve {

float randomFloat(float min, float max)
{
	assert( min < max && "Min value is bigger than max");
	static std::default_random_engine	engine(std::chrono::system_clock::now().time_since_epoch().count());
	static std::uniform_real_distribution<float> distribution(0.0f, 1.0f);

	return distribution(engine) * (max - min) + min;
}

int32_t randomInt(int32_t min, int32_t max)
{
	assert( min < max && "Min value is bigger than max");

	static std::default_random_engine engine(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<int32_t> distribution(min, max);
    return distribution(engine);
}

uint32_t randomUint(uint32_t min, uint32_t max)
{
	assert( min < max && "Min value is bigger than max");

	static std::default_random_engine engine(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<uint32_t> distribution(min, max);
    return distribution(engine);
}

vec3	generateRandomColor()
{
	return vec3(randomFloat(), randomFloat(), randomFloat());
}

vec3	generateRandomGreyscale()
{
	float	grey = randomFloat();

	return vec3(grey, grey, grey);
}

vec3	generateSoftGreyscale()
{
	static std::default_random_engine	engine(std::chrono::system_clock::now().time_since_epoch().count());
	static std::uniform_real_distribution<float>	distribution(0.25f, 0.4f);
	float	grey = distribution(engine);

	return vec3(grey, grey, grey);
}


std::vector<char> readFile(std::string const& filePath)
{
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);

	if (file.is_open() == false)
	{
		throw std::runtime_error("failed to open file: " + filePath);
	}
	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<char>	buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

}