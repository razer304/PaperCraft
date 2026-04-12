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

#define GLM_ENABLE_EXPERIMENTAL

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

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include "tinyfiledialogs.h"

#include "inputhandler.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>



class VulkanBackend {
public:

	VulkanBackend() : input(this) {}

    void runVulkanBackend();

	InputHandler input;

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

	void VulkanBackend::initProject();

	void VulkanBackend::changenextline();


	const float edgePickThreshold = 0.1f; // Adjust as needed for sensitivity

	// Camera and interaction variables

	bool modelLoaded = false;

	int duplicateCount = 0;

	struct MeshVertex {
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec3 bary;

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription binding{};
			binding.binding = 0;
			binding.stride = sizeof(MeshVertex);
			binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			return binding;
		}

		static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 3> attrs{};

			// position
			attrs[0].binding = 0;
			attrs[0].location = 0;
			attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attrs[0].offset = offsetof(MeshVertex, pos);

			// normal
			attrs[1].binding = 0;
			attrs[1].location = 1;
			attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attrs[1].offset = offsetof(MeshVertex, normal);

			// barycentric coords
			attrs[2].binding = 0;
			attrs[2].location = 2;
			attrs[2].format = VK_FORMAT_R32G32B32_SFLOAT;
			attrs[2].offset = offsetof(MeshVertex, bary);


			return attrs;
		}
	};




	struct Mesh {
		
		//vertex buffer
		VkBuffer vertexBuffer{};
		VkDeviceMemory vertexMemory{};


		//index buffers
		VkBuffer fillindexBuffer{};
		VkDeviceMemory fillindexMemory{};

		VkBuffer lineindexBuffer{};
		VkDeviceMemory lineindexMemory{};

		//storage buffers
		VkBuffer SelectorStorageBuffer{};
		VkDeviceMemory SelectorStorageMemory{};

		VkBuffer DuplicateEdgeStorageBuffer{};
		VkDeviceMemory DuplicateEdgeStorageMemory{};

		VkBuffer DoneEdgeStorageBuffer{};
		VkDeviceMemory DoneEdgeStorageMemory{};




		uint32_t fillindexCount = 0;
		uint32_t lineindexCount = 0;
		uint32_t lineCount = 0;


		//uint32_t* vertPtr = nullptr;

		//uint32_t* fillindexPtr = nullptr;
		//uint32_t* lineindexPtr = nullptr;

		uint32_t* selectorPtr = nullptr;
		uint32_t* dupedgePtr = nullptr;
		uint32_t* doneedgePtr = nullptr;
		




		//cpu version
		//std::vector<MeshVertex> joinedVerticesCPU;
		//std::vector<uint32_t> joinedIndicesCPU;

		std::vector<MeshVertex> VerticesCPU;
		std::vector<uint32_t> fillIndicesCPU;
		std::vector<uint32_t> lineIndicesCPU;
		std::vector<uint32_t> duplicate_edgesCPU;



	};



	

	Mesh gMesh;

	std::array<uint32_t, 2> pickEdge(double mouseX, double mouseY);

	void vertexbuffer(std::vector<MeshVertex> vertices, Mesh& result);
	void fillindexbuffer(std::vector<uint32_t> fillindices, Mesh& result);
	void lineindexbuffer(std::vector<uint32_t> lineindices, Mesh& result);
	void selectorbuffer(Mesh& result);
	void doneedgesbuffer(Mesh& result);
	void edgebuffer(Mesh& result);

	void VulkanBackend::setSelector(int index);

	void updatevertexbuffer();

	bool VulkanBackend::check_facewithtwocuts(std::array<uint32_t, 3> face_vert_indicies);

	std::array < uint32_t, 3> VulkanBackend::facewithtwocuts(std::array<uint32_t, 3> face_vert_indicies);

	glm::quat VulkanBackend::getrotatefacedown(glm::vec3 srcnormal);

	bool VulkanBackend::rotatenonstillverts(std::vector<bool> stilllines, uint32_t face_vert_indicies, uint32_t nextline);


	std::array<uint32_t, 3> VulkanBackend::faceindex2verts(uint32_t nextface);
	std::array<uint32_t, 2> VulkanBackend::lineindex2verts(uint32_t nextline);


private:

	//const char* vertexShaderPath = "C:/Users/razer/source/repos/PaperCraft/PaperCraft/src/shaders/vert.spv";
	//const char* fragmentShaderPath = "C:/Users/razer/source/repos/PaperCraft/PaperCraft/src/shaders/frag.spv";

	const char* fill_vertexShaderPath = SHADER_DIR "/fill_vert.spv";
	const char* line_vertexShaderPath = SHADER_DIR "/line_vert.spv";
	const char* line_geometryShaderPath = SHADER_DIR "/line_geom.spv";
	const char* fragmentShaderPath = SHADER_DIR "/frag.spv";



	const int MAX_FRAMES_IN_FLIGHT = 2;


	
	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	/*
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
	*/
	

	//make sure the data is aligned
	struct UniformBufferObject {
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
	};


	bool VulkanBackend::rayHitsEdge(
		const glm::vec3& rayOrigin,
		const glm::vec3& rayDir,
		const glm::vec3& a,
		const glm::vec3& b,
		float threshold,
		float& outDist);

	bool VulkanBackend::rayTriangleHit(const glm::vec3& rayOrigin,const glm::vec3& rayDir,const glm::vec3& v0,const glm::vec3& v1,const glm::vec3& v2,float& outT,glm::vec3& outHit);


	float VulkanBackend::pointSegmentDistance(const glm::vec3& p,const glm::vec3& a,const glm::vec3& b);


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

	void createFilledGraohicsPipeline(VkPipelineShaderStageCreateInfo* shaderStages, VkPipelineVertexInputStateCreateInfo vertexInputInfo, VkPipelineInputAssemblyStateCreateInfo inputAssembly, VkPipelineViewportStateCreateInfo viewportState, VkPipelineDynamicStateCreateInfo dynamicState);

	void createLinedGraohicsPipeline(VkPipelineShaderStageCreateInfo* shaderStages, VkPipelineVertexInputStateCreateInfo vertexInputInfo, VkPipelineInputAssemblyStateCreateInfo inputAssembly, VkPipelineViewportStateCreateInfo viewportState, VkPipelineDynamicStateCreateInfo dynamicState);

	void flattenmesh();

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

	void setsixlines(std::vector<MeshVertex>& vertices, std::vector<uint32_t>& fillindices, std::vector<uint32_t>& lineindices, std::vector<uint32_t>& non_edges);


	VkShaderModule createShaderModule(const std::vector<char>& code);


	static std::vector<char> readFile(const std::string& filename);

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);


	bool rayHitsVertex(const glm::vec3& rayOrigin,const glm::vec3& rayDir,const glm::vec3& v,float threshold,float& outDist);


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
	
	VkPipeline graphicsPipeline_Filled;
	VkPipeline graphicsPipeline_Line;


	std::vector<VkFramebuffer> swapChainFramebuffers;

	VkCommandPool commandPool;

	std::vector<VkCommandBuffer> commandBuffers;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;

	uint32_t currentFrame = 0;

	std::vector<VkFence> imagesInFlight;


	//VkBuffer vertexBuffer;
	//VkDeviceMemory vertexBufferMemory;

	//VkBuffer indexBuffer;
	//VkDeviceMemory indexBufferMemory;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;

	UniformBufferObject uboCPU;

	


	
	std::vector<VkDescriptorSet> descriptorSets;

	bool framebufferResized = false;


	const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	const std::vector<const char*> instanceExtensions = {
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
	};


	// ImGui related functions
	void createImGuiDescriptorPool();
	void initImGui();
	void buildImGui();


	VkDescriptorPool imguiPool;



	

	



	


	void createVertexBuffer(Mesh& result, aiMesh* unjoinedmesh);
	void createIndexBuffer(Mesh& result, aiMesh* unjoinedmesh);

	void VulkanBackend::updateSelectorDescriptors(VkDeviceSize selectorSize);


	

	


	//void buildEdges(const aiMesh* mesh);

	Mesh loadMesh(const char* path);

	//static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

	//static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

	//static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);

	//void onScroll(double xoffset, double yoffset);
	//void onMouseButton(int button, int action, int mods);
	//void onCursorMove(double xpos, double ypos);








};

