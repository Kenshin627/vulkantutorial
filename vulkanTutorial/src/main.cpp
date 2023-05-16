#include <iostream>
#include "Application.h"
int main()
{
	Application app;
	try
	{
		app.Run();
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
}