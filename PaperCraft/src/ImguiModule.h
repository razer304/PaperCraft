#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "tinyfiledialogs.h"

class ModelLoader;
#include "ModelLoader.h"


class VulkanBackend;

class ImguiModule {

	public:

		ImguiModule(VulkanBackend& vulkanbackend);

		


		void createImGuiDescriptorPool();
		void initImGui();
		void buildImGui();


		VkDescriptorPool imguiPool;

	private:

		VulkanBackend& vulkanbackend;

};