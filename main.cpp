
#define  GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define  GLM_FORCE_RANIANS
#define  GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include <functional>
#include <cstdlib>
#include <vector>
#include <set>
#include<algorithm>
#include <fstream>
#include<array>
#include<chrono>


const int WIDTH = 800;
const int HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
		"VK_LAYER_LUNARG_standard_validation"
};

const std::vector<const char*> deviceExtension = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};


static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData
)
{
	std::cerr << "validation layer:" << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

VkDebugUtilsMessengerEXT callback;

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif // NDEBUG

struct QueueFamilyIndices
{
	int graphicsFamily = -1;
	int presentFamily = -1;

	bool isComplete()
	{
		return graphicsFamily >= 0
				&&presentFamily>=0;
	}
};

static std::vector<char> readFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(),fileSize);
	file.close();

	return buffer;
}

struct Vertex
{
	glm::vec2 pos;
	glm::vec3 color;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}
};

const std::vector<Vertex> vertices = {
	{{-0.5f,-0.5f},{1.0f,0.0f,0.0f}},
	{{0.5f,-0.5f},{0.0f,1.0f,0.0f}},
	{{0.5f,0.5f},{0.0f,0.0f,1.0f}},
	{{-0.5f,0.5f},{0.0f,1.0f,1.0f}}
};

const std::vector<uint16_t> indices = {
	0,1,2,2,3,0
};

struct UniformBufferObject
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

class HelloTriangleApplication
{
public:
	void run()
	{
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}
private:
	GLFWwindow* _window;
	VkInstance _vkInstance;
	VkPhysicalDevice _physicalDevice;
	VkDevice  _vkDevice;
	VkQueue  _graphicsQueue;
	VkQueue _presentQueue;
	VkSurfaceKHR _surface;
	VkViewport _viewport;
	VkRect2D _scissor;
	VkSwapchainKHR _swapChain;
	std::vector<VkImage> _swapChainImages;
	VkFormat _swapChainImageFormat;
	VkExtent2D _swapChainExtent;
	std::vector<VkImageView> _swapChainImageViews;
	VkRenderPass _renderPass;
	VkDescriptorSetLayout _descriptorSetLayout;
	VkPipelineLayout _pipelineLayout;
	VkPipeline _graphicPipeline;
	std::vector<VkFramebuffer> _swapChainFramembuffers;
	VkCommandPool _commandPool;
	std::vector<VkCommandBuffer> _commandBuffers;
	std::vector<VkSemaphore> _imageAvailableSemaphores;
	std::vector<VkSemaphore> _renderFinishedSemaphores;
	std::vector<VkFence> _inFlightFences;
	size_t _currentFrame = 0;
	bool _framebufferResized = false;
	VkBuffer _vertexBuffer;
	VkDeviceMemory _vertexBufferMemory;
	VkBuffer _indexBuffer;
	VkDeviceMemory _indexBufferMemory;
	std::vector<VkBuffer> _uniformBuffers;
	std::vector<VkBuffer> _uniformBuffersMemory;
	VkDescriptorPool _descriptorPool;
	std::vector<VkDescriptorSet> _descriptorSets;
	VkImage _textureImage;
	VkDeviceMemory  _textureImageMemory;


	void initWindow()
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
		glfwSetWindowUserPointer(_window,this);
		glfwSetFramebufferSizeCallback(_window, framebufferResizeCallback);
	}

	static void framebufferResizeCallback(GLFWwindow* window, int widht, int height)
	{
		auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
		app->_framebufferResized = true;
	}

	void initVulkan()
	{
		createInstance();
		setupDebugCallback();
		createSurface();
		selectPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createImageViews();
		createRenderPass();
		createDescriptorSetLayout();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandPool();
		createTextureImage();
		createVertexBuffer();
		createIndexBuffer();
		createUniformBuffers();
		createDescriptorPool();
		createDescriptorSets();
		createCommandBuffers();
		createSyncObjects();
	}

	void mainLoop()
	{
		while (!glfwWindowShouldClose(_window))
		{
			glfwPollEvents();
			drawFrame();
		}

		vkDeviceWaitIdle(_vkDevice);
	}

	void cleanup()
	{
		cleanupSwapChain();

		vkDestroyImage(_vkDevice, _textureImage, nullptr);
		vkFreeMemory(_vkDevice,_textureImageMemory,nullptr);

		vkDestroyDescriptorPool(_vkDevice, _descriptorPool, nullptr);

		vkDestroyDescriptorSetLayout(_vkDevice, _descriptorSetLayout, nullptr);

		for (size_t i = 0; i < _swapChainImages.size(); ++i)
		{
			vkDestroyBuffer(_vkDevice,_uniformBuffers[i],nullptr);
			vkFreeMemory(_vkDevice, _uniformBuffersMemory[i], nullptr);
		}

		vkDestroyBuffer(_vkDevice, _indexBuffer, nullptr);
		vkFreeMemory(_vkDevice, _indexBufferMemory, nullptr);

		vkDestroyBuffer(_vkDevice,_vertexBuffer,nullptr);
		vkFreeMemory(_vkDevice,_vertexBufferMemory,nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			vkDestroySemaphore(_vkDevice, _renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(_vkDevice, _imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(_vkDevice, _inFlightFences[i], nullptr);
		}

		vkDestroyCommandPool(_vkDevice, _commandPool, nullptr);
		
		vkDestroyDevice(_vkDevice, nullptr);
		if (enableValidationLayers)
		{
			DestroyDebugUtilsMessengerEXT(_vkInstance, callback, nullptr);
		}

		vkDestroySurfaceKHR(_vkInstance, _surface, nullptr);
		vkDestroyInstance(_vkInstance, nullptr);
		glfwDestroyWindow(_window);
		glfwTerminate();
	}

	bool checkValidationLayerSupport()
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount,availableLayers.data());

		for (const char* layerName : validationLayers)
		{
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
				return false;
		}


		return true;
	}

	void createInstance()
	{
		if (enableValidationLayers && !checkValidationLayerSupport())
		{
			throw std::runtime_error("validation layers requested,but not available!");
		}

		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "helloTriangl";
		appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
		appInfo.pEngineName = "No_Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1,0,0);
		appInfo.apiVersion = VK_API_VERSION_1_1;
		appInfo.pNext = nullptr;

		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		
		std::vector<const char*> extensionNames;
		//getRequiredExtensions(extensionNames);
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> extensions(extensionCount);
		extensionNames.resize(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

		size_t count = extensions.size();
		for (size_t i=0;i<count;++i)
		{
			extensionNames.at(i) = extensions.at(i).extensionName;
		}

		if (enableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			bool hasExtDebug=false;
			for (auto perExtensionName : extensionNames)
			{
				if (strcmp(perExtensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
				{
					hasExtDebug = true;
					break;
				}
			}

			if(!hasExtDebug)
				extensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensionNames.size());
		createInfo.ppEnabledExtensionNames = extensionNames.data();

		VkResult result = vkCreateInstance(&createInfo, nullptr,&_vkInstance);
		
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create instance!");
		}
	}

	void setupDebugCallback()
	{
		if (!enableValidationLayers) 
			return;
		VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
		createInfo.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
		createInfo.pUserData = nullptr;

		auto CreateDebugUtilsMessengerEXT = [](VkInstance instance,
			const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
			const VkAllocationCallbacks* pAllocator,
			VkDebugUtilsMessengerEXT* pCallback)
		{
			auto func = (PFN_vkCreateDebugUtilsMessengerEXT)
				vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
			
			if (func != nullptr)
			{
				return func(instance, pCreateInfo, pAllocator, pCallback);
			}
			else
			{
				return VK_ERROR_EXTENSION_NOT_PRESENT;
			}
		};

		if (CreateDebugUtilsMessengerEXT(_vkInstance, &createInfo, nullptr, &callback) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to set up debug callback!");
		}
	}

	void DestroyDebugUtilsMessengerEXT(VkInstance instance,
						VkDebugUtilsMessengerEXT callback,
						const VkAllocationCallbacks* pAllocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)
			vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

		if (func != nullptr)
		{
			func(instance,callback,pAllocator);
		}
	}

	void selectPhysicalDevice()
	{
		_physicalDevice = VK_NULL_HANDLE;
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(_vkInstance, &deviceCount, nullptr);
		if (deviceCount == 0)
		{
			throw std::runtime_error("failed to find GPUs withs Vulkan support!");
		}
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(_vkInstance,&deviceCount,devices.data());
		for (const auto& device : devices)
		{
			if (isDeviceSuitable(device))
			{
				_physicalDevice = device;
				break;
			}
		}
		if (_physicalDevice == VK_NULL_HANDLE)
		{
			throw std::runtime_error("failed to find a suitable GPU！");
		}
	}

	bool isDeviceSuitable(VkPhysicalDevice device)
	{
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;

		vkGetPhysicalDeviceProperties(device,&deviceProperties);
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
		QueueFamilyIndices indices = findQueueFamilies(device);

		bool extensionSupported = checkDeviceExtensionSupport(device);

		bool swapChainAdequate = false;
		if (extensionSupported)
		{
			SwapChainSupportDetails swapChainSupport =
				querySwapChainSupport(device);

			swapChainAdequate = !swapChainSupport.formats.empty()
				&& !swapChainSupport.presentModes.empty();
		}

		return deviceProperties.deviceType==VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
			&&deviceFeatures.geometryShader&&
			indices.isComplete()&&extensionSupported
			&&swapChainAdequate;
	}

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		for (uint32_t i = 0; i < queueFamilyCount; ++i)
		{
			VkQueueFamilyProperties queueFamily = queueFamilies[i];

			if (queueFamily.queueCount > 0
				&& queueFamily.queueFlags&VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphicsFamily = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _surface, &presentSupport);

			if (queueFamily.queueCount > 0 && presentSupport)
			{
				indices.presentFamily = i;
			}

			if (indices.isComplete())
				break;
		}
		return indices;
	}
	
	void createLogicalDevice()
	{
		if (_physicalDevice == VK_NULL_HANDLE)
			return;

		QueueFamilyIndices indices = findQueueFamilies(_physicalDevice);
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<int> uniqueQueueFamilies = {indices.graphicsFamily,
			indices.presentFamily};

		float queuePriority = 1.0f;
		for (int queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures = {};

		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtension.size());
		createInfo.ppEnabledExtensionNames = deviceExtension.data();
		if (enableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}
		
		if (vkCreateDevice(_physicalDevice, &createInfo, nullptr, &_vkDevice) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create logical device!");
		}

		vkGetDeviceQueue(_vkDevice, indices.graphicsFamily, 0, &_graphicsQueue);
		vkGetDeviceQueue(_vkDevice,indices.presentFamily,0,&_presentQueue);
	}

	void createSurface()
	{
		VkResult res= glfwCreateWindowSurface(_vkInstance, _window, nullptr, &_surface);
		if (res != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create window surface!");
		}
	}

	bool checkDeviceExtensionSupport(VkPhysicalDevice device)
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
		std::set<std::string> requiredExtensions(deviceExtension.begin(), deviceExtension.end());
		
		for (const auto& extension : availableExtensions)
		{
			requiredExtensions.erase(extension.extensionName);
		}
		return requiredExtensions.empty();
	}

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
	{
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, nullptr);
		if (formatCount != 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, nullptr);
		if (presentModeCount != 0)
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> availableFormats)
	{
		if (availableFormats.size() == 1 &&
			availableFormats[0].format == VK_FORMAT_UNDEFINED)
		{
			return {VK_FORMAT_B8G8R8A8_UNORM,VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
		}

		for (const auto& availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM
				&& availableFormat.colorSpace
				== VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				return availableFormat;
		}

		return availableFormats[0];
	}

	VkPresentModeKHR   chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
	{
		VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;
		for (const auto& availablePresentMode : availablePresentModes)
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
				return availablePresentMode;
			else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
				bestMode= availablePresentMode;
		}
		return bestMode;
	}

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}
		else
		{
			int width, height;
			glfwGetFramebufferSize(_window,&width,&height);

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::max(capabilities.minImageExtent.width,
				std::min(capabilities.maxImageExtent.width,actualExtent.width));


			actualExtent.height = std::max(capabilities.minImageExtent.height,
				std::min(capabilities.maxImageExtent.height, actualExtent.height));

			return actualExtent;
		}
	}

	void createSwapChain()
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(_physicalDevice);

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 &&
			imageCount > swapChainSupport.capabilities.maxImageCount)
		{
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = _surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = findQueueFamilies(_physicalDevice);
		uint32_t queueFamilyIndices[] = {(uint32_t)indices.graphicsFamily,(uint32_t)indices.presentFamily};
		if (indices.graphicsFamily != indices.presentFamily)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(_vkDevice, &createInfo, nullptr, &_swapChain) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create swap chain!");
		}

		vkGetSwapchainImagesKHR(_vkDevice, _swapChain, &imageCount, nullptr);
		_swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(_vkDevice, _swapChain, &imageCount, _swapChainImages.data());
		
		_swapChainImageFormat = surfaceFormat.format;
		_swapChainExtent = extent;
	}

	void createImageViews()
	{
		_swapChainImageViews.resize(_swapChainImages.size());
		for (size_t i = 0; i < _swapChainImages.size(); ++i)
		{
			VkImageViewCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = _swapChainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = _swapChainImageFormat;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(_vkDevice, &createInfo, nullptr,
				&_swapChainImageViews[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create image views!");
			}
		}
	}

	void createGraphicsPipeline()
	{
		/*可编程阶段配置*/
		auto vertShaderCode = readFile("shaders/vert.spv");
		auto fragShaderCode = readFile("shaders/frag.spv");

		VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);
		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";
		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";
		VkPipelineShaderStageCreateInfo shaderStages[] = {
			vertShaderStageInfo,
			fragShaderStageInfo
		};

		/*固定功能配置*/
		//顶点输入

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescription = Vertex::getAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescription.data();
		//输入装配
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;
		//视口裁剪
		_viewport = {};
		_viewport.x = 0.0f;
		_viewport.y = 0.0f;
		_viewport.width = (float)_swapChainExtent.width;
		_viewport.height = (float)_swapChainExtent.height;
		_viewport.minDepth = 0.0f;
		_viewport.maxDepth = 1.0f;

		_scissor = {};
		_scissor.offset = {0,0};
		_scissor.extent = _swapChainExtent;

		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &_viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &_scissor;

		//光栅化
		VkPipelineRasterizationStateCreateInfo rasterizationStage = {};
		rasterizationStage.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationStage.depthClampEnable = VK_FALSE;
		rasterizationStage.rasterizerDiscardEnable = VK_FALSE;
		rasterizationStage.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationStage.lineWidth = 1.0f;
		rasterizationStage.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizationStage.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationStage.depthBiasEnable = VK_FALSE;
		rasterizationStage.depthBiasConstantFactor = 0.0f;
		rasterizationStage.depthBiasClamp = 0.0f;
		rasterizationStage.depthBiasSlopeFactor = 0.0f;

		//多重采样
		VkPipelineMultisampleStateCreateInfo multisampleStage = {};
		multisampleStage.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleStage.sampleShadingEnable = VK_FALSE;
		multisampleStage.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleStage.minSampleShading = 1.0f;
		multisampleStage.pSampleMask = nullptr;
		multisampleStage.alphaToCoverageEnable = VK_FALSE;
		multisampleStage.alphaToOneEnable = VK_FALSE;

		//深度和模板测试
		VkPipelineDepthStencilStateCreateInfo depthStencilStage = {};

		//颜色混合
		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlendStage = {};
		colorBlendStage.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendStage.logicOpEnable = VK_FALSE;
		colorBlendStage.attachmentCount = 1;
		colorBlendStage.pAttachments = &colorBlendAttachment;
		colorBlendStage.blendConstants[0] = 0.0f; 
		colorBlendStage.blendConstants[1] = 0.0f;
		colorBlendStage.blendConstants[2] = 0.0f;
		colorBlendStage.blendConstants[3] = 0.0f;

		//动态状态
		//VkDynamicState dynamicStages[] = {
		//	VK_DYNAMIC_STATE_VIEWPORT,
		//	VK_DYNAMIC_STATE_SCISSOR,
		//	VK_DYNAMIC_STATE_LINE_WIDTH
		//};

		//VkPipelineDynamicStateCreateInfo dynamicStage = {};
		//dynamicStage.sType= VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		//dynamicStage.dynamicStateCount = 3;
		//dynamicStage.pDynamicStates = dynamicStages;

		//管线布局
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &_descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		if (vkCreatePipelineLayout(_vkDevice, &pipelineLayoutInfo, nullptr, &_pipelineLayout)
			!= VK_SUCCESS)
		{
			throw std::runtime_error("failed to create pipeline layout!");
		}

		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizationStage;
		pipelineInfo.pMultisampleState = &multisampleStage;
		pipelineInfo.pDepthStencilState = &depthStencilStage;
		pipelineInfo.pColorBlendState = &colorBlendStage;
		//pipelineInfo.pDynamicState =&dynamicStage;
		pipelineInfo.layout = _pipelineLayout;
		pipelineInfo.renderPass = _renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;
		if (vkCreateGraphicsPipelines(_vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo,
			nullptr, &_graphicPipeline) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create graphics pipeline!");
		}

		vkDestroyShaderModule(_vkDevice, fragShaderModule, nullptr);
		vkDestroyShaderModule(_vkDevice, vertShaderModule, nullptr);
	}

	VkShaderModule createShaderModule(const std::vector<char>& code)
	{
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
		VkShaderModule shaderMoudule;
		if (vkCreateShaderModule(_vkDevice, &createInfo, nullptr, &shaderMoudule)
			!= VK_SUCCESS)
		{
			throw std::runtime_error("failed to create shader module!");
		}

		return shaderMoudule;
	}

	void createRenderPass()
	{
		VkAttachmentDescription colorAttachment = {};
		colorAttachment.format = _swapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(_vkDevice, &renderPassInfo, nullptr,
			&_renderPass) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create render pass!");
		}

		
	}

	void createFramebuffers()
	{
		_swapChainFramembuffers.resize(_swapChainImageViews.size());

		size_t fbCount = _swapChainFramembuffers.size();
		for (size_t i = 0; i < fbCount; ++i)
		{
			VkImageView attachments[]={
				_swapChainImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = _renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = _swapChainExtent.width;
			framebufferInfo.height = _swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(_vkDevice, &framebufferInfo,
				nullptr,&_swapChainFramembuffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create framebuffer!");
			}

		}
	}

	void createCommandPool()
	{
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(_physicalDevice);

		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
		poolInfo.flags = 0;
		if (vkCreateCommandPool(_vkDevice, &poolInfo, nullptr, &_commandPool)
			!= VK_SUCCESS)
		{
			throw std::runtime_error("failed to create command pool!");
		}
	}

	void createCommandBuffers()
	{
		_commandBuffers.resize(_swapChainFramembuffers.size());

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandBufferCount = (uint32_t)_commandBuffers.size();
		allocInfo.commandPool = _commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		if (vkAllocateCommandBuffers(_vkDevice, &allocInfo,
			_commandBuffers.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create command buffers!");
		}

		size_t commandBufferCount = _commandBuffers.size();
		for (size_t i = 0; i < commandBufferCount; ++i)
		{
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
			beginInfo.pInheritanceInfo = nullptr;
			if (vkBeginCommandBuffer(_commandBuffers[i], &beginInfo)
				!=VK_SUCCESS)
			{
				throw std::runtime_error("failed to create begin recording command buffer!");
			}

			//开始渲染流程
			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = _renderPass;
			renderPassInfo.framebuffer = _swapChainFramembuffers[i];
			renderPassInfo.renderArea.offset = {0,0};
			renderPassInfo.renderArea.extent = _swapChainExtent;
			VkClearValue clearColor = {0.2,0.2,0.2,1.0};
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			vkCmdBeginRenderPass(_commandBuffers[i],&renderPassInfo,VK_SUBPASS_CONTENTS_INLINE);
			//vkCmdSetViewport(_commandBuffers[i], 0, 1,&_viewport);
			//vkCmdSetScissor(_commandBuffers[i], 0, 1, &_scissor);
			//vkCmdSetLineWidth(_commandBuffers[i], 1.0);
			vkCmdBindPipeline(_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicPipeline);
			VkBuffer vertexBuffers[] = {_vertexBuffer};
			VkDeviceSize offsets[] = {0};
			vkCmdBindVertexBuffers(_commandBuffers[i],0,1, vertexBuffers,offsets);
			vkCmdBindIndexBuffer(_commandBuffers[i],_indexBuffer,0,VK_INDEX_TYPE_UINT16);
			//vkCmdDraw(_commandBuffers[i], 
			//	static_cast<uint32_t>(vertices.size()), 1, 0,0);
			vkCmdBindDescriptorSets(_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS
				, _pipelineLayout, 0, 1, &_descriptorSets[i], 0, nullptr);
			vkCmdDrawIndexed(_commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
			vkCmdEndRenderPass(_commandBuffers[i]);

			if (vkEndCommandBuffer(_commandBuffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to record command buffer!");
			}
		}
	}

	void createSyncObjects()
	{
		_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			if (vkCreateSemaphore(_vkDevice, &semaphoreInfo, nullptr,
				&_imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(_vkDevice, &semaphoreInfo, nullptr,
					&_renderFinishedSemaphores[i]) != VK_SUCCESS
				||vkCreateFence(_vkDevice,&fenceInfo,nullptr,&_inFlightFences[i])!=VK_SUCCESS)
			{
				throw std::runtime_error("failed to create semaphores!");
			}
		}
	}

	void drawFrame()
	{
		vkWaitForFences(_vkDevice,1,&_inFlightFences[_currentFrame],VK_TRUE,
			std::numeric_limits<uint64_t>::max());

		//获取交换链图片索引
		uint32_t imageIndex;
		VkResult result= vkAcquireNextImageKHR(_vkDevice, _swapChain,
			std::numeric_limits<uint64_t>::max()
			, _imageAvailableSemaphores[_currentFrame], VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			recreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		updateUniformBuffer(imageIndex);

		//提交指令缓冲
		VkSemaphore waitSemaphores[] = { _imageAvailableSemaphores[_currentFrame] };

		VkPipelineStageFlags waitStages[] = {
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		};
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &_commandBuffers[imageIndex];
		VkSemaphore signalSemaphores[] = {_renderFinishedSemaphores[_currentFrame]};
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		vkResetFences(_vkDevice, 1, &_inFlightFences[_currentFrame]);

		if (vkQueueSubmit(_graphicsQueue, 1,
			&submitInfo, _inFlightFences[_currentFrame]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to submit \
				draw command buffer!");
		}

		VkSwapchainKHR swapChains[] = {_swapChain};
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;
		result= vkQueuePresentKHR(_presentQueue, &presentInfo);


		if (result == VK_ERROR_OUT_OF_DATE_KHR
			||result==VK_SUBOPTIMAL_KHR||_framebufferResized)
		{
			_framebufferResized = false;
			recreateSwapChain();
		}
		else if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		_currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void recreateSwapChain()
	{
		int width = 0, height = 0;
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(_window,&width,&height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(_vkDevice);

		cleanupSwapChain();

		createSwapChain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandBuffers();
	}

	void cleanupSwapChain()
	{
		for (auto framebuffer : _swapChainFramembuffers)
		{
			vkDestroyFramebuffer(_vkDevice, framebuffer, nullptr);
		}

		vkFreeCommandBuffers(_vkDevice, _commandPool,
			static_cast<uint32_t>(_commandBuffers.size()), _commandBuffers.data());

		vkDestroyPipeline(_vkDevice, _graphicPipeline, nullptr);
		vkDestroyPipelineLayout(_vkDevice, _pipelineLayout, nullptr);
		vkDestroyRenderPass(_vkDevice, _renderPass, nullptr);
		for (auto imageView : _swapChainImageViews)
		{
			vkDestroyImageView(_vkDevice, imageView, nullptr);
		}

		vkDestroySwapchainKHR(_vkDevice, _swapChain, nullptr);
	}

	uint32_t findMemoryType(uint32_t typeFilter,VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(_physicalDevice,&memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
		{
			if (typeFilter&(1 << i) && (memProperties
				.memoryTypes[i].propertyFlags&properties) == properties)
			{
				return i;
			}	
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}

	void createVertexBuffer()
	{
		VkDeviceSize bufferSize = sizeof(vertices[0])*vertices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);


		void* data;
		vkMapMemory(_vkDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), (size_t)bufferSize);
		vkUnmapMemory(_vkDevice, stagingBufferMemory);

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT|
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,_vertexBuffer,_vertexBufferMemory);

		copyBuffer(stagingBuffer,_vertexBuffer,bufferSize);

		vkDestroyBuffer(_vkDevice,stagingBuffer,nullptr);
		vkFreeMemory(_vkDevice,stagingBufferMemory,nullptr);
	}

	void createBuffer(VkDeviceSize size,VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,VkBuffer& buffer,VkDeviceMemory& bufferMemory)
	{
		VkBufferCreateInfo bufferInfo={};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(_vkDevice, &bufferInfo, nullptr, &buffer)
			!= VK_SUCCESS)
		{
			throw std::runtime_error("failed to create buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(_vkDevice, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(
			memRequirements.memoryTypeBits,properties);

		if (vkAllocateMemory(_vkDevice, &allocInfo, nullptr,
			&bufferMemory) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate buffer memory!");
		}

		vkBindBufferMemory(_vkDevice,buffer,bufferMemory,0);
	}

	void copyBuffer(VkBuffer srcBuffer,VkBuffer dstBuffer,VkDeviceSize size)
	{
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer,srcBuffer,dstBuffer,1,&copyRegion);
		
		endSingleTimeCommands(commandBuffer);
	}

	void createIndexBuffer()
	{
		VkDeviceSize bufferSize = sizeof(indices[0])*indices.size();
		
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,stagingBuffer,stagingBufferMemory);

		void* data;
		vkMapMemory(_vkDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), (size_t)bufferSize);
		vkUnmapMemory(_vkDevice, stagingBufferMemory);

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT |
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			_indexBuffer, _indexBufferMemory);

		copyBuffer(stagingBuffer, _indexBuffer, bufferSize);

		vkDestroyBuffer(_vkDevice, stagingBuffer, nullptr);
		vkFreeMemory(_vkDevice, stagingBufferMemory, nullptr);
	}

	void createDescriptorSetLayout()
	{
		VkDescriptorSetLayoutBinding uboLayoutBinding = {};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &uboLayoutBinding;

		if (vkCreateDescriptorSetLayout(_vkDevice, &layoutInfo, nullptr,
			&_descriptorSetLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	void createUniformBuffers()
	{
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		_uniformBuffers.resize(_swapChainImages.size());
		_uniformBuffersMemory.resize(_swapChainImages.size());

		for (size_t i = 0; i < _swapChainImages.size(); ++i)
		{
			createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				_uniformBuffers[i], _uniformBuffersMemory[i]);
		}
	}

	void createDescriptorPool()
	{
		VkDescriptorPoolSize poolSize = {};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = static_cast<uint32_t>(_swapChainImages.size());

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = static_cast<uint32_t>(_swapChainImages.size());
		poolInfo.flags = 0;
		
		if (vkCreateDescriptorPool(_vkDevice, &poolInfo, nullptr,
			&_descriptorPool)!=VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor pool!");
		}

	}

	void updateUniformBuffer(uint32_t currentImage)
	{
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();

		float time = std::chrono::duration<float,
			std::chrono::seconds::period>(currentTime-startTime).count();

		UniformBufferObject ubo = {};

		ubo.model = glm::rotate(glm::mat4(1.0f),time*glm::radians(90.0f),
			glm::vec3(0.0f,0.0f,1.0f));

		ubo.view = glm::lookAt(glm::vec3(2.0f,2.0f,2.0f),
			glm::vec3(0.0f,0.0f,0.0f),glm::vec3(0.0f,0.0f,1.0f));

		ubo.proj = glm::perspective(glm::radians(45.0f),
			_swapChainExtent.width / (float)_swapChainExtent.height,
			0.1f, 10.0f);

		ubo.proj[1][1] *= -1;

		void* data;
		vkMapMemory(_vkDevice, _uniformBuffersMemory[currentImage], 0,
			sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(_vkDevice, _uniformBuffersMemory[currentImage]);
	}

	void createDescriptorSets()
	{
		std::vector<VkDescriptorSetLayout>
			layouts(_swapChainImages.size(),_descriptorSetLayout);

		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = _descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(_swapChainImages.size());
		allocInfo.pSetLayouts = layouts.data();

		_descriptorSets.resize(_swapChainImages.size());
		if (vkAllocateDescriptorSets(_vkDevice, &allocInfo,
			&_descriptorSets[0]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate descriptor sets");
		}

		for (size_t i = 0; i < _swapChainImages.size(); ++i)
		{
			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = _uniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkWriteDescriptorSet descriptorWrite = {};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = _descriptorSets[i];
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = &bufferInfo;
			descriptorWrite.pImageInfo = nullptr;
			descriptorWrite.pTexelBufferView = nullptr;

			vkUpdateDescriptorSets(_vkDevice,1,&descriptorWrite,0,nullptr);
		}
	}

	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
	{
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferImageCopy region = {};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = {0,0,0};
		region.imageExtent = {width,height,1};

		vkCmdCopyBufferToImage(commandBuffer, buffer, image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		endSingleTimeCommands(commandBuffer);
	}

	void createTextureImage()
	{
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load("textures/texture.jpg",
			&texWidth,&texHeight,&texChannels,STBI_rgb_alpha);

		VkDeviceSize imageSize = texWidth * texHeight * 4;

		if (!pixels)
		{
			throw std::runtime_error("failed to load \
				texture image!");
		}

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(_vkDevice, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(_vkDevice, stagingBufferMemory);

		stbi_image_free(pixels);

		createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _textureImage, _textureImageMemory);

		transitionImageLayout(_textureImage, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		copyBufferToImage(stagingBuffer, _textureImage,
			static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

		transitionImageLayout(_textureImage, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(_vkDevice, stagingBuffer, nullptr);
		vkFreeMemory(_vkDevice, stagingBufferMemory, nullptr);
	}

	void createImage(uint32_t width, uint32_t height, VkFormat format,
		VkImageTiling tiling, VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties, VkImage& image,
		VkDeviceMemory& imageMemory)
	{
		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateImage(_vkDevice, &imageInfo, nullptr, &image)
			!= VK_SUCCESS)
		{
			throw std::runtime_error("failed to create image!");
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(_vkDevice, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(
			memRequirements.memoryTypeBits, properties);
		if (vkAllocateMemory(_vkDevice, &allocInfo, nullptr
			, &imageMemory) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate image memory!");
		}

		vkBindImageMemory(_vkDevice, image, imageMemory, 0);
	}

	VkCommandBuffer beginSingleTimeCommands()
	{
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = _commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(_vkDevice, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}

	void endSingleTimeCommands(VkCommandBuffer commandBuffer)
	{
		vkEndCommandBuffer(commandBuffer);
		
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(_graphicsQueue);

		vkFreeCommandBuffers(_vkDevice, _commandPool, 1, &commandBuffer);
	}

	void transitionImageLayout(VkImage image, VkFormat format,
		VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex= VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 1;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = 0;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
			newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
			newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else
		{
			throw std::invalid_argument("unsupported layout transition!");
		}

		vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr,
			0, nullptr, 1, &barrier);

		endSingleTimeCommands(commandBuffer);
	}
};

int main()
{
	HelloTriangleApplication app;
	try
	{
		app.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return 0;
}
