#include "Mat3.hpp"
#include "Mat4.hpp"
#include "Vec3.hpp"
#include "Vec4.hpp"

#include <cmath>
#include <cstring>


mat4	mat4::idMat( void )
{
	mat4	id(0.0f);

	id.data[0][0] = 1.0f;
	id.data[1][1] = 1.0f;
	id.data[2][2] = 1.0f;
	id.data[3][3] = 1.0f;
	return id;
}

mat4	mat4::transMat( vec3 const& transVect )
{
	mat4	trans = idMat();

	trans.data[0][3] = transVect.x;
	trans.data[1][3] = transVect.y;
	trans.data[2][3] = transVect.z;
	return trans;
}

mat4	mat4::scaleMat( vec3 const& scaleVect )
{
	mat4	scale = idMat();

	scale.data[0][0] = scaleVect.x;
	scale.data[1][1] = scaleVect.y;
	scale.data[2][2] = scaleVect.z;
	scale.data[3][3] = 1.0f;
	return scale;
}

mat4	mat4::rotationMat( float angleRadians, vec3 const& axis )
{

	mat4		rotation(0.0f);
	const float	cosAngle = std::cos(angleRadians);
	const float	sinAngle = std::sin(angleRadians);

	rotation.data[0][0] = cosAngle + axis.x * axis.x * (1 - cosAngle);
	rotation.data[0][1] = axis.x * axis.y * (1 - cosAngle) - axis.z * sinAngle;
	rotation.data[0][2] = axis.x * axis.z * (1 - cosAngle) + axis.y * sinAngle;
	
	rotation.data[1][0] = axis.y * axis.x * (1 - cosAngle) + axis.z * sinAngle;
	rotation.data[1][1] = cosAngle + axis.y * axis.y * (1 - cosAngle);
	rotation.data[1][2] = axis.y * axis.z * (1 - cosAngle) - axis.x * sinAngle;
	
	rotation.data[2][0] = axis.z * axis.x * (1 - cosAngle) - axis.y * sinAngle;
	rotation.data[2][1] = axis.z * axis.y * (1 - cosAngle) + axis.x * sinAngle;
	rotation.data[2][2] = cosAngle + axis.z * axis.z * (1 - cosAngle);
	return rotation;
}


mat4::mat4()
{
	std::memset(data, 0, sizeof(data));
}

mat4::mat4(float value)
{
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			if (i == j)
			{
				data[j][i] = value;
			}
			else
			{
				data[j][i] = 0.0f;
			}
		}
	}
}

mat4::mat4(const mat3& matrix3x3)
{
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			data[i][j] = matrix3x3[i][j];
		}
	}
	data[0][3] = 0.0f;
	data[1][3] = 0.0f;
	data[2][3] = 0.0f;
	data[3][0] = 0.0f;
	data[3][1] = 0.0f;
	data[3][2] = 0.0f;
	data[3][3] = 1.0f;
}

mat4::mat4(const vec4& row0,
	const vec4& row1, 
	const vec4& row2,
	const vec4& row3)
{
	data[0][0] = row0.x; data[0][1] = row0.y; data[0][2] = row0.z; data[0][3] = row0.w;
	data[1][0] = row1.x; data[1][1] = row1.y; data[1][2] = row1.z; data[1][3] = row1.w;
	data[2][0] = row2.x; data[2][1] = row2.y; data[2][2] = row2.z; data[2][3] = row2.w;
	data[3][0] = row3.x; data[3][1] = row3.y; data[3][2] = row3.z; data[3][3] = row3.w;
}

mat4::mat4(std::initializer_list<std::initializer_list<float>> rows)
{
	int	i = 0;
	int	j;

	for (const std::initializer_list<float>& row : rows)
	{
		j = 0;
		for (float val : row)
		{
			if (i < 4 && j < 4)
			{
				data[i][j] = val;
			}
			j++;
		}
		i++;
	}
}

mat4::mat4(const mat4& other)
{
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			data[j][i] = other.data[j][i];
		}
	}
}

mat4&	mat4::operator=(const mat4& other)
{
	if (this != &other)
	{
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				data[j][i] = other.data[j][i];
			}
		}
	}
	return *this;
}

mat4	mat4::operator*(const mat4& other) const
{
	mat4	result(0.0f);

	for (int row = 0; row < 4; row++)
	{
		for (int col = 0; col < 4; col++)
		{
			for (int k = 0; k < 4; k++)
			{
				result.data[row][col] += data[k][col] * other.data[row][k];
			}
		}
	}
	return result;
}

mat4&	mat4::operator*=(const mat4& other)
{
	*this = *this * other;
	return *this;
}

vec4	mat4::operator*( vec4 const& v ) const {
	return vec4{
		(*this)[0][0] * v.x + (*this)[0][1] * v.y + (*this)[0][2] * v.z + (*this)[0][3] * v.w,
		(*this)[1][0] * v.x + (*this)[1][1] * v.y + (*this)[1][2] * v.z + (*this)[1][3] * v.w,
		(*this)[2][0] * v.x + (*this)[2][1] * v.y + (*this)[2][2] * v.z + (*this)[2][3] * v.w,
		(*this)[3][0] * v.x + (*this)[3][1] * v.y + (*this)[3][2] * v.z + (*this)[3][3] * v.w,
	};
}

mat4&	mat4::transpose() noexcept {
	for (int row = 0; row < 4; row++) {
		for (int col = row + 1; col < 4; col++)
			std::swap((*this)[row][col], (*this)[col][row]);
	}
	return *this;
}

mat4	mat4::transposed() const noexcept {
	mat4 transposed = *this;
	return transposed.transpose();
}

mat4&	mat4::translate(const vec3& translation) noexcept
{
	*this *= mat4::transMat(translation);
	return *this;
}

mat4	mat4::translated(const vec3& translation) const noexcept
{
	return (*this) * mat4::transMat(translation);
}


mat4&	mat4::rotate(float angleRadians, const vec3& axis) noexcept
{
	*this *= mat4::rotationMat(angleRadians, axis);
	return *this;
}

mat4	mat4::rotated(float angleRadians, const vec3& axis) const noexcept
{
	return (*this) * mat4::rotationMat(angleRadians, axis);
}

std::ostream&	operator<<(std::ostream& os, const mat4& matrix)
{
	for (int row = 0; row < 4; row++)
	{
		os << "[";
		for (int col = 0; col < 4; col++)
		{
			os << matrix.data[row][col] << "]";
			if (col < 3)
			{
				os << " [";
			}
		}
		os << "\n";
	}
	return os;
}