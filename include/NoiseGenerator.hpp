#pragma once

#include <array>
#include <cstdint>
#include <random>

#include "Vectors.hpp"
#include "TypeAliases.hpp"


namespace vox {

class NoiseGenerator
{
	public:
		NoiseGenerator( void ) = delete;
		NoiseGenerator( ui32 baseSeed, ui32 permSize );

		float	perlinValue2D( float x, float y ) const noexcept;
		float	perlinValueSimple3D( float x, float y, float z ) const noexcept;
		float	perlinValue3D( float x, float y, float z ) const noexcept;

		float	octavePerlin2D( float x, float y, ui32 octaves = NoiseGenerator::N_OCTAVES) const noexcept;
		float	octavePerlinSimple3D( float x, float y, float z, ui32 octaves = NoiseGenerator::N_OCTAVES) const noexcept;
		float	octavePerlin3D( float x, float y, float z, ui32 octaves = NoiseGenerator::N_OCTAVES) const noexcept;

		void	setSeed( ui32 baseSeed ) noexcept;

	private:
		float	_perlinValue2D( float x, float y ) const noexcept;
		float	_perlinValueSimple3D( float x, float y, float z ) const noexcept;
		float	_perlinValue3D( float x, float y, float z ) const noexcept;

		void		setPermutations( void ) noexcept;
		float		lerp( float t, float m1, float m2 ) const noexcept;
		float		smooth( float t ) const noexcept;

		vec2 const&	getGradient2D( i32 input ) const noexcept;
		vec3 const&	getGradient3DSimple( i32 input ) const noexcept;
		float		getGradient3D( ui32 hash, float x, float y, float z ) const noexcept;

		static constexpr ui32	GOLDEN_RATIO_HASH = 2654435761U;
		static constexpr ui32	N_FEATURES = 4U;
		static constexpr ui32	N_OCTAVES = 6U;
		static constexpr float	LACUNARITY = 2.0f;
		static constexpr float	PERSISTANCE = 0.5f;

		static constexpr std::array<vec2,4> gradients2D{
			vec2{ 1.0f,  1.0f},
			vec2{-1.0f,  1.0f},
			vec2{-1.0f, -1.0f},
			vec2{ 1.0f, -1.0f}
		};
		static constexpr std::array<vec3,12> gradients3D{
			vec3{1.0f,  1.0f,  0.0f}, vec3{-1.0f,  1.0f,  0.0f},
			vec3{1.0f, -1.0f,  0.0f}, vec3{-1.0f, -1.0f,  0.0f},
			vec3{1.0f,  0.0f,  1.0f}, vec3{-1.0f,  0.0f,  1.0f},
			vec3{1.0f,  0.0f, -1.0f}, vec3{-1.0f,  0.0f, -1.0f},
			vec3{0.0f,  1.0f,  1.0f}, vec3{ 0.0f, -1.0f,  1.0f},
			vec3{0.0f,  1.0f, -1.0f}, vec3{ 0.0f, -1.0f, -1.0f}
		};

		float				noiseScalar;
		ui32				nPermutations;
		std::vector<ui32>	permutations;

		ui32			terrainSeed;
		ui32			cavesSeed;
		std::mt19937	rngSeed;
};

}	// namespace vox
