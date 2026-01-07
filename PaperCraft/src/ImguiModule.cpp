#include "ImguiModule.h"
#include "VulkanBackend.h"
#include "ModelLoader.h"



ImguiModule::ImguiModule(VulkanBackend& vulkanbackend) : vulkanbackend(vulkanbackend) {}

void ImguiModule::createImGuiDescriptorPool() {
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

    if (vkCreateDescriptorPool(vulkanbackend.device, &pool_info, nullptr, &imguiPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create ImGui descriptor pool");
    }
}

void ImguiModule::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForVulkan(vulkanbackend.window, true);

    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance = vulkanbackend.instance;
    init_info.PhysicalDevice = vulkanbackend.physicalDevice;
    init_info.Device = vulkanbackend.device;
    init_info.QueueFamily = vulkanbackend.queueFamilyIndicesprivate.graphicsFamily.value();
    init_info.Queue = vulkanbackend.graphicsQueue;
    init_info.DescriptorPool = imguiPool;
    init_info.MinImageCount = 2;
    init_info.ImageCount = vulkanbackend.swapChainImages.size();
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    init_info.RenderPass = vulkanbackend.renderPass; // <- must be set 
    init_info.Subpass = 0; //


    ImGui_ImplVulkan_Init(&init_info);

    // Upload fonts
    VkCommandBuffer cmd = vulkanbackend.beginSingleTimeCommands();
    ImGui_ImplVulkan_CreateFontsTexture();
    vulkanbackend.endSingleTimeCommands(cmd);

    ImGui_ImplVulkan_DestroyFontsTexture();
}


void ImguiModule::buildImGui() {

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
            //gMesh = loadMesh(file);

        }
    }



    ImGui::Render();
}
