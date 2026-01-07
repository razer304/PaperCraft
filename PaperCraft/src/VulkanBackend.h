#pragma once
#include <vector>
#include <iostream>
#include <cstring>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <stdexcept>
#include <fstream>
#include <array>

#include <glm/glm.hpp>



#define NOMINMAX // To prevent windows.h from defining min and max macros so that limits can define max properly
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <cstdint> // Necessary for uint32_t
#include <limits> // Necessary for std::numeric_limits
#include <algorithm> // Necessary for std::clamp

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>


//these two for imgui
class ImguiModule;
#include "ImguiModule.h"



class VulkanBackend {
public:
    void runVulkanBackend();

	//these two for imgui
	ImguiModule imgui;   // Add this
	VulkanBackend() : imgui(*this) {}  // Construct it properly

	const uint32_t WIDTH = 800;
	const uint32_t HEIGHT = 600;

	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	#ifdef NDEBUG
		const bool enableValidationLayers = false;
	#else
		const bool enableValidationLayers = true;
	#endif

	GLFWwindow* window;
	VkInstance instance;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;

	VkQueue graphicsQueue;

	VkDescriptorPool descriptorPool;

	std::vector<VkImage> swapChainImages;

	VkRenderPass renderPass;

	const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0
	};

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;


		bool isComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};


	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

	QueueFamilyIndices queueFamilyIndicesprivate;

	VkCommandBuffer beginSingleTimeCommands();

	void endSingleTimeCommands(VkCommandBuffer commandBuffer);




private:

	//const char* vertexShaderPath = "C:/Users/razer/source/repos/PaperCraft/PaperCraft/src/shaders/vert.spv";
	//const char* fragmentShaderPath = "C:/Users/razer/source/repos/PaperCraft/PaperCraft/src/shaders/frag.spv";

	const char* vertexShaderPath = SHADER_DIR "/vert.spv";
	const char* fragmentShaderPath = SHADER_DIR "/frag.spv";


	const int MAX_FRAMES_IN_FLIGHT = 2;


	

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct Vertex {
		glm::vec2 pos;
		glm::vec3 color;

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

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
		{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
		{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
	};


	//make sure the data is aligned
	struct UniformBufferObject {
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
	};



	


	void initWindow();
    void initVulkan();
	void mainLoop();
	void cleanupVulkan();

	void createInstance();
	bool checkValidationLayerSupport();
	std::vector<const char*> getRequiredExtensions();
	void setupDebugMessenger();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);


	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

	void pickPhysicalDevice();

	bool isDeviceSuitable(VkPhysicalDevice device);
	int rateDeviceSuitability(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);


	void createLogicalDevice();

	void createSurface();

	void createSwapChain();

	void createImageViews();

	void createGraphicsPipeline();

	void createRenderPass();

	void createFramebuffers();

	void createCommandPool();

	void createCommandBuffers();

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	void drawFrame();

	void createSyncObjects();

	void recreateSwapChain();

	void cleanupSwapChain();

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);


	VkShaderModule createShaderModule(const std::vector<char>& code);


	static std::vector<char> readFile(const std::string& filename);

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	void createVertexBuffer();

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	void createIndexBuffer();

	void createDescriptorSetLayout();

	void createUniformBuffers();

	void updateUniformBuffer(uint32_t currentImage);

	void createDescriptorPool();

	void createDescriptorSets();



	VkDebugUtilsMessengerEXT debugMessenger;
	
	VkSurfaceKHR surface;
	VkQueue presentQueue;

	VkSwapchainKHR swapChain;
	
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	std::vector<VkImageView> swapChainImageViews;

	
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	std::vector<VkFramebuffer> swapChainFramebuffers;

	VkCommandPool commandPool;

	std::vector<VkCommandBuffer> commandBuffers;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;

	uint32_t currentFrame = 0;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;




	
	std::vector<VkDescriptorSet> descriptorSets;

	bool framebufferResized = false;


	const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};


};

