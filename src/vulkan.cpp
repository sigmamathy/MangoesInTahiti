#include "vulkan.hpp"
#include "window.hpp"

#define THISFILE "vulkan.cpp"

static VkInstance s_instance;
static VkDebugUtilsMessengerEXT s_debug_messenger;

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

	VALIDATE(vkCreateInstance(&create_info, nullptr, &s_instance) == VK_SUCCESS);

#ifndef NDEBUG

	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(s_instance, "vkCreateDebugUtilsMessengerEXT");
	VALIDATE(func);
	func(s_instance, &dci, nullptr, &s_debug_messenger);

#endif

}

void EndVulkan()
{
	glfwTerminate();

#ifndef NDEBUG

	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(s_instance, "vkDestroyDebugUtilsMessengerEXT");
	VALIDATE(func);
	func(s_instance, s_debug_messenger, nullptr);

#endif

	vkDestroyInstance(s_instance, nullptr);
}

VkInstance GetVulkanInstance()
{
	return s_instance;
}

GraphicsDevice::GraphicsDevice(Window const& window)
{
	auto surface = window.GetSurface();

	uint32_t gpu_count = 0;
	vkEnumeratePhysicalDevices(s_instance, &gpu_count, nullptr);
	VALIDATE(gpu_count); // Ensure there is at least one gpu available.
	gpu_count = 1; // Only query the first GPU anyways.
	vkEnumeratePhysicalDevices(s_instance, &gpu_count, &m_physical);

	VkPhysicalDeviceFeatures supported_features;
	vkGetPhysicalDeviceFeatures(m_physical, &supported_features);
	VALIDATE(supported_features.samplerAnisotropy); // Ensure sampler anisotropy is supported.

	uint32_t sfcount, spmcount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical, surface, &sfcount, nullptr);
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical, surface, &spmcount, nullptr);
	VALIDATE(sfcount && spmcount); // Ensure there is at least one surface format and one surface present mode.

	std::vector<char const*> required_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	uint32_t extension_count;
	vkEnumerateDeviceExtensionProperties(m_physical, nullptr, &extension_count, nullptr);
	std::vector<VkExtensionProperties> available_extensions(extension_count);
	vkEnumerateDeviceExtensionProperties(m_physical, nullptr, &extension_count, available_extensions.data());

	// Ensures all required extensions is presented in available extensions.
	for (auto rext : required_extensions) {
		VALIDATE(std::find_if(available_extensions.begin(), available_extensions.end(), [rext](VkExtensionProperties const& aext) {
			return std::strcmp(rext, aext.extensionName) == 0; }) != available_extensions.end());
	}

	// Find the indices of queue families that support graphics and present.

	uint32_t qf_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(m_physical, &qf_count, nullptr);
	std::vector<VkQueueFamilyProperties> families(qf_count);
	vkGetPhysicalDeviceQueueFamilyProperties(m_physical, &qf_count, families.data());

	bool graphics_queue_found = false, present_queue_found = false;

	for (int i = 0; i < families.size(); i++)
	{
		if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			m_graphics_queue.FamilyIndex = i, graphics_queue_found = true;

		VkBool32 present_support;
		vkGetPhysicalDeviceSurfaceSupportKHR(m_physical, i, surface, &present_support);
		if (present_support)
			m_present_queue.FamilyIndex = i, present_queue_found = true;

		if (graphics_queue_found && present_queue_found)
			break;
	}

	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
	float priority = 1.0f;

	// As queue indices may overlap, unordered_map is used to eliminate repeated values.
	std::unordered_set<uint32_t> distinct_indices = {
		m_graphics_queue.FamilyIndex,
		m_present_queue.FamilyIndex
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

	// Enable sampler anisotropy
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

	VALIDATE(vkCreateDevice(m_physical, &create_info, nullptr, &m_logical) == VK_SUCCESS);

	// Obtain device queue as well
	vkGetDeviceQueue(m_logical, m_graphics_queue.FamilyIndex, 0, &m_graphics_queue.Queue);
	vkGetDeviceQueue(m_logical, m_present_queue.FamilyIndex, 0, &m_present_queue.Queue);
}

GraphicsDevice::~GraphicsDevice()
{
	vkDestroyDevice(m_logical, nullptr);
}