#include "KeyboardInput.hpp"


namespace vox {

KeyboardInput::KeyboardInput() noexcept
{
	this->reset();
	keysPressed.fill(false);
}

void	KeyboardInput::reset() noexcept
{
	keyStates.fill(false);
	// keysPressed.fill(false);
	keysReleased.fill(false);
}

}	// namespace vox
