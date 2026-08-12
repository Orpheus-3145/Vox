#include "NoiseGenerator.hpp"
#include "Utils.hpp"

#include <cassert>


namespace vox {

NoiseGenerator::NoiseGenerator( ui32 seed, ui32 mapSize )
{
	assert(mapSize > 1U && "Provide at least 2 slots for permutation array");
	this->nPermutations = mapSize;
	this->noiseScalar = NoiseGenerator::N_FEATURES / static_cast<float>(this->nPermutations);

	this->terrainSeed = seed ^ (1 * NoiseGenerator::GOLDEN_RATIO_HASH);
	this->cavesSeed = seed ^ (2 * NoiseGenerator::GOLDEN_RATIO_HASH);
	this->rngSeed.seed(seed ^ (3 * NoiseGenerator::GOLDEN_RATIO_HASH));

	this->permutations.resize(this->nPermutations * 2);
	auto endSequence = this->permutations.begin() + this->nPermutations;

	std::iota(this->permutations.begin(), endSequence, 0);
	std::shuffle(this->permutations.begin(), endSequence, this->rngSeed);
	std::copy(this->permutations.begin(), endSequence, endSequence);
}

float NoiseGenerator::perlinValue2D( float x, float y ) const noexcept
{
	x *= this->noiseScalar;
	y *= this->noiseScalar;

	float perlinValue = this->_perlinValue2D(x, y);

	return (perlinValue + 1.0f) * 0.5f;
}

float NoiseGenerator::perlinValue3D( float x, float y, float z ) const noexcept
{
	x *= this->noiseScalar;
	y *= this->noiseScalar;
	z *= this->noiseScalar;

	float perlinValue = this->_perlinValue3D(x, y, z);

	return (perlinValue + 1.0f) * 0.5f;
}

float NoiseGenerator::octavePerlin2D(float x, float y, ui32 octaves) const noexcept
{
	float octavePerlin = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxValue  = 0.0f;

	x *= this->noiseScalar;
	y *= this->noiseScalar;

	for (ui32 i = 0; i < octaves; i++)
    {
        octavePerlin += _perlinValue2D(x * frequency, y * frequency) * amplitude;
        maxValue += amplitude;
        frequency *= NoiseGenerator::LACUNARITY;
        amplitude *= NoiseGenerator::PERSISTANCE;
    }

	octavePerlin /= maxValue;
    return (octavePerlin + 1.0f) * 0.5f;
}

float NoiseGenerator::octavePerlin3D(float x, float y, float z, ui32 octaves) const noexcept
{
	float octavePerlin = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxValue  = 0.0f;

	x *= this->noiseScalar;
	y *= this->noiseScalar;
	z *= this->noiseScalar;

    for (ui32 i = 0; i < octaves; i++)
    {
        octavePerlin += _perlinValue3D(x * frequency, y * frequency, z * frequency) * amplitude;
        maxValue += amplitude;
        frequency *= NoiseGenerator::LACUNARITY;
        amplitude *= NoiseGenerator::PERSISTANCE;
    }

	octavePerlin /= maxValue;
    return (octavePerlin + 1.0f) * 0.5f;
}

float NoiseGenerator::_perlinValue2D( float x, float y ) const noexcept
{
	ui32 Xi =  positiveModulo(static_cast<i32>(std::floor(x)), this->nPermutations);
	ui32 Yi =  positiveModulo(static_cast<i32>(std::floor(y)), this->nPermutations);

	float xf = x - std::floor(x);
	float yf = y - std::floor(y);

	vec2 topRight{xf - 1.0f, yf - 1.0f};
	vec2 topLeft{xf, yf - 1.0f};
	vec2 bottomLeft{xf, yf};
	vec2 bottomRight{xf - 1.0f, yf};

	i32 permTopRight = this->permutations[this->permutations[Xi + 1] + Yi + 1];
	i32 permTopLeft = this->permutations[this->permutations[Xi] + Yi + 1];
	i32 permBottomLeft = this->permutations[this->permutations[Xi] + Yi];
	i32 permBottomRight = this->permutations[this->permutations[Xi + 1] + Yi];

	float dotTopRight = vec2::dot(topRight, this->getGradient2D(permTopRight));
	float dotTopLeft = vec2::dot(topLeft, this->getGradient2D(permTopLeft));
	float dotBottomLeft = vec2::dot(bottomLeft, this->getGradient2D(permBottomLeft));
	float dotBottomRight = vec2::dot(bottomRight, this->getGradient2D(permBottomRight));

	float u = this->smooth(xf);
	float v = this->smooth(yf);

	return this->lerp(
		u,
		this->lerp(v, dotBottomLeft, dotTopLeft),
		this->lerp(v, dotBottomRight, dotTopRight)
	);
}

float NoiseGenerator::_perlinValue3D( float x, float y, float z ) const noexcept
{
	ui32 Xi =  positiveModulo(static_cast<i32>(std::floor(x)), this->nPermutations);
	ui32 Yi =  positiveModulo(static_cast<i32>(std::floor(y)), this->nPermutations);
	ui32 Zi =  positiveModulo(static_cast<i32>(std::floor(z)), this->nPermutations);

	float xf = x - std::floor(x);
	float yf = y - std::floor(y);
	float zf = z - std::floor(z);

	i32 p000 = permutations[permutations[permutations[Xi]     + Yi]     + Zi];
	i32 p001 = permutations[permutations[permutations[Xi]     + Yi]     + Zi + 1];
	i32 p010 = permutations[permutations[permutations[Xi]     + Yi + 1] + Zi];
	i32 p011 = permutations[permutations[permutations[Xi]     + Yi + 1] + Zi + 1];
	i32 p100 = permutations[permutations[permutations[Xi + 1] + Yi]     + Zi];
	i32 p101 = permutations[permutations[permutations[Xi + 1] + Yi]     + Zi + 1];
	i32 p110 = permutations[permutations[permutations[Xi + 1] + Yi + 1] + Zi];
	i32 p111 = permutations[permutations[permutations[Xi + 1] + Yi + 1] + Zi + 1];

	float d000 = this->getGradient3D(p000, xf,        yf,        zf);
	float d001 = this->getGradient3D(p001, xf,        yf,        zf - 1.0f);
	float d010 = this->getGradient3D(p010, xf,        yf - 1.0f, zf);
	float d011 = this->getGradient3D(p011, xf,        yf - 1.0f, zf - 1.0f);
	float d100 = this->getGradient3D(p100, xf - 1.0f, yf,        zf);
	float d101 = this->getGradient3D(p101, xf - 1.0f, yf,        zf - 1.0f);
	float d110 = this->getGradient3D(p110, xf - 1.0f, yf - 1.0f, zf);
	float d111 = this->getGradient3D(p111, xf - 1.0f, yf - 1.0f, zf - 1.0f);

	float u = this->smooth(xf);
	float v = this->smooth(yf);
	float w = this->smooth(zf);

	return this->lerp(w,
		this->lerp(v,
			this->lerp(u, d000, d100),
			this->lerp(u, d010, d110)
		),
		this->lerp(v,
			this->lerp(u, d001, d101),
			this->lerp(u, d011, d111)
		)
	);
}

float NoiseGenerator::lerp(float t, float m1, float m2) const noexcept
{
	return m1 + t * (m2 - m1);
}

float NoiseGenerator::smooth(float t) const noexcept
{
	return ((6 * t - 15) * t + 10) * t * t * t;
}

vec2 const&	NoiseGenerator::getGradient2D( i32 input ) const noexcept
{
	return this->gradients2D[input % this->gradients2D.size()];
}

float NoiseGenerator::getGradient3D( ui32 hash, float x, float y, float z ) const noexcept
{
	switch (hash & 0xF)
	{
		case 0x0: return  x + y;
		case 0x1: return -x + y;
		case 0x2: return  x - y;
		case 0x3: return -x - y;
		case 0x4: return  x + z;
		case 0x5: return -x + z;
		case 0x6: return  x - z;
		case 0x7: return -x - z;
		case 0x8: return  y + z;
		case 0x9: return -y + z;
		case 0xA: return  y - z;
		case 0xB: return -y - z;
		case 0xC: return  y + x;
		case 0xD: return -y + z;
		case 0xE: return  y - x;
		case 0xF: return -y - z;
		default:  return 0;
	}
}

}	// namespace vox
