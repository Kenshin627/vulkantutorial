#pragma once

#include "Device.h"
#include <vulkan/vulkan.hpp>

class CommandManager
{
public:
	void SetContext(Device& device);
	vk::CommandPool CreatePool(uint32_t queueFamilyIndex, vk::CommandPoolCreateFlags flag = vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
	vk::CommandBuffer AllocateCommandBuffer(vk::CommandBufferLevel level, vk::CommandPool pool, bool begin = true);
	vk::CommandBuffer AllocateCommandBuffer(vk::CommandBufferLevel level, bool begin = true);
	void CommandBegin(vk::CommandBuffer command);
	void FlushCommandBuffer(vk::CommandBuffer command, vk::Queue queue, vk::CommandPool pool, bool free = true);
	vk::CommandPool GetCommandPool() { return m_DefaultPool; }
private:
	Device m_Device;
	vk::CommandPool m_DefaultPool;
};