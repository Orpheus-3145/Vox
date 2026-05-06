#pragma once

#include <array>
#include <ostream>

class vec3;
class vec4;
class mat3;

class mat4
{
	public:

	static mat4	idMat( void );
	static mat4	transMat( vec3 const& );
	static mat4	scaleMat( vec3 const& );
	static mat4	rotationMat( float, vec3 const& );

	mat4();
	mat4(float diagonal);
	mat4(const vec4& col0,
		 const vec4& col1,
		 const vec4& col2,
		 const vec4& col3);
	mat4(std::initializer_list<std::initializer_list<float>> rows);
	mat4(const mat3& matrix3x3);
	~mat4() = default;
	mat4(const mat4& other);
	mat4&	operator=(const mat4& other);

	mat4	operator*(const mat4& other) const;
	mat4&	operator*=(const mat4& other);
	vec4	operator*(vec4 const&) const;

	mat4&	transpose() noexcept;
	mat4	transposed() const noexcept;
	mat4&	translate(const vec3& translation) noexcept;
	mat4	translated(const vec3& translation) const noexcept;
	mat4&	rotate(float angleRadians, const vec3& axis) noexcept;
	mat4	rotated(float angleRadians, const vec3& axis) const noexcept;

	float*			operator[](int row) noexcept { return data[row]; }
	const float*	operator[](int row) const noexcept { return data[row]; }

	float	data[4][4];	
};

std::ostream&	operator<<(std::ostream& os, const mat4& matrix);