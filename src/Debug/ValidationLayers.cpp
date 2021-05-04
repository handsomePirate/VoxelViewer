#include "Debug.hpp"
#include "Events/EventSystem.hpp"
#include "Logger/Logger.hpp"

bool Debug::ValidationLayers::CheckLayerPresent(const char* name)
{
	uint32_t instanceLayerCount;
	vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
	std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
	vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());
	bool validationLayerPresent = false;
	for (VkLayerProperties layer : instanceLayerProperties)
	{
		if (strcmp(layer.layerName, name) == 0)
		{
			return true;
		}
	}
	return false;
}

VKAPI_ATTR VkBool32 VKAPI_CALL ValidationCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData);

static PFN_vkCreateDebugUtilsMessengerEXT VkCreateDebugUtilsMessengerEXT;
static PFN_vkDestroyDebugUtilsMessengerEXT VkDestroyDebugUtilsMessengerEXT;
static VkDebugUtilsMessengerEXT DebugUtilsMessenger;

void Debug::ValidationLayers::Start(VkInstance instance)
{
	VkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
		vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
	VkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
		vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

	VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo{};
	debugUtilsMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugUtilsMessengerCreateInfo.messageSeverity = 
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugUtilsMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
	debugUtilsMessengerCreateInfo.pfnUserCallback = ValidationCallback;
	VkResult result = VkCreateDebugUtilsMessengerEXT(instance, &debugUtilsMessengerCreateInfo, nullptr, &DebugUtilsMessenger);
	assert(result == VK_SUCCESS);
}

void Debug::ValidationLayers::Shutdown(VkInstance instance)
{
	VkDestroyDebugUtilsMessengerEXT(instance, DebugUtilsMessenger, nullptr);
}

VKAPI_ATTR VkBool32 VKAPI_CALL ValidationCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	Core::EventData eventData;
	eventData.data.u64[0] = (uint64_t)pCallbackData;
	eventData.data.u32[2] = (uint32_t)messageSeverity;
	bool result = CoreEventSystem.SignalEvent(Core::EventCode::VulkanValidation, eventData);
	return (VkBool32)result;
}
