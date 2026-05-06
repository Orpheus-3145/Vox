#pragma once

#include "Vectors.hpp"

namespace vox {

struct CameraSettings {
	static constexpr float	projectionFov = 50.0f;
	static constexpr float	projectionNear = .1f;
	static constexpr float	projectionFar = 1000.0f;
	static constexpr float	cameraDistance = 50.0f;
	static constexpr float	cameraSensitivity = 0.1f;
	// the up depends on what axis the camera is watching (so it depends on forward): 
	// camera looks (forward) along x or z -> __up is non-null in y, 
	// camera looks (forward) along y -> __up is non-null in z
	static constexpr vec3	cameraDefaultUp{0.0f, 1.0f, 0.0f};
};


class Camera
{
	public:
		Camera( vec3 const& pos, vec3 const& forward, float aspect ) : 
			position(pos),
			forward(forward),
			aspect(aspect) { this->updateCameraAxis(); };

		mat4	getProjectionMatrix( bool columnMajor = true ) const noexcept;
		mat4	getViewMatrix( bool columnMajor = true ) const noexcept;
		mat4	getViewMatrixNoTranslation( bool columnMajor = true ) const noexcept;

		vec3 const&	getCameraPos( void ) const noexcept;

		void	moveForward( float ) noexcept;
		void	moveBackward( float ) noexcept;
		void	moveRight( float ) noexcept;
		void	moveLeft( float ) noexcept;
		void	moveUp( float ) noexcept;
		void	moveDown( float ) noexcept;
		void	move(const vec3& direction) noexcept;
		void	rotate( float, float, float ) noexcept;
		void	updateAspect( float ) noexcept;

	private:
		void	updateCameraAxis( void ) noexcept;

		vec3	position;										// position of the camera
		vec3	forward;										// where the camera is looking at
		float	aspect;											// ratio screen width / height
		vec3	_up{CameraSettings::cameraDefaultUp};			// general up, stored in a variabile since it can change due to roll rotations
		vec3	cameraForward;									// z axis of the camera
		vec3	cameraRight;									// x axis of the camera
		vec3	cameraUp;										// y axis of the camera

		float	currentPitch = 0.0f;	// to avoid vertical rotations > 90° or < -90°
};

}	// namespace vox