#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.hpp>

class CommandManager
{
public:
	void SetContext(vk::Device& device, uint32_t queueFamilyIndex);
	vk::CommandPool CreatePool(uint32_t queueFamilyIndex, vk::CommandPoolCreateFlags flag = vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
	vk::CommandBuffer AllocateCommandBuffer(vk::CommandBufferLevel level, vk::CommandPool pool, bool begin = true);
	vk::CommandBuffer AllocateCommandBuffer(vk::CommandBufferLevel level, bool begin = true);
	void CommandBegin(vk::CommandBuffer command);
	void CommandEnd(vk::CommandBuffer command);
	void FlushCommandBuffer(vk::CommandBuffer command, vk::Queue queue, vk::CommandPool pool, bool free = true);
	void FlushCommandBuffer(vk::CommandBuffer command, vk::Queue queue, bool free = true);
	vk::CommandPool GetCommandPool() { return m_DefaultPool; }
private:
	vk::Device m_Device;
	vk::CommandPool m_DefaultPool;
};