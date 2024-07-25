#include "vksys.hpp"

#include <array>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <string>
#include <unordered_set>
#include <algorithm>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define THISFILE "vksys.cpp"
#define VALIDATE(...) if (!(__VA_ARGS__)) throw std::runtime_error("Statement \"" #__VA_ARGS__ "\" failed at " THISFILE " (line " + std::to_string(__LINE__) + ").")

namespace graphics
{

static struct s_VulkanResource
{
	// Instance
	VkInstance Instance;
	VkDebugUtilsMessengerEXT DebugMessenger;

	// Window
	GLFWwindow* Window;
	VkSurfaceKHR Surface;

	// Device
	VkPhysicalDevice PhysicalDevice;
	VkDevice Device;
	uint32_t GraphicsQueueIndex, PresentQueueIndex;
	VkQueue GraphicsQueue, PresentQueue;

	// Swapchain
	VkSwapchainKHR Swapchain;
	VkFormat SwapchainImageFormat;
	VkExtent2D SwapchainExtent;
	std::vector<VkImageView> SwapchainImageViews;

} s_vulkan;

static WindowData s_window_data;

static char const* s_VALIDATION_LAYER = "VK_LAYER_KHRONOS_validation"; // default vulkan validation layer

#ifndef NDEBUG

static VKAPI_ATTR VkBool32 VKAPI_CALL s_DebugMessageCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData)
{
	bool require_attention = messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;

	if (require_attention) std::cout << "\n";
	std::cout << "[Vulkan] ";
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		std::cout << "\033[91m";
	else if (require_attention)
		std::cout << "\033[93m";
	std::cout << pCallbackData->pMessage << "\n\033[0m";
	if (require_attention) std::cout << "\n";

	return VK_FALSE;
}

#endif

void LaunchVulkan()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	VkApplicationInfo app_info{};
	app_info.sType					= VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName		= "Vulkan Example";
	app_info.applicationVersion		= VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName			= "No Engine";
	app_info.engineVersion			= VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion				= VK_API_VERSION_1_3;

	VkInstanceCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;

	uint32_t glfw_extension_count = 0;
	char const** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

#ifndef NDEBUG

	uint32_t layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
	std::vector<VkLayerProperties> available_layers(layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

	VALIDATE(std::find_if(available_layers.begin(), available_layers.end(), [](VkLayerProperties const& layer) {
				return std::strcmp(s_VALIDATION_LAYER, layer.layerName) == 0;
				}) != available_layers.end());

	create_info.enabledLayerCount = 1;
	create_info.ppEnabledLayerNames = &s_VALIDATION_LAYER;

	std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	create_info.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT dci{};
	dci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	dci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	dci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	dci.pfnUserCallback = s_DebugMessageCallback;
	dci.pUserData = nullptr;

	create_info.pNext = &dci;

#else

	create_info.enabledLayerCount = 0;
	create_info.enabledExtensionCount = glfw_extension_count;
	create_info.ppEnabledExtensionNames = glfw_extensions;

#endif

	VALIDATE(vkCreateInstance(&create_info, nullptr, &s_vulkan.Instance) == VK_SUCCESS);

#ifndef NDEBUG

	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(s_vulkan.Instance, "vkCreateDebugUtilsMessengerEXT");
	VALIDATE(func);
	func(s_vulkan.Instance, &dci, nullptr, &s_vulkan.DebugMessenger);

#endif

}

void EndVulkan()
{
	glfwTerminate();

#ifndef NDEBUG

	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(s_vulkan.Instance, "vkDestroyDebugUtilsMessengerEXT");
	VALIDATE(func);
	func(s_vulkan.Instance, s_vulkan.DebugMessenger, nullptr);

#endif

	vkDestroyInstance(s_vulkan.Instance, nullptr);
}

void CreateWindow(int x, int y, bool fullscreen)
{
	// Use the primary monitor for fullscreen display.
	s_vulkan.Window = glfwCreateWindow(x, y, "Have some GOD DAMN FAITH", fullscreen ? glfwGetPrimaryMonitor() : 0, 0);
	s_window_data.Width = x, s_window_data.Height = y, s_window_data.Fullscreen = fullscreen;
	VALIDATE(glfwCreateWindowSurface(s_vulkan.Instance, s_vulkan.Window, nullptr, &s_vulkan.Surface) == VK_SUCCESS);
}

void DestroyWindow()
{
	vkDestroySurfaceKHR(s_vulkan.Instance, s_vulkan.Surface, nullptr);
	glfwDestroyWindow(s_vulkan.Window);
}

WindowData QueryWindowData()
{
	return s_window_data;
}

void CreateGraphicsLogicalDevice()
{
	uint32_t gpu_count = 0;
	vkEnumeratePhysicalDevices(s_vulkan.Instance, &gpu_count, nullptr);
	VALIDATE(gpu_count); // Ensure there is at least one gpu available.
	gpu_count = 1; // Only query the first GPU anyways.
	vkEnumeratePhysicalDevices(s_vulkan.Instance, &gpu_count, &s_vulkan.PhysicalDevice);

	VkPhysicalDeviceFeatures supported_features;
	vkGetPhysicalDeviceFeatures(s_vulkan.PhysicalDevice, &supported_features);
	VALIDATE(supported_features.samplerAnisotropy); // Ensure samplerAnisotropy is supported.

	uint32_t sfcount, spmcount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(s_vulkan.PhysicalDevice, s_vulkan.Surface, &sfcount, nullptr);
	vkGetPhysicalDeviceSurfacePresentModesKHR(s_vulkan.PhysicalDevice, s_vulkan.Surface, &spmcount, nullptr);
	VALIDATE(sfcount && spmcount); // Ensure there is at least one surface format and one surface present mode.

	std::vector<char const*> required_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	uint32_t extension_count;
	vkEnumerateDeviceExtensionProperties(s_vulkan.PhysicalDevice, nullptr, &extension_count, nullptr);
	std::vector<VkExtensionProperties> available_extensions(extension_count);
	vkEnumerateDeviceExtensionProperties(s_vulkan.PhysicalDevice, nullptr, &extension_count, available_extensions.data());

	// Ensures all required extensions is presented in available extensions.
	for (auto rext : required_extensions) {
		VALIDATE(std::find_if(available_extensions.begin(), available_extensions.end(), [rext](VkExtensionProperties const& aext) {
			return std::strcmp(rext, aext.extensionName) == 0; }) != available_extensions.end());
	}

	// Find the indices of queue families that support graphics and present.

	uint32_t qf_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(s_vulkan.PhysicalDevice, &qf_count, nullptr);
	std::vector<VkQueueFamilyProperties> families(qf_count);
	vkGetPhysicalDeviceQueueFamilyProperties(s_vulkan.PhysicalDevice, &qf_count, families.data());

	bool graphics_queue_found = false, present_queue_found = false;

	for (int i = 0; i < families.size(); i++)
	{
		if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			s_vulkan.GraphicsQueueIndex = i, graphics_queue_found = true;

		VkBool32 present_support;
		vkGetPhysicalDeviceSurfaceSupportKHR(s_vulkan.PhysicalDevice, i, s_vulkan.Surface, &present_support);
		if (present_support)
			s_vulkan.PresentQueueIndex = i, present_queue_found = true;

		if (graphics_queue_found && present_queue_found)
			break;
	}

	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
	float priority = 1.0f;

	// As queue indices may overlap, unordered_map is used to eliminate repeated values.
	std::unordered_set<uint32_t> distinct_indices = {
		s_vulkan.GraphicsQueueIndex,
		s_vulkan.PresentQueueIndex
	};

	for (uint32_t index : distinct_indices)
	{
		VkDeviceQueueCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		info.queueFamilyIndex = index;
		info.queueCount = 1;
		info.pQueuePriorities = &priority;
		queue_create_infos.push_back(info);
	}

	// Enable samplerAnisotropy
	VkPhysicalDeviceFeatures device_features{};
	device_features.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.pQueueCreateInfos = queue_create_infos.data();
	create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
	create_info.pEnabledFeatures = &device_features;
	create_info.ppEnabledExtensionNames = required_extensions.data();
	create_info.enabledExtensionCount = static_cast<uint32_t>(required_extensions.size());

#ifndef NDEBUG

	create_info.enabledLayerCount = 1;
	create_info.ppEnabledLayerNames = &s_VALIDATION_LAYER;

#else

	create_info.enabledLayerCount = 0;

#endif

	VALIDATE(vkCreateDevice(s_vulkan.PhysicalDevice, &create_info, nullptr, &s_vulkan.Device) == VK_SUCCESS);

	// Obtain device queue as well
	vkGetDeviceQueue(s_vulkan.Device, s_vulkan.GraphicsQueueIndex, 0, &s_vulkan.GraphicsQueue);
	vkGetDeviceQueue(s_vulkan.Device, s_vulkan.PresentQueueIndex, 0, &s_vulkan.PresentQueue);
}

void DestroyGraphicsLogicalDevice()
{
	vkDestroyDevice(s_vulkan.Device, nullptr);
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

void CreateSwapchain()
{
	VkSurfaceCapabilitiesKHR caps;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(s_vulkan.PhysicalDevice, s_vulkan.Surface, &caps);

	VkSurfaceFormatKHR format = s_ChooseSurfaceFormat(s_vulkan.PhysicalDevice, s_vulkan.Surface);
	VkPresentModeKHR present_mode = s_ChoosePresentModes(s_vulkan.PhysicalDevice, s_vulkan.Surface);
	VkExtent2D extent = s_ChooseSwapExtent(s_vulkan.Window, caps);

	uint32_t image_count = caps.minImageCount + 1;
	if (caps.maxImageCount > 0 && image_count > caps.maxImageCount)
		image_count = caps.maxImageCount;

	VkSwapchainCreateInfoKHR create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = s_vulkan.Surface;
	create_info.minImageCount = image_count;
	create_info.imageFormat = format.format;
	create_info.imageColorSpace = format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t queue_family_indices[] = { s_vulkan.GraphicsQueueIndex, s_vulkan.PresentQueueIndex };

	if (s_vulkan.GraphicsQueueIndex != s_vulkan.PresentQueueIndex)
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

	VALIDATE(vkCreateSwapchainKHR(s_vulkan.Device, &create_info, nullptr, &s_vulkan.Swapchain) == VK_SUCCESS);

	s_vulkan.SwapchainImageFormat = format.format;
	s_vulkan.SwapchainExtent = extent;

	// Create image views for images

	std::vector<VkImage> images;
	uint32_t imcount;
	vkGetSwapchainImagesKHR(s_vulkan.Device, s_vulkan.Swapchain, &imcount, nullptr);
	images.resize(imcount);
	vkGetSwapchainImagesKHR(s_vulkan.Device, s_vulkan.Swapchain, &imcount, images.data());
	s_vulkan.SwapchainImageViews.resize(imcount);

	for (uint32_t i = 0; i < imcount; i++)
	{
		VkImageViewCreateInfo ivci{};
		ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ivci.image = images[i];
		ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ivci.format = s_vulkan.SwapchainImageFormat;
		ivci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		ivci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		ivci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		ivci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ivci.subresourceRange.baseMipLevel = 0;
		ivci.subresourceRange.levelCount = 1;
		ivci.subresourceRange.baseArrayLayer = 0;
		ivci.subresourceRange.layerCount = 1;

		VALIDATE(vkCreateImageView(s_vulkan.Device, &ivci, nullptr, &s_vulkan.SwapchainImageViews[i]) == VK_SUCCESS);
	}

}

void DestroySwapchain()
{
	for (auto view : s_vulkan.SwapchainImageViews)
		vkDestroyImageView(s_vulkan.Device, view, nullptr);
	vkDestroySwapchainKHR(s_vulkan.Device, s_vulkan.Swapchain, nullptr);
}

}