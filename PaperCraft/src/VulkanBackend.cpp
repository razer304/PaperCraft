#include "VulkanBackend.h"
#include "vk_initializers.h"


void VulkanBackend::runVulkanBackend() {
    if (enableValidationLayers) {
        std::cout << "- runVulkanBackend " << std::endl;
    }
	initWindow();
    initVulkan();
    mainLoop();
    cleanupVulkan();
}
//base Vulkan functions


void VulkanBackend::initWindow() {
    if (enableValidationLayers) {
        std::cout << "Init Window " << std::endl;
    }
	glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);






    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

    glfwSetWindowUserPointer(window, this);


    glfwSetScrollCallback(window, scroll_callback); 
    glfwSetMouseButtonCallback(window, mouse_button_callback); 
    glfwSetCursorPosCallback(window, cursor_position_callback); 


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

    createDescriptorSets();

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


    vkDestroyBuffer(device, gMesh.indexBuffer, nullptr);
    vkFreeMemory(device, gMesh.indexMemory, nullptr);

    vkDestroyBuffer(device, gMesh.vertexBuffer, nullptr);
    vkFreeMemory(device, gMesh.vertexMemory, nullptr);

    

    

    vkDestroyPipeline(device, graphicsPipeline, nullptr);
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
        std::cout << "- drawFrame " << currentFrame << std::endl;
    }

    


    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        recreateSwapChain();
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

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




void VulkanBackend::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    auto* backend = reinterpret_cast<VulkanBackend*>(glfwGetWindowUserPointer(window));
    backend->onScroll(xoffset, yoffset);
}

void VulkanBackend::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    auto* backend = reinterpret_cast<VulkanBackend*>(glfwGetWindowUserPointer(window));
    backend->onMouseButton(button, action, mods);
}

void VulkanBackend::cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    auto* backend = reinterpret_cast<VulkanBackend*>(glfwGetWindowUserPointer(window));
    backend->onCursorMove(xpos, ypos);
}



void VulkanBackend::onScroll(double xoffset, double yoffset) {
    gScale += yoffset * 0.1f;
    if (gScale < 0.1f) gScale = 0.1f;
}

void VulkanBackend::onMouseButton(int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            gDragging = true;
            glfwGetCursorPos(window, &gLastX, &gLastY);
            xstart = gLastX;
            ystart = gLastY;
        }
        else if (action == GLFW_RELEASE) {
            gDragging = false;

            double releaseX, releaseY;
            glfwGetCursorPos(window, &releaseX, &releaseY);

            double dx = releaseX - xstart;
            double dy = releaseY - ystart;
            double dist = sqrt(dx * dx + dy * dy);

            if (dist < CLICK_THRESHOLD && modelLoaded) {
                int picked = 0;
                gEdgeList[picked].cut = !gEdgeList[picked].cut;
            }
        }
    }

    if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (action == GLFW_PRESS) {
            gPanning = true;
            glfwGetCursorPos(window, &gLastPanX, &gLastPanY);
        }
        else if (action == GLFW_RELEASE) {
            gPanning = false;
        }
    }
}

void VulkanBackend::onCursorMove(double xpos, double ypos) {
    if (gDragging) {
        double dx = xpos - gLastX;
        double dy = ypos - gLastY;
        gLastX = xpos;
        gLastY = ypos;

        gYaw += dx * 0.5f;
        gPitch += dy * 0.5f;
    }

    if (gPanning) {
        double dx = xpos - gLastPanX;
        double dy = ypos - gLastPanY;
        gLastPanX = xpos;
        gLastPanY = ypos;

        gPanX += dx * 0.01f;
        gPanY -= dy * 0.01f;
    }
}


int VulkanBackend::pickEdge(double mouseX, double mouseY, int screenWidth, int screenHeight, glm::mat4 viewProj) {
    // Convert mouse coords to NDC
    float ndcX = (2.0f * mouseX) / screenWidth - 1.0f;
    float ndcY = 1.0f - (2.0f * mouseY) / screenHeight;

    // Ray in clip space
    glm::vec4 rayClip(ndcX, ndcY, -1.0f, 1.0f);

    // Transform to world space
    glm::vec4 rayEye = glm::inverse(viewProj) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
    glm::vec3 rayWorld = glm::normalize(glm::vec3(rayEye));

    // Test against each edge
    for (int i = 0; i < gEdgeList.size(); i++) {
        //float dist = distanceRayToSegment(cameraPos, rayWorld, gEdgeList[i].v1, gEdgeList[i].v2);
        float dist = 0.0f; // placeholder
        if (dist < 0.05f) return i; // threshold
    }
    return -1;
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


void VulkanBackend::buildImGui() {

    if (enableValidationLayers) {
        std::cout << "- buildImGui " << std::endl;
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

        }
    }



    ImGui::Render();
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

    if (!scene || !scene->HasMeshes()) {
        std::cerr << "Failed to load mesh: " << path << std::endl;
        return result;
    }

    aiMesh* mesh = scene->mMeshes[0]; // just take the first mesh


    //i wanna have one vertex buffer with all seperate vertexs and also one where they are the same vertexs
    //i might be able to do this by just changeing index buffer

    createVertexBuffer(result, mesh);
    createIndexBuffer(result, mesh);



    // --- Final mesh info ---
    
    modelLoaded = true;

    return result;
}


void VulkanBackend::createVertexBuffer(Mesh& result, aiMesh* mesh) {

    if (enableValidationLayers) {
        std::cout << "- createVertexBuffer " << std::endl;
    }


    // Collect vertices (x,y,z as floats)
    std::vector<MeshVertex> vertices;
    vertices.reserve(mesh->mNumVertices * 3);


    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        MeshVertex v{};
        v.pos = glm::vec3(
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z);
        v.normal = glm::vec3(
            mesh->mNormals[i].x,
            mesh->mNormals[i].y,
            mesh->mNormals[i].z);

        vertices.push_back(v);

        
    }


    


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



void VulkanBackend::createIndexBuffer(Mesh& result, aiMesh* mesh) {

    if (enableValidationLayers) {
        std::cout << "- createIndexBuffer " << std::endl;
    }

    // Collect indices
    std::vector<uint32_t> indices;
    indices.reserve(mesh->mNumFaces * 3);
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }



    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

    VkDeviceSize indexSize = indices.size() * sizeof(uint32_t);

    createBuffer(
        indexSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingMemory
    );


    void* data;
    vkMapMemory(device, stagingMemory, 0, indexSize, 0, &data);
    memcpy(data, indices.data(), static_cast<size_t>(indexSize));
    vkUnmapMemory(device, stagingMemory);

    createBuffer(
        indexSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        result.indexBuffer,
        result.indexMemory
    );

    copyBuffer(stagingBuffer, result.indexBuffer, indexSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);

    result.indexCount = static_cast<uint32_t>(indices.size());
}



//vulkan stuffs


void VulkanBackend::createDescriptorSets() {

    if (enableValidationLayers) {
        std::cout << "- createDescriptorSets " << std::endl;
    }


    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;
        descriptorWrite.pImageInfo = nullptr; // Optional
        descriptorWrite.pTexelBufferView = nullptr; // Optional

        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);

    }
}


void VulkanBackend::createDescriptorPool() {

    if (enableValidationLayers) {
        std::cout << "- createDescriptorPool " << std::endl;
    }


    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;

    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);



    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void VulkanBackend::updateUniformBuffer(uint32_t currentImage) {
    UniformBufferObject ubo{};

    // --- MODEL TRANSFORM ---
    glm::mat4 model = glm::mat4(1.0f);

    // Apply user-controlled rotation
    model = glm::rotate(model, glm::radians((float)gPitch), glm::vec3(1, 0, 0));
    model = glm::rotate(model, glm::radians((float)gYaw), glm::vec3(0, 1, 0));

    // Apply user-controlled zoom (scale)
    model = glm::scale(model, glm::vec3(gScale));

    // Apply user-controlled panning
    model = glm::translate(model, glm::vec3(gPanX, gPanY, 0.0f));

    ubo.model = model;

    // --- VIEW MATRIX ---
    ubo.view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 3.0f),   // camera position
        glm::vec3(0.0f, 0.0f, 0.0f),   // look at origin
        glm::vec3(0.0f, 1.0f, 0.0f)    // up
    );

    // --- PROJECTION MATRIX ---
    ubo.proj = glm::perspective(
        glm::radians(45.0f),
        swapChainExtent.width / (float)swapChainExtent.height,
        0.1f,
        100.0f
    );
    ubo.proj[1][1] *= -1; // Vulkan Y flip

    // Copy to GPU
    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
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


    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}



void VulkanBackend::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {

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



void VulkanBackend::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
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
        std::cout << "- recordCommandBuffer " << std::endl;
    }


    VkCommandBufferBeginInfo beginInfo = vkinit::command_buffer_begin_info();




    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }




    // Build ImGUI
	buildImGui();

    if (enableValidationLayers) {
        std::cout << "- after buildImGui" << std::endl;
    }

    VkClearValue clearColor = { {{0.1f, 0.2f, 0.3f, 1.0f}} };

    VkRenderPassBeginInfo renderPassInfo = vkinit::renderpass_begin_info(renderPass, swapChainExtent, swapChainFramebuffers[imageIndex], &clearColor);



    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);




    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    VkBuffer vertexBuffers[] = { gMesh.vertexBuffer };
    VkDeviceSize offsets[] = { 0 };


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
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &gMesh.vertexBuffer, offsets);

        vkCmdBindIndexBuffer(commandBuffer, gMesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

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

        vkCmdDrawIndexed(commandBuffer, gMesh.indexCount, 1, 0, 0, 0);
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


std::vector<char> VulkanBackend::readFile(const std::string& filename) {



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


    auto vertShaderCode = readFile(vertexShaderPath);
    auto fragShaderCode = readFile(fragmentShaderPath);



    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);


    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;


    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };


    std::vector<VkDynamicState> dynamicStates = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();


    auto bindingDescription = MeshVertex::getBindingDescription();
    auto attributeDescriptions = MeshVertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();




    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; //VK_PRIMITIVE_TOPOLOGY_POINT_LIST, VK_PRIMITIVE_TOPOLOGY_LINE_LIST, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
    inputAssembly.primitiveRestartEnable = VK_FALSE;

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







    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;


    //rasterizer


    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    //VK_POLYGON_MODE_FILL: fill the area of the polygon with fragments
    //VK_POLYGON_MODE_LINE : polygon edges are drawn as lines 
    //VK_POLYGON_MODE_POINT : polygon vertices are drawn as points
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional


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


    

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;


    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }


    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional


    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }







    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);

}

VkShaderModule VulkanBackend::createShaderModule(const std::vector<char>& code) {

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



void VulkanBackend:: createSurface() {
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

		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
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
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
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


VkResult VulkanBackend::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void VulkanBackend::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}


void VulkanBackend::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {

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

void VulkanBackend:: createLogicalDevice() {

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




    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo = vkinit::device_create_info(queueCreateInfos, deviceFeatures, deviceExtensions, validationLayers, enableValidationLayers);



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

VkSurfaceFormatKHR VulkanBackend::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {

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
VkPresentModeKHR VulkanBackend::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {

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


VkExtent2D VulkanBackend::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {

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
