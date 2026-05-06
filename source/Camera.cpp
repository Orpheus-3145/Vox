#include "Camera.hpp"

#include <cassert>
#include <cmath>
#include <iostream>

namespace vox {

mat4 Camera::getProjectionMatrix( bool columnMajor ) const noexcept
{
	float fov = CameraSettings::projectionFov;
	if ( fov > M_2_PI or fov < -M_2_PI )
		fov = radians(fov);
	float near = CameraSettings::projectionNear;
	float far = CameraSettings::projectionFar;
	const float tanHalfFovy = std::tan(fov / 2.f);

	if (columnMajor == true)
	{
		return mat4{
			{1.f / (this->aspect * tanHalfFovy),  0.0f,                0.0f,                          0.0f},
			{0.0f,                                -1.f / tanHalfFovy,  0.0f,                          0.0f},
			{0.0f,                                0.0f,                far / (far - near),            1.0f},
			{0.0f,                                0.0f,                -(far * near) / (far - near),  0.0f}
		};
	}
	else
	{
		return mat4{
			{1.f / (this->aspect * tanHalfFovy),  0.0f,                0.0f,                0.0f                        },
			{0.0f,                                -1.f / tanHalfFovy,  0.0f,                0.0f                        },
			{0.0f,                                0.0f,                far / (far - near),  -(far * near) / (far - near)},
			{0.0f,                                0.0f,                1.0f,                0.0f                        }
		};
	}
}

mat4 Camera::getViewMatrix( bool columnMajor ) const noexcept
{
	float dotLeftPos = vec3::dot(this->cameraRight, this->position);
	float dotUpPos = vec3::dot(this->cameraUp, this->position);
	float dotForwardPos = vec3::dot(this->cameraForward, this->position);

	if (columnMajor == true)
	{
		return mat4{
			{this->cameraRight.x,  this->cameraUp.x,  -this->cameraForward.x,  0.0f},
			{this->cameraRight.y,  this->cameraUp.y,  -this->cameraForward.y,  0.0f},
			{this->cameraRight.z,  this->cameraUp.z,  -this->cameraForward.z,  0.0f},
			{-dotLeftPos,          -dotUpPos,         dotForwardPos,           1.0f}
		};
	}
	else
	{
		return mat4{
			{this->cameraRight.x,     this->cameraRight.y,     this->cameraRight.z,     -dotLeftPos},
			{this->cameraUp.x,        this->cameraUp.y,        this->cameraUp.z,        -dotUpPos},
			{-this->cameraForward.x,  -this->cameraForward.y,  -this->cameraForward.z,  dotForwardPos},
			{0.0f,                    0.0f,                    0.0f,                    1.0f}
		};
	}
}

mat4 Camera::getViewMatrixNoTranslation( bool columnMajor ) const noexcept
{
	if (columnMajor == true)
	{
		return mat4{
			{this->cameraRight.x,  this->cameraUp.x,  -this->cameraForward.x,  0.0f},
			{this->cameraRight.y,  this->cameraUp.y,  -this->cameraForward.y,  0.0f},
			{this->cameraRight.z,  this->cameraUp.z,  -this->cameraForward.z,  0.0f},
			{0.0f,                 0.0f,              0.0f,                    1.0f}
		};
	}
	else
	{
		return mat4{
			{this->cameraRight.x,     this->cameraRight.y,     this->cameraRight.z,     0.0f},
			{this->cameraUp.x,        this->cameraUp.y,        this->cameraUp.z,        0.0f},
			{-this->cameraForward.x,  -this->cameraForward.y,  -this->cameraForward.z,  0.0f},
			{0.0f,                    0.0f,                    0.0f,                    1.0f}
		};
	}
}

vec3 const& Camera::getCameraPos( void ) const noexcept
{
	return this->position;
}

void Camera::moveForward( float delta ) noexcept
{
	this->position -= this->cameraForward * delta;
	this->updateCameraAxis();
}

void Camera::moveBackward( float delta ) noexcept
{
	this->position += this->cameraForward * delta;
	this->updateCameraAxis();
}

void Camera::moveRight( float delta ) noexcept
{
	this->position += this->cameraRight * delta;
	this->updateCameraAxis();
}

void Camera::moveLeft( float delta ) noexcept
{
	this->position -= this->cameraRight * delta;
	this->updateCameraAxis();
}

void Camera::moveUp( float delta ) noexcept
{
	this->position += this->cameraUp * delta;
	this->updateCameraAxis();
}

void Camera::moveDown( float delta ) noexcept
{
	this->position -= this->cameraUp * delta;
	this->updateCameraAxis();
}

void Camera::move( const vec3& direction ) noexcept
{
	vec3 right = this->cameraRight * direction.x;
	vec3 up = this->cameraUp * direction.y;
	vec3 forward = this->cameraForward * direction.z;

	this->position += up + right + forward;
}

void Camera::rotate( float pitch, float yaw, float roll ) noexcept
{
	if (this->currentPitch + pitch > 89.0f)
	{
		pitch = 89.0f - this->currentPitch;
		this->currentPitch = 89.0f;
	}
	else if (this->currentPitch + pitch < -89.0f)
	{
		pitch = -89.0f - this->currentPitch;
		this->currentPitch = -89.0f;
	} else
	{
		this->currentPitch += pitch;
	}

	yaw = radians(yaw / 2.0f);
	pitch = radians(pitch / 2.0f);
	roll = radians(roll / 2.0f);

	quat qYaw(-yaw, this->_up);
	quat qPitch(pitch, this->cameraRight);
	quat qRoll(roll, this->cameraForward);

	quat qForward{0.0f, this->forward.x, this->forward.y, this->forward.z};
	// apply yaw rotation, forward rotates horizontally
	quat qForwardRotated = quat::product(quat::product(qYaw, qForward), qYaw.conjugated());
	// apply pitch rotation, forward rotates vertically
	qForwardRotated = quat::product(quat::product(qPitch, qForwardRotated), qPitch.conjugated());
	this->forward = qForwardRotated.normalized().vector();

	if (roll != 0.0f)
	{
		// apply roll rotation, (global) up rotates
		quat qUp{0.0f, this->_up.x, this->_up.y, this->_up.z};
		quat qUpRotated = quat::product(quat::product(qRoll, qUp), qRoll.conjugated());
		this->_up = qUpRotated.normalized().vector();
	}
	this->updateCameraAxis();
}

void Camera::updateAspect( float aspect ) noexcept
{
	assert(std::abs(this->aspect - epsilon()) > 0.0f);
	this->aspect = aspect;
}

void Camera::updateCameraAxis( void ) noexcept
{
	this->cameraForward = this->forward.normalize();
	this->cameraRight = vec3::cross(this->_up, this->cameraForward).normalize();
	this->cameraUp = vec3::cross(this->cameraForward, this->cameraRight).normalize();
}

}	// namespace vox