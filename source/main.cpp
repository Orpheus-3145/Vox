#include <iostream>
#include <stdexcept>
#include <thread>
#include <cmath>

#include "Vox.hpp"


i32	main( void )
{
	try
	{
		vox::Vox app;

		app.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
