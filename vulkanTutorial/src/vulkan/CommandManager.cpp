#include "../Core.h"
#include "CommandManager.h"

void CommandManager::SetContext(vk::Device& device, uint32_t queueFamilyIndex)
{
	m_Device = device;
	m_DefaultPool = CreatePool(queueFamilyIndex);
}

vk::CommandPool CommandManager::CreatePool(uint32_t queueFamilyIndex, vk::CommandPoolCreateFlags flag)
{
	vk::CommandPool pool;
	vk::CommandPoolCreateInfo poolInfo;
	poolInfo.sType = vk::StructureType::eCommandPoolCreateInfo;
	poolInfo.setFlags(flag).setQueueFamilyIndex(queueFamilyIndex);
	VK_CHECK_RESULT(m_Device.createCommandPool(&poolInfo, nullptr, &pool));
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
	VK_CHECK_RESULT(m_Device.allocateCommandBuffers(&bufferInfo, &command));
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

void CommandManager::CommandEnd(vk::CommandBuffer command)
{
	command.end();
}

void CommandManager::FlushCommandBuffer(vk::CommandBuffer command, vk::Queue queue, vk::CommandPool pool, bool free)
{
	command.end();
	vk::FenceCreateInfo fenceInfo;
	fenceInfo.sType = vk::StructureType::eFenceCreateInfo;
	fenceInfo.setFlags(vk::FenceCreateFlags());
	vk::SubmitInfo submitInfo;
	submitInfo.sType = vk::StructureType::eSubmitInfo;
	submitInfo.setCommandBufferCount(1)
			  .setPCommandBuffers(&command);
	VK_CHECK_RESULT(queue.submit(1, &submitInfo, nullptr));
	queue.waitIdle();
	if (free)
	{
		m_Device.freeCommandBuffers(pool, 1, &command);
	}
}

void CommandManager::FlushCommandBuffer(vk::CommandBuffer command, vk::Queue queue, bool free)
{
	FlushCommandBuffer(command, queue, m_DefaultPool, free);
}