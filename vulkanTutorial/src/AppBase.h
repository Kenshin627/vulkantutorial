#pragma once

class AppBase
{
public:
	AppBase() = default;
	virtual~AppBase() = default;
	virtual void RebuildFrameBuffer() = 0;
	virtual void CreateSetLayout() = 0;
};