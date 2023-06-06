#include <iostream>
#include "examples/PBRBasic.h"

const static uint32_t WIDTH = 1920, HEIGHT = 1080;

int main()
{
	PBRBasic app(WIDTH, HEIGHT, "vulkan");
	try
	{
		app.Run();
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
}