#pragma once
#include <iostream>
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.hpp>

#define VK_CHECK_RESULT(f)																				\
{																										\
	vk::Result res = (f);																					\
	if (res != vk::Result::eSuccess)																				\
	{																									\
		std::cout << "Fatal : VkResult is \"" << res << "\" in " << __FILE__ << " at line " << __LINE__ << "\n"; \
		assert(res == vk::Result::eSuccess);																		\
	}																									\
}