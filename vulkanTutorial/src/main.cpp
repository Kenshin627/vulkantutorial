#include <iostream>
#include "examples/GlTFApp.h"
//#include "examples/RGBSpliter2Pass.h"

const static uint32_t WIDTH = 1024, HEIGHT = 728;

int main()
{
	//RGBSpliter2Pass app(WIDTH, HEIGHT, "RGBSpliter");
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