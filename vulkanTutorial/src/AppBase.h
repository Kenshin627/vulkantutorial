#pragma once
#include "Window.h"

class AppBase
{
public:
	AppBase(int width, int height, const char* title);
	void InitWindow(int width, int height, const char* title);
	virtual~AppBase() = default;
	virtual void RebuildFrameBuffer() = 0;
	virtual void CreateSetLayout() = 0;
	static AppBase& Get() { return *m_Instance; }
	Window& GetWindow() { return m_Window; }
protected:
	Window m_Window;
private:
	static AppBase* m_Instance;
};