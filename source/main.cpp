#include "Vox.hpp"

#include <iostream>
#include <stdexcept>
#include <thread>

i32	main( void )
{
	try
	{
		vox::Vox app;

		app.setupVulkan();
		app.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
