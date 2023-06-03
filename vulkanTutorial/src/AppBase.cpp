#include "AppBase.h"

AppBase* AppBase::m_Instance = nullptr;

AppBase::AppBase(int width, int height, const char* title):m_Window(width, height, title)
{
	m_Instance = this;
}

void AppBase::InitWindow(int width, int height, const char* title)
{
	m_Window = Window(width, height, title);
}
