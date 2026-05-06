#include "Quat.hpp"

quat::quat(float angle, const vec3& v3)
{
	w = std::cos(angle);
	x = v3.x * std::sin(angle);
	y = v3.y * std::sin(angle);
	z = v3.z * std::sin(angle);
}

quat&	quat::operator=(const quat& other)
{
	if (this != &other)
	{
		w = other.w;
		x = other.x;
		y = other.y;
		z = other.z;
	}
	return *this;
}

vec3	quat::vector() const noexcept {
	return vec3{this->x, this->y, this->z};
}

quat	quat::clone() const noexcept
{
	return quat(w, x, y, z);
}

quat&	quat::conjugate() noexcept
{
	x = -x;
	y = -y;
	z = -z;
	return *this;
}

quat	quat::conjugated() const noexcept
{
	return this->clone().conjugate();
}

quat&	quat::normalize() noexcept
{
	float len = std::sqrt(w * w + x * x + y * y + z * z);

	if (len > 0.0f)
	{
		float invLen = 1.0f / len;

		w *= invLen;
		x *= invLen;
		y *= invLen;
		z *= invLen;
	}
	return *this;
}

quat	quat::normalized() const noexcept
{
	return this->clone().normalize();
}

quat&	quat::fastNormalize() noexcept
{
	float lenSq = w * w + x * x + y * y + z * z;

	if (lenSq > 0.0f)
	{
		float invSqrtLen = fastInverseSqrt(lenSq);

		w *= invSqrtLen;
		x *= invSqrtLen;
		y *= invSqrtLen;
		z *= invSqrtLen;
	}
	return *this;
}

quat	quat::fastNormalized() const noexcept
{
	return this->clone().fastNormalize();
}

mat4 quat::getMatrix(bool columnMajor) const noexcept
{
	const float x2  = x + x;
	const float y2  = y + y;
	const float z2  = z + z;
	const float xx2 = x * x2;
	const float xy2 = x * y2;
	const float xz2 = x * z2;
	const float yy2 = y * y2;
	const float yz2 = y * z2;
	const float zz2 = z * z2;
	const float sx2 = w * x2;
	const float sy2 = w * y2;
	const float sz2 = w * z2;

	if (columnMajor == true)
	{
		return mat4{{1 - (yy2 + zz2),  xy2 + sz2,        xz2 - sy2,        0},
					{xy2 - sz2,        1 - (xx2 + zz2),  yz2 + sx2,        0},
					{xz2 + sy2,        yz2 - sx2,        1 - (xx2 + yy2),  0},
					{0,                0,                0,                1}};
	}
	else
	{
		return mat4{{1 - (yy2 + zz2),  xy2 - sz2,        xz2 + sy2,        0},
					{xy2 + sz2,        1 - (xx2 + zz2),  yz2 + sx2,        0},
					{xz2 - sy2,        yz2 + sx2,        1 - (xx2 + yy2),  0},
					{0,                0,                0,                1}};
	}
}

quat	quat::product(const quat& a, const quat& b) noexcept
{
	return quat(
		a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
		a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
		a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
		a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w
	);
}

vec3	quat::rotated(const vec3& rotateAround, quat rotationQuat) noexcept
{
	quat	vectorQuat(0.0f, rotateAround.x, rotateAround.y, rotateAround.z);
	rotationQuat.normalize();
	quat	resultQuat = quat::product(quat::product(rotationQuat, vectorQuat), rotationQuat.conjugated());

	return vec3(resultQuat.x, resultQuat.y, resultQuat.z);
}

std::ostream&	operator<<(std::ostream& os, const quat& q) noexcept
{
	os << "Quat(w: " << q.w << ", x: " << q.x << ", y: " << q.y << ", z: " << q.z << ")";
	return os;
}