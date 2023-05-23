#pragma once
#include <GLFW/glfw3.h>

class Window
{
public:
	Window(int width, int height, const char* title);
	~Window();
	bool ShouldClose();
	void PollEvents();
	GLFWwindow* GetNativeWindow() { return m_NativeWindow; }
	bool GetWindowResized() const { return m_WindowResize; }
	void SetWindowResized(bool resized) { m_WindowResize = resized; }
	void GetFrameBufferSize(int* width, int* height);
private:
	GLFWwindow* m_NativeWindow = nullptr;
	bool m_WindowResize = false;
};