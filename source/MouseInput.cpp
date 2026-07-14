#include "MouseInput.hpp"


namespace vox {

void	MouseInput::reset() noexcept
{
	mouseStates.fill(false);
	buttonsPressed.fill(false);
	buttonsReleased.fill(false);
}

}	// namespace vox
