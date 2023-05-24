#include "Window.h"
#include <stdexcept>


void FrameBufferResize(GLFWwindow* window, int width, int height)
{
	Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
	win->SetWindowResized(true);
}

Window::Window(int width, int height, const char* title)
{
	if (glfwInit() != GLFW_TRUE)
	{
		throw std::runtime_error("glfw init failed!");
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	m_NativeWindow = glfwCreateWindow(width, height, title, nullptr, nullptr);
	glfwSetWindowUserPointer(m_NativeWindow, this);
	glfwSetFramebufferSizeCallback(m_NativeWindow, FrameBufferResize);
}

Window::~Window()
{
	glfwDestroyWindow(m_NativeWindow);
	glfwTerminate();
}

bool Window::ShouldClose()
{
	return glfwWindowShouldClose(m_NativeWindow);
}

void Window::PollEvents()
{
	glfwPollEvents();
}

void Window::GetFrameBufferSize(int* width, int* height) const
{
	glfwGetFramebufferSize(m_NativeWindow, width, height);
}
