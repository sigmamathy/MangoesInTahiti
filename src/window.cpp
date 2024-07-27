#include "window.hpp"
#include "vulkan.hpp"

#define THISFILE "window.cpp"

Window::Window(int width, int height, bool fullscreen)
	: m_width(width), m_height(height), m_fullscreen(fullscreen)
{
	m_window = glfwCreateWindow(width, height, "Have some GOD DAMN FAITH",
		fullscreen ? glfwGetPrimaryMonitor() : 0, 0);
	VALIDATE(glfwCreateWindowSurface(GetVulkanInstance(), m_window, nullptr, &m_surface) == VK_SUCCESS);
}

Window::~Window()
{
	vkDestroySurfaceKHR(GetVulkanInstance(), m_surface, nullptr);
	glfwDestroyWindow(m_window);
}

static VkSurfaceFormatKHR s_ChooseSurfaceFormat(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	std::vector<VkSurfaceFormatKHR> formats;
	uint32_t count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, nullptr);
	formats.resize(count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, formats.data());

	for (auto const& format : formats) {
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return format;
	}

	return formats[0];
}

static VkPresentModeKHR s_ChoosePresentModes(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	std::vector<VkPresentModeKHR> modes;
	uint32_t count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, nullptr);
	modes.resize(count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, modes.data());

	for (auto const& mode : modes) {
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return mode;
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D s_ChooseSwapExtent(GLFWwindow* window, VkSurfaceCapabilitiesKHR const& caps)
{
	if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max())
		return caps.currentExtent;

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	VkExtent2D actual = {
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height)
	};

	actual.width = std::clamp(actual.width, caps.minImageExtent.width, caps.maxImageExtent.width);
	actual.height = std::clamp(actual.height, caps.minImageExtent.height, caps.maxImageExtent.height);

	return actual;
}

Swapchain::Swapchain(Window const& window, GraphicsDevice const& device)
	: m_device(&device)
{
	auto surface = window.GetSurface();

	auto pd = device.GetPhysical();
	auto ld = device.GetLogical();
	auto gq = device.GetGraphicsQueue();
	auto pq = device.GetPresentQueue();

	VkSurfaceCapabilitiesKHR caps;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pd, surface, &caps);

	VkSurfaceFormatKHR format			= s_ChooseSurfaceFormat(pd, surface);
	VkPresentModeKHR present_mode		= s_ChoosePresentModes(pd, surface);
	VkExtent2D extent					= s_ChooseSwapExtent(window.GetNativePointer(), caps);

	uint32_t image_count = caps.minImageCount + 1;
	if (caps.maxImageCount > 0 && image_count > caps.maxImageCount)
		image_count = caps.maxImageCount;

	VkSwapchainCreateInfoKHR create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = surface;
	create_info.minImageCount = image_count;
	create_info.imageFormat = format.format;
	create_info.imageColorSpace = format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t queue_family_indices[] = { gq.FamilyIndex, pq.FamilyIndex };

	if (gq.FamilyIndex != pq.FamilyIndex)
	{
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 2;
		create_info.pQueueFamilyIndices = queue_family_indices;
	}
	else
	{
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		create_info.queueFamilyIndexCount = 0;
		create_info.pQueueFamilyIndices = nullptr;
	}

	create_info.preTransform = caps.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.clipped = VK_TRUE;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;

	VALIDATE(vkCreateSwapchainKHR(ld, &create_info, nullptr, &m_swapchain) == VK_SUCCESS);

	m_image_format = format.format;
	m_extent = extent;

	std::vector<VkImage> images;
	uint32_t imcount;
	vkGetSwapchainImagesKHR(ld, m_swapchain, &imcount, nullptr);
	images.resize(imcount);
	vkGetSwapchainImagesKHR(ld, m_swapchain, &imcount, images.data());
	m_image_views.resize(imcount);

	for (uint32_t i = 0; i < imcount; i++)
	{
		VkImageViewCreateInfo ivci{};
		ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ivci.image = images[i];
		ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ivci.format = m_image_format;
		ivci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		ivci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		ivci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		ivci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ivci.subresourceRange.baseMipLevel = 0;
		ivci.subresourceRange.levelCount = 1;
		ivci.subresourceRange.baseArrayLayer = 0;
		ivci.subresourceRange.layerCount = 1;

		VALIDATE(vkCreateImageView(ld, &ivci, nullptr, &m_image_views[i]) == VK_SUCCESS);
	}
}

Swapchain::~Swapchain()
{
	for (auto view : m_image_views)
		vkDestroyImageView(m_device->GetLogical(), view, nullptr);
	vkDestroySwapchainKHR(m_device->GetLogical(), m_swapchain, nullptr);
}