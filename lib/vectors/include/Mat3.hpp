#pragma once

#include <array>
#include <ostream>

class vec3;

class mat4;

class mat3
{
	public:

	static mat3	idMat( void );

	mat3() = default;
	mat3(float diagonal);
	mat3(const vec3& row0,
		 const vec3& row1,
		 const vec3& row2);
	mat3(const mat4& matrix4x4);
	mat3(std::initializer_list<std::initializer_list<float>> rows);
	~mat3() = default;
	mat3(const mat3& other) = default;
	mat3&	operator=(const mat3& other) = default;

	mat3	operator*(const mat3& other) const;
	mat3&	operator*=(const mat3& other) {	*this = *this * other; return *this;}

	float*			operator[](int row) noexcept { return data[row]; }
	const float*	operator[](int row) const noexcept { return data[row]; }

	float	data[3][3];

	private:
};

std::ostream&	operator<<(std::ostream& os, const mat3& matrix);