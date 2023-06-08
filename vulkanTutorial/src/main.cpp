#include <iostream>
#include "examples/PBRModel.h"

const static uint32_t WIDTH = 1920, HEIGHT = 1080;

int main()
{
	PBRModel app(WIDTH, HEIGHT, "vulkan");
	try
	{
		app.Run();
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
}