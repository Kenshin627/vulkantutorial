#pragma once
#include "KeyCode.h"
#include "MouseCode.h"
#include "../AppBase.h"
#include <GLFW/glfw3.h>
#include <utility>


class WindowsInput
{
public:
	inline static bool IsKeyPressed(KeyCode key) 
	{
		int state = glfwGetKey(static_cast<GLFWwindow*>(AppBase::Get().GetWindow().GetNativeWindow()), key);
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	};
	inline static bool IsMousePressed(MouseCode button) 
	{ 
		int state = glfwGetMouseButton(static_cast<GLFWwindow*>(AppBase::Get().GetWindow().GetNativeWindow()), button);
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	};
	inline static std::pair<float, float> GetMousePos() 
	{
		double x, y;
		glfwGetCursorPos(static_cast<GLFWwindow*>(AppBase::Get().GetWindow().GetNativeWindow()), &x, &y);
		return std::make_pair(float(x), float(y));
	};
	inline static float GetMouseX() 
	{
		auto [x, y] = GetMousePos();
		return x;
	};
	inline static float GetMouseY() 
	{ 
		auto [x, y] = GetMousePos();
		return y;
	};
};
