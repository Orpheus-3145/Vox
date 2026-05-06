#include "KeyboardInput.hpp"


namespace vox {

/**
 * Zero all data
 */
KeyboardInput::KeyboardInput() noexcept
{
	this->reset();
	keysPressed.fill(false);
}

/**
 * Zero all data
 */
void	KeyboardInput::reset() noexcept
{
	keyStates.fill(false);
	// keysPressed.fill(false);
	keysReleased.fill(false);
}

}	// namespace vox
