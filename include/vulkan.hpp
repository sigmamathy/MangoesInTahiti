#pragma once

#include "core.hpp"

// Create vulkan instance
// Create debug messenger (ifndef NDEBUG)
void LaunchVulkan();

// Destroy vulkan instance
// Destroy debug messenger (ifndef NDEBUG)
void EndVulkan();

VkInstance GetVulkanInstance();

// Forward declaration of Window in window.hpp
class Window;

struct CommandQueue
{
	VkQueue Queue;
	uint32_t FamilyIndex;
};

class GraphicsDevice
{
public:

	GraphicsDevice(Window const& window);
	~GraphicsDevice();

	inline VkPhysicalDevice GetPhysical() const { return m_physical; }

	inline VkDevice GetLogical() const { return m_logical;  }

	inline CommandQueue GetGraphicsQueue() const { return m_graphics_queue; }

	inline CommandQueue GetPresentQueue() const { return m_present_queue; }

	GraphicsDevice(GraphicsDevice const&) = delete;
	GraphicsDevice& operator=(GraphicsDevice const&) = delete;

private:

	VkPhysicalDevice m_physical;
	VkDevice m_logical;

	CommandQueue m_graphics_queue;
	CommandQueue m_present_queue;
};

void CreateSwapchain();

void DestroySwapchain();