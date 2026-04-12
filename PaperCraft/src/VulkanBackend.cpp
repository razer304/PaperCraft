#include "VulkanBackend.h"
#include "vk_initializers.h"
#include "inputhandler.h"
#include <glm/gtx/quaternion.hpp>


void VulkanBackend::runVulkanBackend() {
	if (enableValidationLayers) {
		std::cout << "- runVulkanBackend " << std::endl;
	}

	initProject();
	initWindow();
	initVulkan();
	mainLoop();
	cleanupVulkan();
}
//base Vulkan functions


void VulkanBackend::initProject() {
	if (enableValidationLayers) {
		std::cout << "Init Project " << std::endl;
	}
	input.installCallbacks(window);

}

void VulkanBackend::initWindow() {
	if (enableValidationLayers) {
		std::cout << "Init Window " << std::endl;
	}
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);



	window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

	glfwSetWindowUserPointer(window, this);


	glfwSetScrollCallback(window, input.scroll_callback);
	glfwSetMouseButtonCallback(window, input.mouse_button_callback);
	glfwSetCursorPosCallback(window, input.cursor_position_callback);


	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);




}

void VulkanBackend::initVulkan() {
	if (enableValidationLayers) {
		std::cout << "Init Vulkan " << std::endl;
	}

	createInstance();
	setupDebugMessenger();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain();



	createImageViews();
	createRenderPass();




	createDescriptorSetLayout();
	createGraphicsPipeline();
	createFramebuffers();
	createCommandPool();


	createUniformBuffers();

	createDescriptorPool();




	//initimgui descriptor pool
	createImGuiDescriptorPool();

	//createDescriptorSets();

	//initimgui
	initImGui();

	createCommandBuffers();
	createSyncObjects();



}

void VulkanBackend::mainLoop() {
	if (enableValidationLayers) {
		std::cout << "main loop " << std::endl;
	}
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		drawFrame();

	}

	vkDeviceWaitIdle(device);


}

void VulkanBackend::cleanupVulkan() {
	if (enableValidationLayers) {
		std::cout << "cleanup " << std::endl;
		DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	}


	cleanupSwapChain();

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroyBuffer(device, uniformBuffers[i], nullptr);
		vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
	}


	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	vkDestroyDescriptorPool(device, imguiPool, nullptr);


	vkDestroyDescriptorPool(device, descriptorPool, nullptr);

	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);


	vkDestroyBuffer(device, gMesh.lineindexBuffer, nullptr);
	vkFreeMemory(device, gMesh.lineindexMemory, nullptr);

	vkDestroyBuffer(device, gMesh.fillindexBuffer, nullptr);
	vkFreeMemory(device, gMesh.fillindexMemory, nullptr);

	vkDestroyBuffer(device, gMesh.SelectorStorageBuffer, nullptr);
	vkFreeMemory(device, gMesh.SelectorStorageMemory, nullptr);

	vkDestroyBuffer(device, gMesh.DuplicateEdgeStorageBuffer, nullptr);
	vkFreeMemory(device, gMesh.DuplicateEdgeStorageMemory, nullptr);

	vkDestroyBuffer(device, gMesh.vertexBuffer, nullptr);
	vkFreeMemory(device, gMesh.vertexMemory, nullptr);


	vkDestroyPipeline(device, graphicsPipeline_Filled, nullptr);
	vkDestroyPipeline(device, graphicsPipeline_Line, nullptr);

	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

	vkDestroyRenderPass(device, renderPass, nullptr);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(device, inFlightFences[i], nullptr);
	}

	vkDestroyCommandPool(device, commandPool, nullptr);




	vkDestroyDevice(device, nullptr);

	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);

	glfwDestroyWindow(window);

	glfwTerminate();

}


void VulkanBackend::drawFrame() {

	if (enableValidationLayers) {
		//std::cout << "- drawFrame " << currentFrame << std::endl;
	}





	vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);


	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
		framebufferResized = false;
		recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}


	if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
		vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}

	imagesInFlight[imageIndex] = inFlightFences[currentFrame];




	// Only reset the fence if we are submitting work
	vkResetFences(device, 1, &inFlightFences[currentFrame]);

	vkResetCommandBuffer(commandBuffers[currentFrame], 0);



	recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

	updateUniformBuffer(currentFrame);



	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };

	VkSwapchainKHR swapChains[] = { swapChain };

	VkSubmitInfo submitInfo = vkinit::submit_info(&commandBuffers[currentFrame], waitSemaphores, signalSemaphores, waitStages);



	VkPresentInfoKHR presentInfo = vkinit::present_info(swapChains, signalSemaphores, &imageIndex);



	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}


	result = vkQueuePresentKHR(presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}


	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

}


// InitVulkan helper functions

//mouse pointer funcitons

void VulkanBackend::setSelector(int index) {


	gMesh.selectorPtr[index] ^= 1;


}






std::array<uint32_t, 2> VulkanBackend::pickEdge(double mouseX, double mouseY) {
	// Convert mouse coords to NDC
	float ndcX = (2.0f * mouseX) / swapChainExtent.width - 1.0f;
	float ndcY = 1.0f - (2.0f * mouseY) / swapChainExtent.height;

	glm::mat4 invVP = glm::inverse(uboCPU.proj * uboCPU.view);

	// near point in world space
	glm::vec4 p0 = invVP * glm::vec4(ndcX, ndcY, 0.0f, 1.0f);
	p0 /= p0.w;

	// far point in world space
	glm::vec4 p1 = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
	p1 /= p1.w;

	glm::vec3 rayOrigin = glm::vec3(p0);
	glm::vec3 rayDir = glm::normalize(glm::vec3(p1 - p0));

	std::cout << "rayOrigin: " << rayOrigin.x << " " << rayOrigin.y << " " << rayOrigin.z << std::endl;
	std::cout << "rayDir: " << rayDir.x << " " << rayDir.y << " " << rayDir.z << std::endl;


	std::array<uint32_t, 2> closestEdge = { UINT32_MAX, UINT32_MAX };


	float closestDist = 1e9f;

	for (uint32_t i = 0; i < gMesh.lineindexCount; i += 3) {
		uint32_t i0 = gMesh.lineIndicesCPU[i + 0];
		uint32_t i1 = gMesh.lineIndicesCPU[i + 1];
		uint32_t i2 = gMesh.lineIndicesCPU[i + 2];

		glm::vec3 v0 = gMesh.VerticesCPU[i0].pos;
		glm::vec3 v1 = gMesh.VerticesCPU[i1].pos;
		glm::vec3 v2 = gMesh.VerticesCPU[i2].pos;

		// transform to world space using the same model matrix as the shader
		glm::vec3 w0 = glm::vec3(uboCPU.model * glm::vec4(v0, 1.0f));
		glm::vec3 w1 = glm::vec3(uboCPU.model * glm::vec4(v1, 1.0f));
		glm::vec3 w2 = glm::vec3(uboCPU.model * glm::vec4(v2, 1.0f));


		std::cout << "w0: " << w0.x << " " << w0.y << " " << w0.z << std::endl;
		std::cout << "w1: " << w1.x << " " << w1.y << " " << w1.z << std::endl;
		std::cout << "w2: " << w2.x << " " << w2.y << " " << w2.z << std::endl;



		float triT;
		glm::vec3 hitPoint;
		if (!rayTriangleHit(rayOrigin, rayDir, w0, w1, w2, triT, hitPoint))
			continue; // ray missed this triangle

		// Now measure distance from hitPoint to each edge
		float d01 = pointSegmentDistance(hitPoint, w0, w1);
		float d12 = pointSegmentDistance(hitPoint, w1, w2);
		float d20 = pointSegmentDistance(hitPoint, w2, w0);

		float localMin = d01;
		std::array<uint32_t, 2> localEdge = { i0, i1 };

		if (d12 < localMin) {
			localMin = d12;
			localEdge = { i1, i2 };
		}
		if (d20 < localMin) {
			localMin = d20;
			localEdge = { i2, i0 };
		}

		if (localMin < edgePickThreshold && triT < closestDist) {
			closestDist = triT;
			closestEdge = localEdge;
		}


	}


	std::cout << "closest edge: " << closestEdge[0] << ", " << closestEdge[1] << std::endl;


	return closestEdge;


}


bool VulkanBackend::rayHitsEdge(
	const glm::vec3& rayOrigin,
	const glm::vec3& rayDir,
	const glm::vec3& a,
	const glm::vec3& b,
	float threshold,
	float& outDist)
{
	glm::vec3 ab = b - a;
	glm::vec3 ao = a - rayOrigin;

	float ab_len2 = glm::dot(ab, ab);
	float d_ab = glm::dot(rayDir, ab);
	float d_ao = glm::dot(rayDir, ao);
	float ab_ao = glm::dot(ab, ao);

	float denom = ab_len2 - d_ab * d_ab;
	if (fabs(denom) < 1e-12f)
		return false;

	float t = (ab_len2 * d_ao - d_ab * ab_ao) / denom;
	float u = (ab_ao + t * d_ab) / ab_len2;




	std::cout << "t=" << t << " u=" << u << "\n";


	if (t < 0.0f || u < 0.0f || u > 1.0f)
		return false;

	glm::vec3 pRay = rayOrigin + t * rayDir;
	glm::vec3 pSeg = a + u * ab;

	float dist = glm::length(pRay - pSeg);

	std::cout << "t=" << t << " u=" << u << " dist=" << dist << "\n";

	if (dist < threshold) {
		outDist = t;
		return true;
	}

	return false;
}

bool VulkanBackend::rayTriangleHit(
	const glm::vec3& rayOrigin,
	const glm::vec3& rayDir,
	const glm::vec3& v0,
	const glm::vec3& v1,
	const glm::vec3& v2,
	float& outT,
	glm::vec3& outHit)
{
	const float EPS = 1e-6f;

	glm::vec3 e1 = v1 - v0;
	glm::vec3 e2 = v2 - v0;

	glm::vec3 pvec = glm::cross(rayDir, e2);
	float det = glm::dot(e1, pvec);
	if (fabs(det) < EPS) return false;

	float invDet = 1.0f / det;
	glm::vec3 tvec = rayOrigin - v0;

	float u = glm::dot(tvec, pvec) * invDet;
	if (u < 0.0f || u > 1.0f) return false;

	glm::vec3 qvec = glm::cross(tvec, e1);
	float v = glm::dot(rayDir, qvec) * invDet;
	if (v < 0.0f || u + v > 1.0f) return false;

	float t = glm::dot(e2, qvec) * invDet;
	if (t < 0.0f) return false;

	outT = t;
	outHit = rayOrigin + t * rayDir;
	return true;
}

float VulkanBackend::pointSegmentDistance(
	const glm::vec3& p,
	const glm::vec3& a,
	const glm::vec3& b)
{
	glm::vec3 ab = b - a;
	float ab2 = glm::dot(ab, ab);
	if (ab2 < 1e-12f) return glm::length(p - a);

	float t = glm::dot(p - a, ab) / ab2;
	t = glm::clamp(t, 0.0f, 1.0f);

	glm::vec3 proj = a + t * ab;
	return glm::length(p - proj);
}



bool VulkanBackend::rayHitsVertex(
	const glm::vec3& rayOrigin,
	const glm::vec3& rayDir,
	const glm::vec3& v,
	float threshold,
	float& outDist)
{
	// Project (v - rayOrigin) onto rayDir to get t
	glm::vec3 toV = v - rayOrigin;
	float t = glm::dot(toV, rayDir);

	// Behind the camera → ignore
	if (t < 0.0f) return false;

	// Closest point on ray
	glm::vec3 closestPoint = rayOrigin + t * rayDir;

	// Distance from vertex to ray
	float dist = glm::length(closestPoint - v);

	if (dist < threshold) {
		outDist = t;
		return true;
	}

	return false;
}




//imgui helper functions


void VulkanBackend::createImGuiDescriptorPool() {

	VkDescriptorPoolSize pool_sizes[] = {
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	if (vkCreateDescriptorPool(device, &pool_info, nullptr, &imguiPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create ImGui descriptor pool");
	}


}

void VulkanBackend::initImGui() {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui_ImplGlfw_InitForVulkan(window, true);

	ImGui_ImplVulkan_InitInfo init_info{};
	init_info.Instance = instance;
	init_info.PhysicalDevice = physicalDevice;
	init_info.Device = device;
	init_info.QueueFamily = queueFamilyIndicesprivate.graphicsFamily.value();
	init_info.Queue = graphicsQueue;
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = 2;
	init_info.ImageCount = swapChainImages.size();
	init_info.PipelineInfoMain.RenderPass = renderPass;

	init_info.PipelineInfoMain.Subpass = 0;
	init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;



	ImGui_ImplVulkan_Init(&init_info);

	// Upload fonts
	VkCommandBuffer cmd = beginSingleTimeCommands();

	if (enableValidationLayers) {

		VkDebugUtilsObjectNameInfoEXT nameInfo = vkinit::debug_name_create_info(VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)cmd, "ImGuicommandBuffer");

		PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");

		if (vkSetDebugUtilsObjectNameEXT) {
			vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
		}


	}


	//ImGui_ImplVulkan_CreateFontsTexture(cmd);

	endSingleTimeCommands(cmd);

	// No destroy call anymore
	//ImGui_ImplVulkan_DestroyFontUploadObjects();

}



void VulkanBackend::flattenmesh() {


	std::cout << "- flatten mesh start" << std::endl;

	//cut edge is any edge / selected / done face

	//looop through faces


	//std::vector<MeshVertex> transformedVertices = gMesh.VerticesCPU;

	int vertsize = gMesh.VerticesCPU.size();



	for (size_t facestartvert = 0; facestartvert < vertsize; facestartvert = facestartvert + 3)
	{
		std::vector<bool> stillverts(vertsize, false);


		std::cout << " ALL FACES LOOP START: " << facestartvert << std::endl;

		if (gMesh.doneedgePtr[facestartvert] != 1)
		{
			std::cout << " ALL FACES LOOP not done: " << std::endl;


			std::array<uint32_t, 3> face_vert_indicies = { facestartvert, facestartvert + 1, facestartvert + 2 };

			//if face has 2 cut edges (edge / done / selected)

			std::cout << " pre check facestartvert: " << facestartvert << std::endl;


			std::cout << " pre check face_vert_indicies: " << face_vert_indicies[0] << ", " << face_vert_indicies[1] << ", " << face_vert_indicies[2] << std::endl;


			if (check_facewithtwocuts(face_vert_indicies)) {

				std::array < uint32_t, 3> cut_edges = facewithtwocuts(face_vert_indicies);

				std::cout << "cut edges " << cut_edges[0] << ", " << cut_edges[1] << ", " << cut_edges[2] << std::endl;




				std::cout << "checking vert: " << face_vert_indicies[0] << std::endl;
				std::cout << "pre check normal: " << gMesh.VerticesCPU[face_vert_indicies[0]].normal.x << ", " << gMesh.VerticesCPU[face_vert_indicies[0]].normal.y << ", " << gMesh.VerticesCPU[face_vert_indicies[0]].normal.z << std::endl;

				//if face normal is not already pointing downwards
				if (gMesh.VerticesCPU[face_vert_indicies[0]].normal != glm::vec3{ 0, 0, -1 }) {
					std::cout << "pre rotate normal: " << gMesh.VerticesCPU[face_vert_indicies[0]].normal.x << ", " << gMesh.VerticesCPU[face_vert_indicies[0]].normal.y << ", " << gMesh.VerticesCPU[face_vert_indicies[0]].normal.z << std::endl;


					//gets the quaternion that rotates the face normal to point downwards
					glm::quat down_quaternion = getrotatefacedown(gMesh.VerticesCPU[face_vert_indicies[0]].normal);

					std::cout << "down quaternion: " << down_quaternion.x << ", " << down_quaternion.y << ", " << down_quaternion.z << ", " << down_quaternion.w << std::endl;


					//rotate whole mesh (includeing normals) so that face is flat on ground
					for (auto& v : gMesh.VerticesCPU)
					{
						v.pos = down_quaternion * v.pos;
						v.normal = glm::normalize(down_quaternion * v.normal);
					}

					std::cout << "transformed normal: " << gMesh.VerticesCPU[face_vert_indicies[0]].normal.x << ", " << gMesh.VerticesCPU[face_vert_indicies[0]].normal.y << ", " << gMesh.VerticesCPU[face_vert_indicies[0]].normal.z << std::endl;



					uint32_t nextline;
					uint32_t nextface;

					

					for (size_t i = 0; i < 3; i++)
					{
						// mark lines of face 1 as done
						std::cout << " doneline: " << face_vert_indicies[i] << std::endl;
						gMesh.doneedgePtr[face_vert_indicies[i]] = 1;

						stillverts[face_vert_indicies[i]] = true;

						//gets the next face that is connected to the non cut edge of the current face
						if (cut_edges[i] == 0)
						{
							//floor should make it round down
							
							nextline = gMesh.dupedgePtr[face_vert_indicies[i]];

							std::cout << " dup doneline: " << nextline << std::endl;
							gMesh.doneedgePtr[nextline] = 1;

							nextface = std::floor(nextline / 3);


							std::cout << "curr face: " << std::floor(face_vert_indicies[i] / 3) << std::endl;
							std::cout << "next face: " << nextface << std::endl;

						}

					}
					

					rotatenonstillverts(stillverts, nextface, nextline);

				}

			}



		}
		else
		{
			std::cout << " ALL FACES LOOP done: " << std::endl;
		}
	}


	
	//get connected line and then face of non cut edge
	//rotate whole mesh except for done ones along the axis of that line until the face 2 is also flat on xp plane
	// mark face 2 as done
	//check if next face has 2 cut edges, if so repeat the process until all faces are done
	
	
	//if face has 0 cut edges the thing is finnished and we can stop
	//if face has 1 cut edge continue in looping through faces

	//for (size_t i = 0; i < gMesh.vertexBuffer.size(); i++)
	//{

	//}




	std::cout << "LIST OF DONES" << std::endl;

	for (size_t done_lines = 0; done_lines < gMesh.lineCount; done_lines++)
	{
		
		std::cout << "line: "<< done_lines << " done: " << gMesh.doneedgePtr[done_lines] << std::endl;


	}




	//gMesh.VerticesCPU = transformedVertices;


	updatevertexbuffer();


}


std::array<uint32_t, 3> VulkanBackend::faceindex2verts(uint32_t nextface) {

	return { nextface * 3, nextface * 3 + 1, nextface * 3 + 2 };
}

std::array<uint32_t, 2> VulkanBackend::lineindex2verts(uint32_t nextline) {

	return { gMesh.lineIndicesCPU[nextline * 2], gMesh.lineIndicesCPU[nextline * 2 + 1] };
}




bool VulkanBackend::rotatenonstillverts(std::vector<bool> stillverts, uint32_t nextface, uint32_t rotateline) {

	std::array<uint32_t, 3> face_vert_indicies = faceindex2verts(nextface);
	std::array<uint32_t, 2> line_vert_indicies = lineindex2verts(rotateline);




	std::cout << " next face vert-indicie v0: " << face_vert_indicies[0] << ", vert-pos:  x:" << gMesh.VerticesCPU[face_vert_indicies[0]].pos.x << ", y:" << gMesh.VerticesCPU[face_vert_indicies[0]].pos.y << ", z:" << gMesh.VerticesCPU[face_vert_indicies[0]].pos.z << std::endl;
	std::cout << " next face vert-indicie v1: " << face_vert_indicies[1] << ", vert-pos:  x:" << gMesh.VerticesCPU[face_vert_indicies[1]].pos.x << ", y:" << gMesh.VerticesCPU[face_vert_indicies[1]].pos.y << ", z:" << gMesh.VerticesCPU[face_vert_indicies[1]].pos.z << std::endl;
	std::cout << " next face vert-indicie v2: " << face_vert_indicies[2] << ", vert-pos:  x:" << gMesh.VerticesCPU[face_vert_indicies[2]].pos.x << ", y:" << gMesh.VerticesCPU[face_vert_indicies[2]].pos.y << ", z:" << gMesh.VerticesCPU[face_vert_indicies[2]].pos.z << std::endl;


	std::cout << " rotaty line-indicie v0: " << line_vert_indicies[0] << ", line-pos: x:" << gMesh.VerticesCPU[line_vert_indicies[0]].pos.x << ", y:" << gMesh.VerticesCPU[line_vert_indicies[0]].pos.y << ", z:" << gMesh.VerticesCPU[line_vert_indicies[0]].pos.z << std::endl;
	std::cout << " rotaty line-indicie v1: " << line_vert_indicies[1] << ", line-pos: x:" << gMesh.VerticesCPU[line_vert_indicies[1]].pos.x << ", y:" << gMesh.VerticesCPU[line_vert_indicies[1]].pos.y << ", z:" << gMesh.VerticesCPU[line_vert_indicies[1]].pos.z << std::endl;

	//TODO: get good quat
	glm::quat rotate_quaternion;



	for (size_t i = 0; i < gMesh.VerticesCPU.size(); i++)
	{
		if (!stillverts[i])
		{
			//gMesh.VerticesCPU[i].pos = rotate_quaternion * gMesh.VerticesCPU[i].pos;
			//gMesh.VerticesCPU[i].normal = glm::normalize(rotate_quaternion * gMesh.VerticesCPU[i].normal);

		}
	}




	//add lines of the new face to stilllines
	//recurse if there is a rotate line on the next face (theres 2 edges)



	return true;
}


bool VulkanBackend::check_facewithtwocuts(std::array<uint32_t, 3> face_vert_indicies) {

	std::cout << "face_vert_indicies: " << face_vert_indicies[0] << ", " << face_vert_indicies[1] << ", " << face_vert_indicies[2] << std::endl;

	uint32_t cuts = 0;
	for (size_t i = 0; i < 3; i++)
	{

		bool duplicate_edges = (gMesh.dupedgePtr[face_vert_indicies[i]] != -1);
		bool select_edges = (gMesh.selectorPtr[face_vert_indicies[i]] == 1);


		std::cout << "line: " << face_vert_indicies[i] << " dup edge: " << gMesh.dupedgePtr[face_vert_indicies[i]] << " select: " << gMesh.selectorPtr[face_vert_indicies[i]] << std::endl;




		if (!duplicate_edges || select_edges)
		{
			std::cout << "CUT EDGE line: " << face_vert_indicies[i] << std::endl;

			cuts++;
		}
	}


	std::cout << "CUTS: " << cuts << std::endl;

	if (cuts == 2)
	{
		std::cout << "2 CUTS: " << std::endl;
		return true;
	}
	else {

		std::cout << "NOT 2 CUTS: " << std::endl;
		return false;
	}


}


std::array < uint32_t, 3> VulkanBackend::facewithtwocuts(std::array<uint32_t, 3> face_vert_indicies)
{
	std::array < uint32_t, 3> cut_edges = { 0,0,0 };

	for (size_t i = 0; i < 3; i++)
	{

		bool duplicate_edges = (gMesh.dupedgePtr[face_vert_indicies[i]] != -1);
		bool select_edges = (gMesh.selectorPtr[face_vert_indicies[i]] == 1);



		if (!duplicate_edges || select_edges)
		{
			std::cout << "CUT EDGE line: " << face_vert_indicies[i] << std::endl;

			cut_edges[i] = 1;
		}

	}

	return cut_edges;

}


glm::quat VulkanBackend::getrotatefacedown(glm::vec3 srcnormal) {


	glm::vec3 downnormal = {0, 0, -1};


	float dot = glm::dot(glm::normalize(srcnormal), downnormal);

	// If vectors are opposite (dot ≈ -1), glm::rotation fails
	if (dot < -0.9999f)
	{
		// Rotate 180 degrees around ANY axis perpendicular to srcnormal
		glm::vec3 axis = glm::normalize(glm::cross(srcnormal, glm::vec3(1, 0, 0)));
		if (glm::length(axis) < 0.001f)
			axis = glm::normalize(glm::cross(srcnormal, glm::vec3(0, 1, 0)));

		return glm::angleAxis(glm::pi<float>(), axis);
	}

	 return glm::rotation(srcnormal,downnormal);


}






void VulkanBackend::buildImGui() {

	if (enableValidationLayers) {
		//std::cout << "- buildImGui " << std::endl;
	}


	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// UI: Import FBX button
	if (ImGui::Button("Import 3D Obj")) {
		const char* filters[] = { "*" };
		const char* file = tinyfd_openFileDialog(
			"Choose 3D File",
			"",
			1,
			filters,
			nullptr,
			0
		);
		if (file) {
			std::cout << "Selected 3D File: " << file << std::endl;
			gMesh = loadMesh(file);

			createDescriptorSets();

			VkDeviceSize selectorSize = (gMesh.lineCount) * sizeof(uint32_t);
			updateSelectorDescriptors(selectorSize);

		}
	}


	if (ImGui::Button("Flatten")) {
		//todo:
		flattenmesh();


	}

	if (ImGui::Button("proceed")) {
		//iterate line picking algorythm

		changenextline();

	}



	// Display selected edge info
	ImGui::Begin("selector");

	//ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 6));
	int line_counter = 0;

	//std::cout << "gui thingy" << std::endl;
	//std::cout << "   " << std::endl;

	for (size_t i = 0; i < gMesh.lineCount; i++)
	{
		bool selected = (gMesh.selectorPtr[i] == 1);
		bool edge = (gMesh.dupedgePtr[i] == -1);



		if (edge) continue;

		//std::cout << "edge: " << i << " is " << edge << "which is " << gMesh.dupedgePtr[i] << std::endl;
		//std::cout << "selector: " << i << " is " << selected << "which is " << gMesh.selectorPtr[i] << std::endl;


		if (gMesh.dupedgePtr[i] < i) continue;


		line_counter++;

		std::string label = "Line " + std::to_string(line_counter);

		if (ImGui::Checkbox(label.c_str(), &selected)) {
			gMesh.selectorPtr[i] = selected;
			gMesh.selectorPtr[gMesh.dupedgePtr[i]] = selected;
		}


		//std::cout << "selector: " << i << " is " << selected << "which is " << gMesh.selectorPtr[i] << std::endl;

		//std::cout << "edge: " << i << " is " << edge << "which is " << gMesh.edgePtr[i] << std::endl;

	}
	//ImGui::PopStyleVar();
	ImGui::End();


	ImGui::Render();
}


void VulkanBackend::changenextline() {









}

//single time command buffer helper functions

VkCommandBuffer VulkanBackend::beginSingleTimeCommands() {


	VkCommandBufferAllocateInfo allocInfo = vkinit::command_buffer_allocate_info(commandPool, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);






	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);


	if (enableValidationLayers) {

		VkDebugUtilsObjectNameInfoEXT nameInfo = vkinit::debug_name_create_info(VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)commandBuffer, "commandBuffer");

		PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");

		if (vkSetDebugUtilsObjectNameEXT) {
			vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
		}


	}



	VkCommandBufferBeginInfo beginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);


	vkBeginCommandBuffer(commandBuffer, &beginInfo);




	return commandBuffer;
}

void VulkanBackend::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}


//model loader helper functions


VulkanBackend::Mesh VulkanBackend::loadMesh(const char* path) {

	if (enableValidationLayers) {
		std::cout << "- loadMesh " << currentFrame << std::endl;
	}

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(
		path,
		aiProcess_Triangulate |
		aiProcess_FixInfacingNormals |
		aiProcess_SortByPType |
		aiProcess_GenNormals
	);


	VulkanBackend::Mesh result{}; // make sure this exists before use

	/*
	if (!unjoinedscene || !unjoinedscene->HasMeshes()) {
		std::cerr << "Failed to load mesh: " << path << std::endl;
		return result;
	}
	*/


	aiMesh* mesh = scene->mMeshes[0]; // just take the first mesh



		std::cout << "vertex 0 normals: " << mesh[0].mNormals << std::endl;

	createVertexBuffer(result, mesh);
	//createIndexBuffers(result, mesh);


	if (enableValidationLayers) {
		std::cout << "selector count: " << result.lineCount << std::endl;
		//VK_NULL_HANDLE
		if (result.SelectorStorageBuffer == NULL)
		{
			std::cout << "selector i snull: " << std::endl;

		}
		else
		{
			std::cout << "selector is not null: " << std::endl;

		}

	}


	// --- Final mesh info ---




	modelLoaded = true;

	return result;
}

void VulkanBackend::setsixlines(std::vector<MeshVertex>& vertices, std::vector<uint32_t>& fillindices, std::vector<uint32_t>& lineindices, std::vector<uint32_t>& duplicate_edges) {



	int choose1[] = { 3,2,1 };
	int choose2[] = { 2,1,3 };


	uint32_t lineindex = -1;

	for (size_t f = 0; f < 3; f++)
	{



		//lineindices.push_back(fillindices[fillindices.size() - choose1[f]]);
		//lineindices.push_back(fillindices[fillindices.size() - choose2[f]]);





		if (enableValidationLayers) {

			std::cout << "- for every line in the 3 verts " << std::endl;

		}




		if (lineindices.size() >= 2)
		{
			if (enableValidationLayers) {

				std::cout << "- pre verts " << std::endl;
				std::cout << "- line indicies.size " << lineindices.size() << std::endl;
				std::cout << "- prev line 1 " << (lineindices[lineindices.size() - 2]) << std::endl;
				std::cout << "- prev line 2 " << (lineindices[lineindices.size() - 1]) << std::endl;
				std::cout << "- curr line 1 " << (vertices.size() - choose1[f]) << std::endl;
				std::cout << "- curr line 2 " << (vertices.size() - choose2[f]) << std::endl;
			}

			bool duplicate = FALSE;

			

			for (size_t i = 0; i < lineindices.size(); i += 2)
			{

				if (!duplicate) {

					MeshVertex check_vertice_prev_1 = vertices[lineindices[i]];
					MeshVertex check_vertice_prev_2 = vertices[lineindices[i + 1]];


					MeshVertex check_vertice_curr_1 = vertices[(vertices.size() - choose1[f])];
					MeshVertex check_vertice_curr_2 = vertices[(vertices.size() - choose2[f])];




					if (enableValidationLayers) {
						std::cout << "- post check allocations " << i << std::endl;
					}


					if ((check_vertice_prev_1.pos == check_vertice_curr_1.pos && check_vertice_prev_2.pos == check_vertice_curr_2.pos) || (check_vertice_prev_1.pos == check_vertice_curr_2.pos && check_vertice_prev_2.pos == check_vertice_curr_1.pos))
					{
						if (enableValidationLayers) {
							std::cout << "- pos equal " << std::endl;
						}

						duplicate = TRUE;

						lineindex = i/2;


						duplicateCount = duplicateCount + 1;

						std::cout << "- pos equal " << lineindex << std::endl;

					}
				}
			}


			if (!duplicate) {

				std::cout << "- no duplicate found, adding line: " << std::endl;
				lineindices.push_back(fillindices[fillindices.size() - choose1[f]]);
				lineindices.push_back(fillindices[fillindices.size() - choose2[f]]);
				duplicate_edges.push_back(-1);
				
			}
			else{
				//set new storage buffer for edges for this line too 0
				//make sure it is the check_vertice_prev line that is being set to 0 cuz current line isnt being added to line indicies
				//make sure to init the edges storage buffer to 1
				// then xor the selector and edges buffers together for thingy to happen
				//or display edges in different colour that cannot be overridden by selector
				// and make it so that the imgui dosnt show lines that are edges and therefore cannot be affected by selector
				//TODO: BLOOPS

				std::cout << "- duplicate found, not adding line " << std::endl;

				lineindices.push_back(fillindices[fillindices.size() - choose1[f]]);
				lineindices.push_back(fillindices[fillindices.size() - choose2[f]]);


				


				if (lineindex != -1) {
					//non_edges.push_back(lineindex);

					duplicate_edges.push_back(lineindex);
					duplicate_edges[lineindex] = duplicate_edges.size()-1;
					std::cout << "- non edge: " << duplicate_edges[duplicate_edges.size() - 1] << std::endl;
				}
				

			}

		}
		else {
			if (enableValidationLayers) {
				std::cout << "- first line " << std::endl;
			}

			lineindices.push_back(fillindices[fillindices.size() - choose1[f]]);
			lineindices.push_back(fillindices[fillindices.size() - choose2[f]]);
			duplicate_edges.push_back(-1);
		}

	}
}



		void VulkanBackend::createVertexBuffer(Mesh & result, aiMesh * mesh) {

			if (enableValidationLayers) {
				std::cout << "- createVertexBuffer " << std::endl;
			}



			// Collect vertices (x,y,z as floats)
			std::vector<MeshVertex> vertices;
			std::vector<uint32_t> fillindices;
			std::vector<uint32_t> lineindices;
			std::vector<uint32_t> duplicate_edges;


			vertices.reserve(mesh->mNumFaces * 3);
			fillindices.reserve(mesh->mNumFaces * 3);
			lineindices.reserve(mesh->mNumFaces * 3);


			for (unsigned int i = 0; i < mesh->mNumFaces; i++) {

				aiFace& face = mesh->mFaces[i];

				for (unsigned int j = 0; j < face.mNumIndices; j++)
				{
					unsigned int idx = face.mIndices[j];

					MeshVertex v{};

					v.pos = glm::vec3(
						mesh->mVertices[idx].x,
						mesh->mVertices[idx].y,
						mesh->mVertices[idx].z);
					v.normal = glm::vec3(
						mesh->mNormals[idx].x,
						mesh->mNormals[idx].y,
						mesh->mNormals[idx].z);

					// Assign barycentrics based on j (0,1,2) 
					if (j == 0)
						v.bary = glm::vec3(1, 0, 0);
					else if (j == 1) v.bary = glm::vec3(0, 1, 0);
					else v.bary = glm::vec3(0, 0, 1);


					vertices.push_back(v);
					fillindices.push_back(vertices.size() - 1);

					if (fillindices.size() % 3 == 0) {
						//line indicies

						setsixlines(vertices, fillindices, lineindices, duplicate_edges);



					}
				}
			}


			std::cout << "- verts : " << std::endl;


			for (size_t i = 0; i < vertices.size(); i++)
			{
				std::cout << " - pos : " << vertices[i].pos.x << ", " << vertices[i].pos.y << ", " << vertices[i].pos.z << std::endl;
				std::cout << " - normal : " << vertices[i].normal.x << ", " << vertices[i].normal.y << ", " << vertices[i].normal.z << std::endl;

			}



			std::cout << "- fill indices: " << std::endl;


			for (size_t i = 0; i < fillindices.size(); i++)
			{
				std::cout << "- : " << i << ", " << fillindices[i] << std::endl;

			}

			std::cout << "- line indices: " << std::endl;

			for (size_t i = 0; i < lineindices.size(); i++)
			{
				std::cout << "- : " << i << ", " << lineindices[i] << std::endl;

			}

			std::cout << "- edge indices: " << std::endl;

			for (size_t i = 0; i < duplicate_edges.size(); i++)
			{
				std::cout << "- : " << i << ", " << duplicate_edges[i] << std::endl;

			}

			result.VerticesCPU = vertices;
			result.fillIndicesCPU = fillindices;
			result.lineIndicesCPU = lineindices;
			result.duplicate_edgesCPU = duplicate_edges;


			result.fillindexCount = static_cast<uint32_t>(fillindices.size());
			result.lineindexCount = static_cast<uint32_t>(lineindices.size());
			result.lineCount = static_cast<uint32_t>(lineindices.size() / 2);



			std::cout << "line indices size: " << lineindices.size() << std::endl;
			std::cout << "line indices count: " << result.lineCount << std::endl;




			//vertex buffer
			vertexbuffer(vertices, result);

			//INDEX BUFFER STUFFS!!!!!
			fillindexbuffer(fillindices, result);


			lineindexbuffer(lineindices, result);

			std::cout << "selector start" << std::endl;
			selectorbuffer(result);

			std::cout << "selector end" << std::endl;


			std::cout << "doneedges start" << std::endl;
			doneedgesbuffer(result);

			std::cout << "doneedges end" << std::endl;




			std::cout << "edge start" << std::endl;
			edgebuffer(result);


			std::cout << "edge end" << std::endl;

		}


		void VulkanBackend::vertexbuffer(std::vector<MeshVertex> vertices, Mesh & result) {

			std::cout << "vertexbuffer start" << std::endl;


			VkDeviceSize vertexSize = vertices.size() * sizeof(MeshVertex);

			VkBuffer stagingBuffer;
			VkDeviceMemory stagingMemory;

			createBuffer(
				vertexSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				stagingBuffer,
				stagingMemory
			);







			//vkMapMemory(device, result.DuplicateEdgeStorageMemory, 0, edgeSize, 0, (void**)&result.dupedgePtr);


			void* data;
			vkMapMemory(device, stagingMemory, 0, vertexSize, 0, &data);
			memcpy(data, vertices.data(), static_cast<size_t>(vertexSize));
			vkUnmapMemory(device, stagingMemory);

			createBuffer(
				vertexSize,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				result.vertexBuffer,
				result.vertexMemory
			);

			copyBuffer(stagingBuffer, result.vertexBuffer, vertexSize);

			vkDestroyBuffer(device, stagingBuffer, nullptr);
			vkFreeMemory(device, stagingMemory, nullptr);

		}



		void VulkanBackend::updatevertexbuffer() {
		
		
			std::cout << "updatevertexbuffer start" << std::endl;


			VkDeviceSize vertexSize = gMesh.VerticesCPU.size() * sizeof(MeshVertex);




			VkBuffer stagingBuffer;
			VkDeviceMemory stagingMemory;

			createBuffer(
				vertexSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				stagingBuffer,
				stagingMemory
			);



			void* data;
			vkMapMemory(device, stagingMemory, 0, vertexSize, 0, &data);
			memcpy(data, gMesh.VerticesCPU.data(), static_cast<size_t>(vertexSize));
			vkUnmapMemory(device, stagingMemory);

			vkDestroyBuffer(device, gMesh.vertexBuffer, nullptr);
			//vkUnmapMemory(device, gMesh.vertexMemory);



			createBuffer(
				vertexSize,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				gMesh.vertexBuffer,
				gMesh.vertexMemory
			);

			copyBuffer(stagingBuffer, gMesh.vertexBuffer, vertexSize);

			vkDestroyBuffer(device, stagingBuffer, nullptr);
			vkFreeMemory(device, stagingMemory, nullptr);
		
		}


		void VulkanBackend::fillindexbuffer(std::vector<uint32_t> fillindices, Mesh & result) {
			std::cout << "fill indexbuffer start" << std::endl;


			VkBuffer stagingBuffer;
			VkDeviceMemory stagingMemory;

			VkDeviceSize findexSize = fillindices.size() * sizeof(uint32_t);

			//unjoined index buffer creatign
			createBuffer(
				findexSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				stagingBuffer,
				stagingMemory
			);

			void* data;


			vkMapMemory(device, stagingMemory, 0, findexSize, 0, &data);
			memcpy(data, fillindices.data(), static_cast<size_t>(findexSize));
			vkUnmapMemory(device, stagingMemory);

			createBuffer(
				findexSize,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				result.fillindexBuffer,
				result.fillindexMemory
			);

			copyBuffer(stagingBuffer, result.fillindexBuffer, findexSize);


			vkDestroyBuffer(device, stagingBuffer, nullptr);
			vkFreeMemory(device, stagingMemory, nullptr);

		}


		void VulkanBackend::lineindexbuffer(std::vector<uint32_t> lineindices, Mesh & result) {
			std::cout << "line indexbuffer start" << std::endl;


			VkBuffer stagingBuffer;
			VkDeviceMemory stagingMemory;

			VkDeviceSize lindexSize = lineindices.size() * sizeof(uint32_t);

			//std::cout << "line indices size: " << lineindices.size() << std::endl;
			//result.lineindexCount = static_cast<uint32_t>(lineindices.size());


			//unjoined index buffer creatign
			createBuffer(
				lindexSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				stagingBuffer,
				stagingMemory
			);

			void* data;


			vkMapMemory(device, stagingMemory, 0, lindexSize, 0, &data);
			memcpy(data, lineindices.data(), static_cast<size_t>(lindexSize));
			vkUnmapMemory(device, stagingMemory);

			createBuffer(
				lindexSize,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				result.lineindexBuffer,
				result.lineindexMemory
			);

			copyBuffer(stagingBuffer, result.lineindexBuffer, lindexSize);


			vkDestroyBuffer(device, stagingBuffer, nullptr);
			vkFreeMemory(device, stagingMemory, nullptr);


			//SELECTOR BUFFER CREATION


			

		}






		void VulkanBackend::selectorbuffer(Mesh & result) {

			std::vector<uint32_t> selector(result.lineCount, 0);


			VkDeviceSize selectorSize = selector.size() * sizeof(uint32_t);

			createBuffer(
				selectorSize,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, // or whatever you bind it as
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				result.SelectorStorageBuffer,
				result.SelectorStorageMemory
			);






			vkMapMemory(device, result.SelectorStorageMemory, 0, selectorSize, 0, (void**)&result.selectorPtr);


			//test
			//uint32_t testselectorValues[] = {1, 0, 0, 0};
			//uint32_t testselectorValues[] = {0, 0, 0, 1, 1, 1};




			auto* check = static_cast<uint32_t*>(result.selectorPtr);





			std::cout << "selector size: " << selectorSize << std::endl;
			std::cout << "selector count: " << result.lineCount << std::endl;

			//memcpy(result.selectorPtr, testselectorValues, sizeof(testselectorValues));
			memcpy(result.selectorPtr, selector.data(), selectorSize);

			check = static_cast<uint32_t*>(result.selectorPtr);


			for (size_t i = 0; i < result.lineCount; i++)
			{
				std::cout << "selector = " << check[i] << std::endl;


			}

			vkUnmapMemory(device, result.SelectorStorageMemory);


		}




		
		void VulkanBackend::doneedgesbuffer(Mesh& result) {

			std::vector<uint32_t> dones(result.lineCount, 0);


			VkDeviceSize donesSize = dones.size() * sizeof(uint32_t);

			createBuffer(
				donesSize,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, // or whatever you bind it as
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				result.DoneEdgeStorageBuffer,
				result.DoneEdgeStorageMemory
			);






			vkMapMemory(device, result.DoneEdgeStorageMemory, 0, donesSize, 0, (void**)&result.doneedgePtr);


			//test
			//uint32_t testselectorValues[] = {1, 0, 0, 0};
			//uint32_t testselectorValues[] = {0, 0, 0, 1, 1, 1};




			//auto* check = static_cast<uint32_t*>(result.selectorPtr);





			std::cout << "done size: " << donesSize << std::endl;
			std::cout << "done count: " << result.lineCount << std::endl;

			//memcpy(result.selectorPtr, testselectorValues, sizeof(testselectorValues));
			memcpy(result.doneedgePtr, dones.data(), donesSize);

			auto* check = static_cast<uint32_t*>(result.doneedgePtr);


			for (size_t i = 0; i < result.lineCount; i++)
			{
				std::cout << "done = " << check[i] << std::endl;


			}

			vkUnmapMemory(device, result.DoneEdgeStorageMemory);


		}




		void VulkanBackend::edgebuffer(Mesh& result) {

			std::vector<uint32_t> edges(result.lineCount, 1);

			std::cout << "cpu start" << std::endl;

			for (size_t i = 0; i < result.duplicate_edgesCPU.size(); i++)
			{
				edges[i] = result.duplicate_edgesCPU[i];

			}




			VkDeviceSize edgeSize = edges.size() * sizeof(uint32_t);


			std::cout << "pre create buffer" << std::endl;
			createBuffer(
				edgeSize,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, // or whatever you bind it as
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				result.DuplicateEdgeStorageBuffer,
				result.DuplicateEdgeStorageMemory
			);





			std::cout << "pre map mem " << std::endl;
			vkMapMemory(device, result.DuplicateEdgeStorageMemory, 0, edgeSize, 0, (void**)&result.dupedgePtr);




			auto* check = static_cast<uint32_t*>(result.dupedgePtr);





			std::cout << "edge size: " << edgeSize << std::endl;
			std::cout << "edge count: " << result.lineCount << std::endl;

			memcpy(result.dupedgePtr, edges.data(), edgeSize);

			check = static_cast<uint32_t*>(result.dupedgePtr);


			for (size_t i = 0; i < result.lineCount; i++)
			{
				std::cout << "edge = " << check[i] << std::endl;

			}

			vkUnmapMemory(device, result.DuplicateEdgeStorageMemory);


		}



		//vulkan stuffs


		void VulkanBackend::updateSelectorDescriptors(VkDeviceSize selectorSize) {

			if (enableValidationLayers) {
				std::cout << "- updateselectordescriptor " << std::endl;

				std::cout << "- " << std::endl;


			}


			for (size_t i = 0; i < descriptorSets.size(); i++) {
				VkDescriptorBufferInfo uboInfo{};
				uboInfo.buffer = uniformBuffers[i];
				uboInfo.offset = 0;
				uboInfo.range = sizeof(UniformBufferObject); // use your actual UBO type

				VkDescriptorBufferInfo selectorInfo{};
				selectorInfo.buffer = gMesh.SelectorStorageBuffer;
				selectorInfo.offset = 0;
				selectorInfo.range = selectorSize;

				VkDescriptorBufferInfo edgesInfo{};
				edgesInfo.buffer = gMesh.DuplicateEdgeStorageBuffer;
				edgesInfo.offset = 0;
				edgesInfo.range = selectorSize;

				VkDescriptorBufferInfo donesInfo{};
				donesInfo.buffer = gMesh.DoneEdgeStorageBuffer;
				donesInfo.offset = 0;
				donesInfo.range = selectorSize;


				std::array<VkWriteDescriptorSet, 4> writes{};

				writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writes[0].dstSet = descriptorSets[i];
				writes[0].dstBinding = 0;
				writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				writes[0].descriptorCount = 1;
				writes[0].pBufferInfo = &uboInfo;

				writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writes[1].dstSet = descriptorSets[i];
				writes[1].dstBinding = 1;
				writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				writes[1].descriptorCount = 1;
				writes[1].pBufferInfo = &selectorInfo;

				writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writes[2].dstSet = descriptorSets[i];
				writes[2].dstBinding = 2;
				writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				writes[2].descriptorCount = 1;
				writes[2].pBufferInfo = &edgesInfo;

				writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writes[3].dstSet = descriptorSets[i];
				writes[3].dstBinding = 3;
				writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				writes[3].descriptorCount = 1;
				writes[3].pBufferInfo = &donesInfo;



				vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
			}
		}




		void VulkanBackend::createDescriptorSets() {

			if (enableValidationLayers) {
				std::cout << "- createDescriptorSets " << std::endl;
			}

			VkDeviceSize selectorSize = (gMesh.lineCount) * sizeof(uint32_t);

			if (enableValidationLayers) {
				std::cout << "-- createDescriptorSets 1" << std::endl;
			}

			std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = descriptorPool;
			allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
			allocInfo.pSetLayouts = layouts.data();


			if (enableValidationLayers) {
				std::cout << "-- createDescriptorSets 2" << std::endl;
			}


			descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

			if (enableValidationLayers) {
				std::cout << "-- createDescriptorSets 3" << std::endl;
			}

			if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate descriptor sets!");
			}

			if (enableValidationLayers) {
				std::cout << "-- createDescriptorSets 4" << std::endl;
			}

			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
				VkDescriptorBufferInfo bufferInfo{};
				bufferInfo.buffer = uniformBuffers[i];
				bufferInfo.offset = 0;
				bufferInfo.range = sizeof(UniformBufferObject);

				VkDescriptorBufferInfo selectorInfo{};
				selectorInfo.buffer = gMesh.SelectorStorageBuffer;
				selectorInfo.offset = 0;
				selectorInfo.range = selectorSize;

				VkDescriptorBufferInfo edgesInfo{};
				edgesInfo.buffer = gMesh.DuplicateEdgeStorageBuffer;
				edgesInfo.offset = 0;
				edgesInfo.range = selectorSize;



				std::array<VkWriteDescriptorSet, 3> writes{};

				writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writes[0].dstSet = descriptorSets[i];
				writes[0].dstBinding = 0;
				writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				writes[0].descriptorCount = 1;
				writes[0].pBufferInfo = &bufferInfo;


				writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writes[1].dstSet = descriptorSets[i];
				writes[1].dstBinding = 1;
				writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				writes[1].descriptorCount = 1;
				writes[1].pBufferInfo = &selectorInfo;

				writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writes[2].dstSet = descriptorSets[i];
				writes[2].dstBinding = 2;
				writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				writes[2].descriptorCount = 1;
				writes[2].pBufferInfo = &edgesInfo;


				vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);

			}
		}







		void VulkanBackend::createDescriptorPool() {

			if (enableValidationLayers) {
				std::cout << "- createDescriptorPool " << std::endl;
			}

			std::array<VkDescriptorPoolSize, 4> poolSizes{};
			
			poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

			poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			poolSizes[1].descriptorCount = MAX_FRAMES_IN_FLIGHT;

			poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			poolSizes[2].descriptorCount = MAX_FRAMES_IN_FLIGHT;

			poolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			poolSizes[3].descriptorCount = MAX_FRAMES_IN_FLIGHT;


			VkDescriptorPoolCreateInfo poolInfo{};
			poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolInfo.poolSizeCount = poolSizes.size();
			poolInfo.pPoolSizes = poolSizes.data();

			poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);



			if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
				throw std::runtime_error("failed to create descriptor pool!");
			}
		}

		void VulkanBackend::updateUniformBuffer(uint32_t currentImage) {
			//UniformBufferObject ubo{};

			// --- MODEL TRANSFORM ---
			glm::mat4 model = glm::mat4(1.0f);

			// Apply user-controlled rotation
			model = glm::rotate(model, glm::radians((float)input.gPitch), glm::vec3(1, 0, 0));
			model = glm::rotate(model, glm::radians((float)input.gYaw), glm::vec3(0, 1, 0));

			// Apply user-controlled zoom (scale)
			model = glm::scale(model, glm::vec3(input.gScale));

			// Apply user-controlled panning
			model = glm::translate(model, glm::vec3(input.gPanX, input.gPanY, 0.0f));

			uboCPU.model = model;

			// --- VIEW MATRIX ---
			uboCPU.view = glm::lookAt(
				glm::vec3(0.0f, 0.0f, 3.0f),   // camera position
				glm::vec3(0.0f, 0.0f, 0.0f),   // look at origin
				glm::vec3(0.0f, 1.0f, 0.0f)    // up
			);

			// --- PROJECTION MATRIX ---
			uboCPU.proj = glm::perspective(
				glm::radians(45.0f),
				swapChainExtent.width / (float)swapChainExtent.height,
				0.1f,
				100.0f
			);
			uboCPU.proj[1][1] *= -1; // Vulkan Y flip

			// Copy to GPU
			memcpy(uniformBuffersMapped[currentImage], &uboCPU, sizeof(uboCPU));
		}



		void VulkanBackend::createUniformBuffers() {

			if (enableValidationLayers) {
				std::cout << "- createUniformBuffers " << std::endl;
			}

			VkDeviceSize bufferSize = sizeof(UniformBufferObject);

			uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
			uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
			uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
				createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);

				vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
			}
		}


		void VulkanBackend::createDescriptorSetLayout() {

			if (enableValidationLayers) {
				std::cout << "- createDescriptorSetLayout " << std::endl;
			}


			VkDescriptorSetLayoutBinding uboLayoutBinding{};
			uboLayoutBinding.binding = 0;
			uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uboLayoutBinding.descriptorCount = 1;

			uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

			uboLayoutBinding.pImmutableSamplers = nullptr; // Optional



			VkDescriptorSetLayoutBinding selectLayoutBinding{};
			selectLayoutBinding.binding = 1;
			selectLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			selectLayoutBinding.descriptorCount = 1;

			selectLayoutBinding.stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT;

			selectLayoutBinding.pImmutableSamplers = nullptr; // Optional


			VkDescriptorSetLayoutBinding edgeLayoutBinding{};
			edgeLayoutBinding.binding = 2;
			edgeLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			edgeLayoutBinding.descriptorCount = 1;

			edgeLayoutBinding.stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT;

			edgeLayoutBinding.pImmutableSamplers = nullptr; // Optional


			VkDescriptorSetLayoutBinding doneLayoutBinding{};
			doneLayoutBinding.binding = 3;
			doneLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			doneLayoutBinding.descriptorCount = 1;

			doneLayoutBinding.stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT;

			doneLayoutBinding.pImmutableSamplers = nullptr; // Optional



			std::array<VkDescriptorSetLayoutBinding, 4> bindings = { uboLayoutBinding, selectLayoutBinding, edgeLayoutBinding, doneLayoutBinding };


			VkDescriptorSetLayoutCreateInfo layoutInfo{};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = bindings.size();
			layoutInfo.pBindings = bindings.data();

			if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
				throw std::runtime_error("failed to create descriptor set layout!");
			}
		}



		void VulkanBackend::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer & buffer, VkDeviceMemory & bufferMemory) {

			if (enableValidationLayers) {
				std::cout << "- createBuffer " << std::endl;
			}

			VkBufferCreateInfo bufferInfo{};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = size;
			bufferInfo.usage = usage;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;



			if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
				throw std::runtime_error("failed to create buffer!");
			}

			VkMemoryRequirements memRequirements;
			vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

			VkMemoryAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

			if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate buffer memory!");
			}

			vkBindBufferMemory(device, buffer, bufferMemory, 0);
		}


		void VulkanBackend::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {

			if (enableValidationLayers) {
				std::cout << "- copyBuffer " << std::endl;
			}


			VkCommandBufferAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandPool = commandPool;
			allocInfo.commandBufferCount = 1;

			VkCommandBuffer commandBuffer;
			vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			vkBeginCommandBuffer(commandBuffer, &beginInfo);

			VkBufferCopy copyRegion{};
			copyRegion.srcOffset = 0; // Optional
			copyRegion.dstOffset = 0; // Optional
			copyRegion.size = size;
			vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

			vkEndCommandBuffer(commandBuffer);

			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBuffer;

			vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
			vkQueueWaitIdle(graphicsQueue);

			vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);


		}



		uint32_t VulkanBackend::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {


			if (enableValidationLayers) {
				std::cout << "- findMemoryType " << std::endl;
			}

			VkPhysicalDeviceMemoryProperties memProperties;
			vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

			for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
				if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
					return i;
				}
			}

			throw std::runtime_error("failed to find suitable memory type!");



		}



		void VulkanBackend::framebufferResizeCallback(GLFWwindow * window, int width, int height) {
			auto app = reinterpret_cast<VulkanBackend*>(glfwGetWindowUserPointer(window));
			app->framebufferResized = true;
		}



		void VulkanBackend::cleanupSwapChain() {

			for (auto framebuffer : swapChainFramebuffers) {
				vkDestroyFramebuffer(device, framebuffer, nullptr);
			}

			for (auto imageView : swapChainImageViews) {
				vkDestroyImageView(device, imageView, nullptr);
			}

			vkDestroySwapchainKHR(device, swapChain, nullptr);

		}



		void VulkanBackend::recreateSwapChain() {

			int width = 0, height = 0;
			glfwGetFramebufferSize(window, &width, &height);
			while (width == 0 || height == 0) {
				glfwGetFramebufferSize(window, &width, &height);
				glfwWaitEvents();
			}

			vkDeviceWaitIdle(device);

			cleanupSwapChain();

			createSwapChain();
			createImageViews();
			createFramebuffers();

			imagesInFlight.assign(swapChainImages.size(), VK_NULL_HANDLE);
		}



		void VulkanBackend::createSyncObjects() {

			if (enableValidationLayers) {
				std::cout << "- createSyncObjects " << std::endl;
			}


			imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
			renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
			inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);



			VkSemaphoreCreateInfo semaphoreInfo = vkinit::semaphore_create_info();




			VkFenceCreateInfo fenceInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);


			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
				if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
					vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
					vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

					throw std::runtime_error("failed to create synchronization objects for a frame!");
				}
			}



		}


		void VulkanBackend::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {

			if (enableValidationLayers) {
				//std::cout << "- recordCommandBuffer " << std::endl;
			}


			VkCommandBufferBeginInfo beginInfo = vkinit::command_buffer_begin_info();




			if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
				throw std::runtime_error("failed to begin recording command buffer!");
			}




			// Build ImGUI
			buildImGui();

			if (enableValidationLayers) {
				//std::cout << "- after buildImGui" << std::endl;
			}

			VkClearValue clearColor = { {{0.1f, 0.2f, 0.3f, 1.0f}} };

			VkRenderPassBeginInfo renderPassInfo = vkinit::renderpass_begin_info(renderPass, swapChainExtent, swapChainFramebuffers[imageIndex], &clearColor);



			vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);







			//VkBuffer vertexBuffers[] = { gMesh.unjoinedvertexBuffer};
			//VkDeviceSize offsets[] = { 0 };


			//moved these two over too if model loaded since they null otherwise
			//vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
			//vkCmdBindIndexBuffer(commandBuffer, gMesh.indexBuffer, 0, VK_INDEX_TYPE_UINT16);



			VkViewport viewport{};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = static_cast<float>(swapChainExtent.width);
			viewport.height = static_cast<float>(swapChainExtent.height);
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);


			VkRect2D scissor{};
			scissor.offset = { 0, 0 };
			scissor.extent = swapChainExtent;
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);



			//thingy


			// Bind the mesh loaded from Assimp
			if (modelLoaded) {
				VkDeviceSize offsets[] = { 0 };


				//choose which index buffer to use based on selector array
				vkCmdBindVertexBuffers(commandBuffer, 0, 1, &gMesh.vertexBuffer, offsets);
				//vkCmdBindIndexBuffer(commandBuffer, gMesh.fillindexBuffer, 0, VK_INDEX_TYPE_UINT32);

				//vkCmdBindVertexBuffers(commandBuffer, 0, 1, &gMesh.unjoinedvertexBuffer, offsets);
				//vkCmdBindIndexBuffer(commandBuffer, gMesh.unjoinedindexBuffer, 0, VK_INDEX_TYPE_UINT32);


				vkCmdBindDescriptorSets(
					commandBuffer,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipelineLayout,
					0,
					1,
					&descriptorSets[currentFrame],
					0,
					nullptr
				);




				//FILLED PIPELINE----------------------
				//std::cout << "- prebind1" << std::endl;
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_Filled);



				vkCmdBindIndexBuffer(commandBuffer, gMesh.fillindexBuffer, 0, VK_INDEX_TYPE_UINT32);


				//std::cout << "- predraw1" << std::endl;
				vkCmdDrawIndexed(commandBuffer, gMesh.fillindexCount, 1, 0, 0, 0);

				//std::cout << "- prebind2" << std::endl;





				//LINED PIPELINE----------------------

				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_Line);


				vkCmdBindIndexBuffer(commandBuffer, gMesh.lineindexBuffer, 0, VK_INDEX_TYPE_UINT32);


				//std::cout << "- predraw2" << std::endl;

				vkCmdDrawIndexed(commandBuffer, gMesh.lineindexCount, 1, 0, 0, 0);



			}



			// render ImGUI
			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);





			vkCmdEndRenderPass(commandBuffer);


			if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
				throw std::runtime_error("failed to record command buffer!");
			}

		}


		void VulkanBackend::createCommandBuffers() {

			if (enableValidationLayers) {
				std::cout << "- createCommandBuffer " << std::endl;
			}

			commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);


			VkCommandBufferAllocateInfo allocInfo = vkinit::command_buffer_allocate_info(commandPool, (uint32_t)commandBuffers.size());




			if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate command buffers!");
			}
		}



		void VulkanBackend::createCommandPool() {

			if (enableValidationLayers) {
				std::cout << "- createCommandPool " << std::endl;
			}




			QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);


			VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(queueFamilyIndices.graphicsFamily.value(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);



			if (vkCreateCommandPool(device, &commandPoolInfo, nullptr, &commandPool) != VK_SUCCESS) {
				throw std::runtime_error("failed to create command pool!");
			}


			if (enableValidationLayers) {

				VkDebugUtilsObjectNameInfoEXT nameInfo = vkinit::debug_name_create_info(VK_OBJECT_TYPE_COMMAND_POOL, (uint64_t)commandPool, "commandPool");

				PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");

				if (vkSetDebugUtilsObjectNameEXT) {
					vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
				}


			}


		}


		void VulkanBackend::createFramebuffers() {

			if (enableValidationLayers) {
				std::cout << "- createFramebuffers " << std::endl;
			}



			swapChainFramebuffers.resize(swapChainImageViews.size());

			for (size_t i = 0; i < swapChainImageViews.size(); i++) {
				VkImageView attachments[] = {
					swapChainImageViews[i]
				};



				VkFramebufferCreateInfo framebufferInfo = vkinit::framebuffer_create_info(renderPass, swapChainExtent, attachments);


				if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
					throw std::runtime_error("failed to create framebuffer!");
				}




				if (enableValidationLayers) {

					VkDebugUtilsObjectNameInfoEXT nameInfo = vkinit::debug_name_create_info(VK_OBJECT_TYPE_FRAMEBUFFER, (uint64_t)swapChainFramebuffers[i], "framebuffer");

					PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");

					if (vkSetDebugUtilsObjectNameEXT) {
						vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
					}
				}
			}



		}


		void VulkanBackend::createRenderPass() {

			if (enableValidationLayers) {
				std::cout << "- createRenderPass " << std::endl;
			}


			VkAttachmentDescription colorAttachment{};
			colorAttachment.format = swapChainImageFormat;
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;


			VkAttachmentReference colorAttachmentRef{};
			colorAttachmentRef.attachment = 0;
			colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


			VkSubpassDescription subpass{};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorAttachmentRef;


			VkRenderPassCreateInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.attachmentCount = 1;
			renderPassInfo.pAttachments = &colorAttachment;
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;


			VkSubpassDependency dependency{};
			dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass = 0;

			dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.srcAccessMask = 0;

			dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			renderPassInfo.dependencyCount = 1;
			renderPassInfo.pDependencies = &dependency;









			if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
				throw std::runtime_error("failed to create render pass!");
			}


		}


		std::vector<char> VulkanBackend::readFile(const std::string & filename) {



			std::ifstream file(filename, std::ios::ate | std::ios::binary);



			if (!file.is_open()) {
				throw std::runtime_error("failed to open file! : " + filename);
			}
			//else {
			//    throw std::runtime_error("suceeded to open file! : " + filename);
			//}

			size_t fileSize = (size_t)file.tellg();
			std::vector<char> buffer(fileSize);

			file.seekg(0);
			file.read(buffer.data(), fileSize);


			file.close();

			return buffer;



		}


		void VulkanBackend::createGraphicsPipeline() {

			if (enableValidationLayers) {
				std::cout << "- createGraphicsPipeline " << std::endl;
			}


			auto line_vertShaderCode = readFile(line_vertexShaderPath);
			auto line_geoShaderCode = readFile(line_geometryShaderPath);
			auto fill_vertShaderCode = readFile(fill_vertexShaderPath);
			auto fragShaderCode = readFile(fragmentShaderPath);

			//auto fragShaderCode_fill = readFile(filledfragmentShaderPath);
			//auto fragShaderCode_line = readFile(linefragmentShaderPath);


			VkShaderModule line_vertShaderModule = createShaderModule(line_vertShaderCode);
			VkShaderModule line_geoShaderModule = createShaderModule(line_geoShaderCode);

			VkShaderModule fill_vertShaderModule = createShaderModule(fill_vertShaderCode);
			VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);



			//VkShaderModule fragShaderModule_fill = createShaderModule(fragShaderCode_fill);
			//VkShaderModule fragShaderModule_line = createShaderModule(fragShaderCode_line);


			if (enableValidationLayers) {
				std::cout << "- createShaderModules fin " << std::endl;
			}

			VkPipelineShaderStageCreateInfo line_vertShaderStageInfo{};
			line_vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			line_vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			line_vertShaderStageInfo.module = line_vertShaderModule;
			line_vertShaderStageInfo.pName = "main";


			VkPipelineShaderStageCreateInfo line_geomShaderStageInfo{};
			line_geomShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			line_geomShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
			line_geomShaderStageInfo.module = line_geoShaderModule;
			line_geomShaderStageInfo.pName = "main";




			VkPipelineShaderStageCreateInfo fill_vertShaderStageInfo{};
			fill_vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fill_vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			fill_vertShaderStageInfo.module = fill_vertShaderModule;
			fill_vertShaderStageInfo.pName = "main";



			VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
			fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragShaderStageInfo.module = fragShaderModule;
			fragShaderStageInfo.pName = "main";



			VkPipelineShaderStageCreateInfo shaderStages_fill[] = { fill_vertShaderStageInfo, fragShaderStageInfo };

			VkPipelineShaderStageCreateInfo shaderStages_line[] = { line_vertShaderStageInfo, line_geomShaderStageInfo, fragShaderStageInfo };


			if (enableValidationLayers) {
				std::cout << "- post shaderstagesfill " << std::endl;
			}


			std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
			};

			if (enableValidationLayers) {
				std::cout << "- pre dynamicState " << std::endl;
			}

			VkPipelineDynamicStateCreateInfo dynamicState{};
			dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
			dynamicState.pDynamicStates = dynamicStates.data();

			if (enableValidationLayers) {
				std::cout << "- pre bindingDescription " << std::endl;
			}

			auto bindingDescription = MeshVertex::getBindingDescription();

			if (enableValidationLayers) {
				std::cout << "- post bindingDescription " << std::endl;
			}

			auto attributeDescriptions = MeshVertex::getAttributeDescriptions();


			if (enableValidationLayers) {
				std::cout << "- pre vertexInputInfo " << std::endl;
			}

			VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputInfo.vertexBindingDescriptionCount = 1;
			vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
			vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
			vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();


			if (enableValidationLayers) {
				std::cout << "- post vertexInputInfo " << std::endl;
			}

			VkPipelineInputAssemblyStateCreateInfo inputAssembly_filled{};
			inputAssembly_filled.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssembly_filled.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; //VK_PRIMITIVE_TOPOLOGY_POINT_LIST, VK_PRIMITIVE_TOPOLOGY_LINE_LIST, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
			inputAssembly_filled.primitiveRestartEnable = VK_FALSE;

			VkPipelineInputAssemblyStateCreateInfo inputAssembly_lined{};
			inputAssembly_lined.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssembly_lined.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;// , VK_PRIMITIVE_TOPOLOGY_LINE_STRIP, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
			inputAssembly_lined.primitiveRestartEnable = VK_FALSE;

			VkViewport viewport{};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = (float)swapChainExtent.width;
			viewport.height = (float)swapChainExtent.height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			VkRect2D scissor{};
			scissor.offset = { 0, 0 };
			scissor.extent = swapChainExtent;



			if (enableValidationLayers) {
				std::cout << "- pre viewport state " << std::endl;
			}



			VkPipelineViewportStateCreateInfo viewportState{};
			viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportState.viewportCount = 1;
			viewportState.pViewports = &viewport;
			viewportState.scissorCount = 1;
			viewportState.pScissors = &scissor;



			//multisampling currently disabled
			VkPipelineMultisampleStateCreateInfo multisampling{};
			multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampling.sampleShadingEnable = VK_FALSE;
			multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			multisampling.minSampleShading = 1.0f; // Optional
			multisampling.pSampleMask = nullptr; // Optional
			multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
			multisampling.alphaToOneEnable = VK_FALSE; // Optional


			VkPipelineColorBlendAttachmentState colorBlendAttachment{};
			colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			colorBlendAttachment.blendEnable = VK_FALSE;
			colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
			colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
			colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
			colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
			colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
			colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

			colorBlendAttachment.blendEnable = VK_TRUE;
			colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
			colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

			VkPipelineColorBlendStateCreateInfo colorBlending{};
			colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlending.logicOpEnable = VK_FALSE;
			colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
			colorBlending.attachmentCount = 1;
			colorBlending.pAttachments = &colorBlendAttachment;
			colorBlending.blendConstants[0] = 0.0f; // Optional
			colorBlending.blendConstants[1] = 0.0f; // Optional
			colorBlending.blendConstants[2] = 0.0f; // Optional
			colorBlending.blendConstants[3] = 0.0f; // Optional

			//If you want to use the second method of blending (bitwise combination), then you should set logicOpEnable to VK_TRUE

			VkPushConstantRange pushConstantRange{};
			pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // or VERTEX_BIT | FRAGMENT_BIT
			pushConstantRange.offset = 0;
			pushConstantRange.size = sizeof(glm::vec4);



			VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.setLayoutCount = 1;
			pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
			pipelineLayoutInfo.pushConstantRangeCount = 1;
			pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;


			if (enableValidationLayers) {
				std::cout << "- pre create pipeline layout " << std::endl;
			}


			if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
				throw std::runtime_error("failed to create pipeline layout!");
			}

			if (enableValidationLayers) {
				std::cout << "- post create pipeline layout thingy" << std::endl;
			}



			//rasterizer

			VkPipelineDepthStencilStateCreateInfo depthinfo_filled = vkinit::depth_stencil_create_info(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);

			VkPipelineDepthStencilStateCreateInfo depthinfo_line = vkinit::depth_stencil_create_info(VK_TRUE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);



			//filled rasterizer and graphics pipeline
			VkPipelineRasterizationStateCreateInfo rasterizer_filled = vkinit::rasterization_state_filled_create_info();





			VkGraphicsPipelineCreateInfo pipelineInfo_filled{};
			pipelineInfo_filled.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineInfo_filled.stageCount = 2;
			pipelineInfo_filled.pStages = shaderStages_fill;
			pipelineInfo_filled.pVertexInputState = &vertexInputInfo;
			pipelineInfo_filled.pInputAssemblyState = &inputAssembly_filled;
			pipelineInfo_filled.pViewportState = &viewportState;
			pipelineInfo_filled.pRasterizationState = &rasterizer_filled;
			pipelineInfo_filled.pMultisampleState = &multisampling;
			pipelineInfo_filled.pDepthStencilState = &depthinfo_filled; // Optional
			pipelineInfo_filled.pColorBlendState = &colorBlending;
			pipelineInfo_filled.pDynamicState = &dynamicState;
			pipelineInfo_filled.layout = pipelineLayout;
			pipelineInfo_filled.renderPass = renderPass;
			pipelineInfo_filled.subpass = 0;
			pipelineInfo_filled.basePipelineHandle = VK_NULL_HANDLE; // Optional
			pipelineInfo_filled.basePipelineIndex = -1; // Optional



			if (enableValidationLayers) {
				std::cout << "- other pre create pipeline layout thingy" << std::endl;
			}

			if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo_filled, nullptr, &graphicsPipeline_Filled) != VK_SUCCESS) {
				throw std::runtime_error("failed to create graphics pipeline!");
			}


			if (enableValidationLayers) {
				std::cout << "- other post create pipeline layout thingy" << std::endl;
			}




			//line rasterizer and graphics pipeline
			VkPipelineRasterizationStateCreateInfo rasterizer_line = vkinit::rasterization_state_lined_create_info(1);

			VkGraphicsPipelineCreateInfo pipelineInfo_line{};
			pipelineInfo_line.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineInfo_line.stageCount = 3;
			pipelineInfo_line.pStages = shaderStages_line;
			pipelineInfo_line.pVertexInputState = &vertexInputInfo;
			pipelineInfo_line.pInputAssemblyState = &inputAssembly_lined;
			pipelineInfo_line.pViewportState = &viewportState;
			pipelineInfo_line.pRasterizationState = &rasterizer_line;
			pipelineInfo_line.pMultisampleState = &multisampling;
			pipelineInfo_line.pDepthStencilState = &depthinfo_line; // Optional
			pipelineInfo_line.pColorBlendState = &colorBlending;
			pipelineInfo_line.pDynamicState = &dynamicState;
			pipelineInfo_line.layout = pipelineLayout;
			pipelineInfo_line.renderPass = renderPass;
			pipelineInfo_line.subpass = 0;
			pipelineInfo_line.basePipelineHandle = VK_NULL_HANDLE; // Optional
			pipelineInfo_line.basePipelineIndex = -1; // Optional





			if (enableValidationLayers) {
				std::cout << "- other other pre create pipeline layout thingy" << std::endl;
			}


			if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo_line, nullptr, &graphicsPipeline_Line) != VK_SUCCESS) {
				throw std::runtime_error("failed to create graphics pipeline!");
			}


			if (enableValidationLayers) {
				std::cout << "- other other post create pipeline layout thingy" << std::endl;
			}

			//cleanup
			vkDestroyShaderModule(device, fragShaderModule, nullptr);
			vkDestroyShaderModule(device, fill_vertShaderModule, nullptr);
			vkDestroyShaderModule(device, line_vertShaderModule, nullptr);
			vkDestroyShaderModule(device, line_geoShaderModule, nullptr);


			if (enableValidationLayers) {
				std::cout << "- createGraphicsPipeline fin " << std::endl;
			}

		}














		VkShaderModule VulkanBackend::createShaderModule(const std::vector<char>&code) {

			if (enableValidationLayers) {
				std::cout << "- createShaderModule " << std::endl;
			}



			VkShaderModuleCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = code.size();
			createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());


			VkShaderModule shaderModule;
			if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
				throw std::runtime_error("failed to create shader module!");
			}


			return shaderModule;
		}


		void VulkanBackend::createImageViews() {

			if (enableValidationLayers) {
				std::cout << "- createImageViews " << std::endl;
			}



			swapChainImageViews.resize(swapChainImages.size());


			for (size_t i = 0; i < swapChainImages.size(); i++) {

				VkImageViewCreateInfo createInfo{};
				createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				createInfo.image = swapChainImages[i];
				createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				createInfo.format = swapChainImageFormat;

				createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
				createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
				createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
				createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

				createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				createInfo.subresourceRange.baseMipLevel = 0;
				createInfo.subresourceRange.levelCount = 1;
				createInfo.subresourceRange.baseArrayLayer = 0;
				createInfo.subresourceRange.layerCount = 1;


				if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
					throw std::runtime_error("failed to create image views!");
				}


			}

		}



		void VulkanBackend::createSurface() {
			if (enableValidationLayers) {
				std::cout << "- createSurface " << std::endl;
			}

			if (instance == VK_NULL_HANDLE) {
				throw std::runtime_error("Vulkan instance is not created before creating surface!");
			}
			if (window == VK_NULL_HANDLE)
			{
				throw std::runtime_error("Vulkan window is not created before creating surface!");
			}
			//if (surface != VK_NULL_HANDLE) {
			//    throw std::runtime_error("Vulkan surface is already created!");
			//}


			VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
			if (result != VK_SUCCESS) {
				throw std::runtime_error("failed to create window surface! Error code: " + std::to_string(result));
			}


		}



		void VulkanBackend::createInstance() {

			if (enableValidationLayers) {
				std::cout << "- createInstance " << std::endl;



				
				if (!checkValidationLayerSupport()) {
					throw std::runtime_error("validation layers requested, but not available!");
				}
				


			}

			VkApplicationInfo appInfo{};
			appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			appInfo.pApplicationName = "Hello Triangle";
			appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
			appInfo.pEngineName = "No Engine";
			appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
			appInfo.apiVersion = VK_API_VERSION_1_0;




			VkInstanceCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			createInfo.pApplicationInfo = &appInfo;

			uint32_t glfwExtensionCount = 0;

			const char** glfwExtensions;



			VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
			if (enableValidationLayers) {
				createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
				createInfo.ppEnabledLayerNames = validationLayers.data();

				populateDebugMessengerCreateInfo(debugCreateInfo);

				createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
			}
			else {
				createInfo.enabledLayerCount = 0;

				createInfo.pNext = nullptr;
			}


			//Checking for extension support
			uint32_t extensionCount = 0;
			vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

			std::vector<VkExtensionProperties> extensions(extensionCount);

			vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());


			auto RequiredExtensions = getRequiredExtensions();
			createInfo.enabledExtensionCount = static_cast<uint32_t>(RequiredExtensions.size());
			createInfo.ppEnabledExtensionNames = RequiredExtensions.data();

			if (enableValidationLayers) {
				std::cout << "- available extensions:\n";

				for (const auto& extension : extensions) {
					std::cout << '\t' << extension.extensionName << '\n';
				}

				std::cout << "- required extensions:\n";

				for (const auto& rextension : RequiredExtensions) {
					std::cout << '\t' << rextension << '\n';
				}

			}



			if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
				throw std::runtime_error("failed to create instance!");
			}

		}


		bool VulkanBackend::checkValidationLayerSupport() {

			if (enableValidationLayers) {
				std::cout << "- checkValidationLayerSupport " << std::endl;
			}


			uint32_t layerCount;
			vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

			std::vector<VkLayerProperties> availableLayers(layerCount);
			vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

			for (const char* layerName : validationLayers) {
				bool layerFound = false;

				for (const auto& layerProperties : availableLayers) {
					std::cout << layerProperties.layerName << std::endl;
					if (strcmp(layerName, layerProperties.layerName) == 0) {
						layerFound = true;
						break;
					}
				}

				if (!layerFound) {
					return false;
				}
			}

			return true;
		}

		std::vector<const char*> VulkanBackend::getRequiredExtensions() {

			if (enableValidationLayers) {
				std::cout << "- getRequiredExtensions " << std::endl;
			}


			uint32_t glfwExtensionCount = 0;
			const char** glfwExtensions;
			glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

			std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

			if (enableValidationLayers) {
				extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			}

			return extensions;
		}


		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData,
			void* pUserData) {




			std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

			return VK_FALSE;
		}

		void VulkanBackend::setupDebugMessenger() {

			if (enableValidationLayers) {
				std::cout << "- setupDebugMessenger " << std::endl;
			}


			if (!enableValidationLayers) return;





			VkDebugUtilsMessengerCreateInfoEXT createInfo;

			populateDebugMessengerCreateInfo(createInfo);


			if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
				throw std::runtime_error("failed to set up debug messenger!");
			}


		}


		VkResult VulkanBackend::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkDebugUtilsMessengerEXT * pDebugMessenger) {
			auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
			if (func != nullptr) {
				return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
			}
			else {
				return VK_ERROR_EXTENSION_NOT_PRESENT;
			}
		}

		void VulkanBackend::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks * pAllocator) {
			auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
			if (func != nullptr) {
				func(instance, debugMessenger, pAllocator);
			}
		}


		void VulkanBackend::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT & createInfo) {

			if (enableValidationLayers) {
				std::cout << "- populateDebugMessengerCreateInfo " << std::endl;
			}


			createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			createInfo.pfnUserCallback = debugCallback;
		}

		void VulkanBackend::pickPhysicalDevice() {
			if (enableValidationLayers) {
				std::cout << "- pick phyicsal device " << std::endl;
			}

			uint32_t deviceCount = 0;
			vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

			if (deviceCount == 0) {
				throw std::runtime_error("failed to find GPUs with Vulkan support!");
			}

			std::vector<VkPhysicalDevice> devices(deviceCount);
			vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

			std::multimap<int, VkPhysicalDevice> candidates;





			for (const auto& device : devices) {

				if (enableValidationLayers) {
					std::cout << "- phyicsal device scores" << std::endl;
				}
				if (isDeviceSuitable(device)) {

					int score = rateDeviceSuitability(device);
					candidates.insert(std::make_pair(score, device));

				}

			}

			// Check if the best candidate is suitable at all
			if (candidates.rbegin()->first > 0) {
				physicalDevice = candidates.rbegin()->second;
			}
			else {
				throw std::runtime_error("failed to find a suitable GPU!");
			}
		}


		bool VulkanBackend::isDeviceSuitable(VkPhysicalDevice device) {

			if (enableValidationLayers) {
				std::cout << "- isDeviceSuitable " << std::endl;
			}


			QueueFamilyIndices indices = findQueueFamilies(device);

			bool extensionsSupported = checkDeviceExtensionSupport(device);


			bool swapChainAdequate = false;
			if (extensionsSupported) {
				SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
				swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
			}


			return indices.isComplete() && extensionsSupported && swapChainAdequate;
		}

		bool VulkanBackend::checkDeviceExtensionSupport(VkPhysicalDevice device) {

			if (enableValidationLayers) {
				std::cout << "- checkDeviceExtensionSupport " << std::endl;
			}



			uint32_t extensionCount;
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

			std::vector<VkExtensionProperties> availableExtensions(extensionCount);
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

			std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

			for (const auto& extension : availableExtensions) {
				requiredExtensions.erase(extension.extensionName);
			}





			return requiredExtensions.empty();
		}


		int VulkanBackend::rateDeviceSuitability(VkPhysicalDevice device) {

			if (enableValidationLayers) {
				std::cout << "- rateDeviceSuitability " << std::endl;
			}



			VkPhysicalDeviceProperties deviceProperties;
			VkPhysicalDeviceFeatures deviceFeatures;
			vkGetPhysicalDeviceProperties(device, &deviceProperties);
			vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

			int score = 0;

			// Discrete GPUs have a significant performance advantage
			if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
				score += 1000;
			}

			// Maximum possible size of textures affects graphics quality
			score += deviceProperties.limits.maxImageDimension2D;

			// Application can't function without geometry shaders
			if (!deviceFeatures.geometryShader) {
				return 0;
			}

			if (enableValidationLayers) {
				std::cout << "\t- device name: " << deviceProperties.deviceName << std::endl;
				std::cout << "\t- device score: " << score << std::endl;
			}



			return score;
		}

		VulkanBackend::QueueFamilyIndices VulkanBackend::findQueueFamilies(VkPhysicalDevice device) {

			if (enableValidationLayers) {
				std::cout << "- findQueueFamilies " << std::endl;
			}


			QueueFamilyIndices indices;
			// Logic to find queue family indices to populate struct with

			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());




			int i = 0;
			for (const auto& queueFamily : queueFamilies) {

				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

				if (presentSupport) {
					indices.presentFamily = i;
				}

				if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					indices.graphicsFamily = i;
				}

				if (indices.isComplete()) {
					break;
				}

				i++;
			}


			return indices;
		}

		void VulkanBackend::createLogicalDevice() {

			if (enableValidationLayers) {
				std::cout << "- createLogicalDevice " << std::endl;
			}

			QueueFamilyIndices indices = findQueueFamilies(physicalDevice);


			std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
			std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

			float queuePriority = 1.0f;
			for (uint32_t queueFamily : uniqueQueueFamilies) {
				VkDeviceQueueCreateInfo queueCreateInfo{};
				queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueCreateInfo.queueFamilyIndex = queueFamily;
				queueCreateInfo.queueCount = 1;
				queueCreateInfo.pQueuePriorities = &queuePriority;
				queueCreateInfos.push_back(queueCreateInfo);
			}



			VkPhysicalDevice8BitStorageFeatures storage8bit{};
			storage8bit.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES;
			storage8bit.storageBuffer8BitAccess = VK_TRUE;

			



			VkPhysicalDeviceFeatures deviceFeatures{};
			deviceFeatures.fillModeNonSolid = VK_TRUE;
			deviceFeatures.geometryShader = VK_TRUE;



			VkDeviceCreateInfo createInfo = vkinit::device_create_info(queueCreateInfos, deviceFeatures, deviceExtensions, validationLayers, enableValidationLayers);
			createInfo.pNext = &storage8bit;


			if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
				throw std::runtime_error("failed to create logical device!");
			}



			if (enableValidationLayers) {

				VkDebugUtilsObjectNameInfoEXT nameInfo = vkinit::debug_name_create_info(VK_OBJECT_TYPE_DEVICE, (uint64_t)device, "LogicalDevice1");

				PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");

				if (vkSetDebugUtilsObjectNameEXT) {
					vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
				}


			}


			vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
			vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);


		}




		void VulkanBackend::createSwapChain() {

			if (enableValidationLayers) {
				std::cout << "- createSwapChain " << std::endl;
			}


			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

			VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
			VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
			VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

			uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

			if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
				imageCount = swapChainSupport.capabilities.maxImageCount;
			}

			VkSwapchainCreateInfoKHR createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			createInfo.surface = surface;

			createInfo.minImageCount = imageCount;
			createInfo.imageFormat = surfaceFormat.format;
			createInfo.imageColorSpace = surfaceFormat.colorSpace;
			createInfo.imageExtent = extent;
			createInfo.imageArrayLayers = 1;
			createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;


			queueFamilyIndicesprivate = findQueueFamilies(physicalDevice);
			uint32_t queueFamilyIndices[] = { queueFamilyIndicesprivate.graphicsFamily.value(), queueFamilyIndicesprivate.presentFamily.value() };

			if (queueFamilyIndicesprivate.graphicsFamily != queueFamilyIndicesprivate.presentFamily) {
				createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
				createInfo.queueFamilyIndexCount = 2;
				createInfo.pQueueFamilyIndices = queueFamilyIndices;
			}
			else {
				createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
				createInfo.queueFamilyIndexCount = 0; // Optional
				createInfo.pQueueFamilyIndices = nullptr; // Optional
			}

			createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

			createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

			createInfo.presentMode = presentMode;
			createInfo.clipped = VK_TRUE;

			createInfo.oldSwapchain = VK_NULL_HANDLE;

			if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
				throw std::runtime_error("failed to create swap chain!");
			}

			vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
			swapChainImages.resize(imageCount);
			vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

			swapChainImageFormat = surfaceFormat.format;
			swapChainExtent = extent;




			imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);
		}

		VulkanBackend::SwapChainSupportDetails VulkanBackend::querySwapChainSupport(VkPhysicalDevice device) {

			if (enableValidationLayers) {
				std::cout << "- querySwapChainSupport " << std::endl;
			}



			SwapChainSupportDetails details;

			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);


			uint32_t formatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

			if (formatCount != 0) {
				details.formats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
			}

			uint32_t presentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

			if (presentModeCount != 0) {
				details.presentModes.resize(presentModeCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
			}


			return details;
		}

		VkSurfaceFormatKHR VulkanBackend::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>&availableFormats) {

			if (enableValidationLayers) {
				std::cout << "- chooseSwapSurfaceFormat " << std::endl;
			}



			for (const auto& availableFormat : availableFormats) {
				if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
					return availableFormat;
				}
			}

			return availableFormats[0];
		}


		//VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR, VK_PRESENT_MODE_FIFO_RELAXED_KHR, VK_PRESENT_MODE_MAILBOX_KHR
		//mailbox is the best option if non mobile (it takes more energy)
		VkPresentModeKHR VulkanBackend::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>&availablePresentModes) {

			if (enableValidationLayers) {
				std::cout << "- chooseSwapPresentMode " << std::endl;
			}


			for (const auto& availablePresentMode : availablePresentModes) {
				if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
					return availablePresentMode;
				}
			}

			return VK_PRESENT_MODE_FIFO_KHR;
		}


		VkExtent2D VulkanBackend::chooseSwapExtent(const VkSurfaceCapabilitiesKHR & capabilities) {

			if (enableValidationLayers) {
				std::cout << "- chooseSwapExtent " << std::endl;
			}




			if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
				return capabilities.currentExtent;
			}
			else {
				int width, height;
				glfwGetFramebufferSize(window, &width, &height);

				VkExtent2D actualExtent = {
					static_cast<uint32_t>(width),
					static_cast<uint32_t>(height)
				};

				actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
				actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

				return actualExtent;
			}


		}
