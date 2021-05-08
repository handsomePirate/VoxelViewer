#pragma once
#include "Common.hpp"
#include <vector>

namespace VulkanFactory
{
	class Instance
	{
	public:
		static VkInstance Create(const std::vector<const char*>& extensions, 
			uint32_t apiVersion, const char* validationLayerName = "");
		static void Destroy(VkInstance instance);
	};
	class Device
	{
	public:
		struct DeviceInfo;

		static void Create(VkPhysicalDevice physicalDevice,
			VkPhysicalDeviceFeatures& enabledFeatures, std::vector<const char*> extensions,
			DeviceInfo& output,
			VkQueueFlags requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT,
			bool debugMarkersEnabled = false);
		static void Destroy(DeviceInfo& deviceInfo);
		
		struct DeviceInfo
		{
			VkDevice Handle = VK_NULL_HANDLE;
			VkPhysicalDevice PhysicalDevice;

			VkPhysicalDeviceFeatures EnabledFeatures;

			VkPhysicalDeviceProperties Properties;
			VkPhysicalDeviceFeatures Features;
			VkPhysicalDeviceMemoryProperties  MemoryProperties;
			VkFormatProperties FormatProperties;
			std::vector<VkQueueFamilyProperties> QueueFamilyProperties;

			VkFormat DepthFormat;

			bool DebugMarkersEnabled;

			struct DeviceQueueFamilyIndices
			{
				uint32_t graphics;
				uint32_t compute;
				uint32_t transfer;
				uint32_t present;
			} QueueFamilyIndices;

			// Surface static info.
			VkSurfaceFormatKHR SurfaceFormat;
			VkSurfaceCapabilitiesKHR SurfaceCapabilities;
			VkSurfaceTransformFlagBitsKHR SurfaceTransform;

			// Swapchain static info.
			VkPresentModeKHR PresentMode;
			VkCompositeAlphaFlagBitsKHR CompositeAlpha;
		};
	};
	class CommandPool
	{
	public:
		static VkCommandPool Create(VkDevice device, uint32_t queueIndex, VkCommandPoolCreateFlags flags = 0);
		static void Destroy(VkDevice device, VkCommandPool commandPool);
	};
	class Semaphore
	{
	public:
		static VkSemaphore Create(VkDevice device);
		static void Destroy(VkDevice device, VkSemaphore semaphore);
	};
	class Fence
	{
	public:
		static VkFence Create(VkDevice device, VkFenceCreateFlags flags = 0);
		static void Destroy(VkDevice device, VkFence fence);
	};
	class Surface
	{
	public:
		static VkSurfaceKHR Create(VkInstance instance, uint64_t windowHandle);
		static void Destroy(VkInstance instance, VkSurfaceKHR surface);
	};
	class Queue
	{
	public:
		static VkQueue Get(VkDevice device, uint32_t queueFamily, uint32_t queueIndex = 0);
	};
	class Swapchain
	{
	public:
		struct SwapchainInfo;

		static void Create(const Device::DeviceInfo& deviceInfo, uint32_t width, uint32_t height, VkSurfaceKHR surface,
			SwapchainInfo& output, SwapchainInfo* oldSwapchainInfo = nullptr);
		static void Destroy(const Device::DeviceInfo& deviceInfo, SwapchainInfo& swapchainInfo);

		struct SwapchainInfo
		{
			VkSwapchainKHR Handle;

			uint32_t Width;
			uint32_t Height;

			std::vector<VkImage> Images;
			std::vector<VkImageView> ImageViews;

			VkExtent2D Extent;
		};
	};
	class CommandBuffer
	{
	public:
		static void AllocatePrimary(VkDevice device, VkCommandPool commandPool,
			std::vector<VkCommandBuffer>& output, uint32_t bufferCount);
		static VkCommandBuffer AllocatePrimary(VkDevice device, VkCommandPool commandPool);
		static void Free(VkDevice device, VkCommandPool commandPool, std::vector<VkCommandBuffer>& buffers);
		static void Free(VkDevice device, VkCommandPool commandPool, VkCommandBuffer buffer);
	};
	class Image
	{
	public:
		struct ImageInfo;
		struct ImageInfo2;

		static void Create(const Device::DeviceInfo& deviceInfo,
			uint32_t width, uint32_t height, VkFormat format, ImageInfo& output);
		static void Destroy(VkDevice device, ImageInfo& imageInfo);
		static void Create(const Device::DeviceInfo& deviceInfo, uint32_t width, uint32_t height,
			VkFormat format, VkCommandPool commandPool, VkQueue queue, ImageInfo2& output);
		static void Destroy(VkDevice device, ImageInfo2& imageInfo);

		struct ImageInfo
		{
			VkImage Image;
			VkImageView View;
			VkDeviceMemory Memory;
		};

		struct ImageInfo2
		{
			VkImage Image;
			VkDescriptorImageInfo DescriptorImageInfo;
			VkDeviceMemory Memory;
		};
	};
	class Buffer
	{
		struct BufferInfo;

		static void Create(const Device::DeviceInfo& deviceInfo, VkBufferUsageFlags usage,
			VkDeviceSize size, VkMemoryPropertyFlags memoryProperties, BufferInfo& output);
		static void Destroy(const Device::DeviceInfo& deviceInfo, BufferInfo& bufferInfo);


		//static void Create(VkDevice device, BufferInfo& bufferInfo);

		struct BufferInfo
		{
			VkDescriptorBufferInfo DescriptorBufferInfo;
			VkDeviceMemory Memory;
			VkDeviceSize Size;
		};
	};
	class RenderPass
	{
	public:
		static VkRenderPass Create(VkDevice device, VkFormat colorFormat, VkFormat depthFormat);
		static void Destroy(VkDevice device, VkRenderPass renderPass);
	};
	class Framebuffer
	{
	public:
		static VkFramebuffer Create(VkDevice device, VkRenderPass renderPass, uint32_t width, uint32_t height,
			VkImageView colorView, VkImageView depthView);
		static void Destroy(VkDevice device, VkFramebuffer framebuffer);
	};
	class Descriptor
	{
	public:
		static VkDescriptorSetLayout CreateSetLayout(VkDevice device,
			VkDescriptorSetLayoutBinding* layoutBindings, uint32_t bindingCount);
		static void DestroySetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout);
		static VkDescriptorPool CreatePool(VkDevice device, VkDescriptorPoolSize* poolSizes,
			uint32_t sizesCount, uint32_t maxSets /*e.g. 2 for vertex and fragment*/);
		static void DestroyPool(VkDevice device, VkDescriptorPool descriptorPool);
		static VkDescriptorSet AllocateSet(VkDevice device, VkDescriptorPool descriptorPool,
			VkDescriptorSetLayout descriptorSetLayout);
		static void FreeSet(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSet descriptorSet);
	};
	class Pipeline
	{
	public:
		static VkPipeline CreateGraphics(VkDevice device, VkRenderPass renderPass, const Swapchain::SwapchainInfo& swapchain,
			VkShaderModule vertexShader, VkShaderModule fragmentShader, VkPipelineLayout pipelineLayout,
			VkPipelineCache pipelineCache = VK_NULL_HANDLE);
		static VkPipeline CreateCompute(VkDevice device, VkPipelineLayout pipelineLayout, VkShaderModule computeShader,
			VkPipelineCache pipelineCache = VK_NULL_HANDLE);
		static void Destroy(VkDevice device, VkPipeline pipeline);

		static VkPipelineLayout CreateLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout);
		static void DestroyLayout(VkDevice device, VkPipelineLayout pipelineLayout);

		static VkPipelineCache CreateCache(VkDevice device);
		static void DestroyCache(VkDevice device, VkPipelineCache pipelineCache);
	};
	class Shader
	{
	public:
		static VkShaderModule Create(VkDevice device, const std::string& path);
		static void Destroy(VkDevice device, VkShaderModule shader);
	};
};
