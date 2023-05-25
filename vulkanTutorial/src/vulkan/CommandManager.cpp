#include "../Core.h"
#include "CommandManager.h"

void CommandManager::SetContext(Device& device)
{
	m_Device = device;
	m_DefaultPool = CreatePool(m_Device.QueryQueueFamilyIndices(m_Device.GetPhysicalDevice()).GraphicQueueIndex.value());
}

vk::CommandPool CommandManager::CreatePool(uint32_t queueFamilyIndex, vk::CommandPoolCreateFlags flag)
{
	vk::CommandPool pool;
	vk::CommandPoolCreateInfo poolInfo;
	poolInfo.sType = vk::StructureType::eCommandPoolCreateInfo;
	poolInfo.setFlags(flag).setQueueFamilyIndex(queueFamilyIndex);
	VK_CHECK_RESULT(m_Device.GetLogicDevice().createCommandPool(&poolInfo, nullptr, &pool));
	return pool;
}

vk::CommandBuffer CommandManager::AllocateCommandBuffer(vk::CommandBufferLevel level, vk::CommandPool pool, bool begin)
{
	vk::CommandBuffer command;
	vk::CommandBufferAllocateInfo bufferInfo;
	bufferInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
	bufferInfo.setCommandBufferCount(1)
		      .setCommandPool(pool)
		      .setLevel(level);
	VK_CHECK_RESULT(m_Device.GetLogicDevice().allocateCommandBuffers(&bufferInfo, &command));
	if (begin)
	{
		CommandBegin(command);
	}
	return command;
}

vk::CommandBuffer CommandManager::AllocateCommandBuffer(vk::CommandBufferLevel level, bool begin)
{
	return AllocateCommandBuffer(level, m_DefaultPool, begin);
}

void CommandManager::CommandBegin(vk::CommandBuffer command)
{
	vk::CommandBufferBeginInfo beginInfo;
	beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
	beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
		     .setPInheritanceInfo(nullptr);
	VK_CHECK_RESULT(command.begin(&beginInfo));
}

void CommandManager::FlushCommandBuffer(vk::CommandBuffer command, vk::Queue queue, vk::CommandPool pool, bool free)
{
	command.end();
	vk::Device device = m_Device.GetLogicDevice();
	vk::Fence fence;
	vk::FenceCreateInfo fenceInfo;
	fenceInfo.sType = vk::StructureType::eFenceCreateInfo;
	fenceInfo.setFlags(vk::FenceCreateFlags());
	VK_CHECK_RESULT(device.createFence(&fenceInfo, nullptr, &fence));
	vk::SubmitInfo submitInfo;
	submitInfo.sType = vk::StructureType::eSubmitInfo;
	submitInfo.setCommandBufferCount(1)
			  .setPCommandBuffers(&command);
	VK_CHECK_RESULT(queue.submit(1, &submitInfo, fence));
	VK_CHECK_RESULT(device.waitForFences(1, &fence, true, UINT64_MAX));
	device.destroyFence(fence, nullptr);
	if (free)
	{
		device.freeCommandBuffers(pool, 1, &command);
	}
}