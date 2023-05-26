#pragma once
#include "Image.h"
#include <vulkan/vulkan.hpp>

class Texture
{
public:
	void Create(Device& device, CommandManager& commandManager, const char* path, bool generateMipmaps);
	void Clear();
	uint32_t GetMipLevel() { return m_MipLevel; }
	Image& GetImage() { return m_Image; }
private:
	Image m_Image;
	uint32_t m_MipLevel;
	Device m_Device;
	CommandManager m_CommandManager;
};