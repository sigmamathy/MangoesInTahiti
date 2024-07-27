#pragma once

#include "core.hpp"

class GraphicsDevice;

class Window
{
public:

	Window(int width, int height, bool fullscreen);

	~Window();

	inline GLFWwindow* GetNativePointer() const { return m_window; }

	inline VkSurfaceKHR GetSurface() const { return m_surface; }

	Window(Window const&) = delete;
	Window& operator=(Window const&) = delete;

private:

	GLFWwindow* m_window;
	VkSurfaceKHR m_surface;

	int m_width, m_height;
	bool m_fullscreen;
};

class Swapchain
{
public:

	Swapchain(Window const& window, GraphicsDevice const& device);
	~Swapchain();

	inline VkFormat GetImageFormat() const { return m_image_format; }

	inline std::vector<VkImageView> GetImageViews() const { return m_image_views; }

	inline VkExtent2D GetExtent() const { return m_extent; }

	Swapchain(Swapchain const&) = delete;
	Swapchain& operator=(Swapchain const&) = delete;

private:

	GraphicsDevice const* m_device;
	VkSwapchainKHR m_swapchain;
	VkFormat m_image_format;
	VkExtent2D m_extent;
	std::vector<VkImageView> m_image_views;
};