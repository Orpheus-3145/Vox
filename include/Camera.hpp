#pragma once

#include "Vectors.hpp"
#include "Vulkan.hpp"


namespace vox {

struct CameraSettings {
	static constexpr float	projectionFov{80.0f};
	static constexpr float	projectionNear{.1f};
	static constexpr float	projectionFar{1000.0f};
	static constexpr float	cameraDistance{50.0f};
	static constexpr float	cameraSensitivity{0.1f};
	static constexpr vec3	cameraForward{0.0f, 0.0f, 10.0f};
	static constexpr vec3	cameraUpDefault{0.0f, 1.0f, 0.0f};
};

using WindowSize = VkExtent2D;

class Camera
{
	public:
		Camera( vec3 const& pos, vec3 const& forward, WindowSize const& size ) : 
			position(pos),
			forward(forward),
			size(size) { this->updateCameraAxis(); };

		mat4	getOrthographicMatrix( bool columnMajor = true ) const noexcept;
		mat4	getProjectionMatrix( bool columnMajor = true ) const noexcept;
		mat4	getViewMatrix( bool columnMajor = true ) const noexcept;
		mat4	getViewMatrixNoTranslation( bool columnMajor = true ) const noexcept;

		vec3 const&	getCameraPos( void ) const noexcept;
		std::string	formatCameraPos( void ) const noexcept;

		void	moveForward( float ) noexcept;
		void	moveBackward( float ) noexcept;
		void	moveRight( float ) noexcept;
		void	moveLeft( float ) noexcept;
		void	moveUp( float ) noexcept;
		void	moveDown( float ) noexcept;
		void	move(const vec3& direction) noexcept;
		void	rotate( float, float, float ) noexcept;

		vec3	getRelativeMoveDirection(const vec3& rawDirection) const noexcept;
		void	updateWindowSize( ui32 width, ui32 height ) noexcept;

	private:
		void	updateCameraAxis( void ) noexcept;

		vec3		position;								// position of the camera
		vec3		forward;								// where the camera is looking at
		WindowSize	size;									// size of the window
		vec3		_up{CameraSettings::cameraUpDefault};	// general up, stored in a variabile since it can change due to roll rotations
		vec3		cameraForward;							// z axis of the camera
		vec3		cameraRight;							// x axis of the camera
		vec3		cameraUp;								// y axis of the camera

		float		currentPitch{0.0f};	// to avoid vertical rotations > 90° or < -90°
};

}	// namespace vox
