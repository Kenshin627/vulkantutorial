#include <iostream>
#include "GlTFApp.h"

const static uint32_t WIDTH = 1024, HEIGHT = 728;

int main()
{
	GLTFApp app(WIDTH, HEIGHT, "vulkan");
	try
	{
		app.Run();
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
}