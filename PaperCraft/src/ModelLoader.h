/*
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

#include <glad/glad.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include "tinyfiledialogs.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <filesystem>
#include <glm/gtc/type_ptr.hpp>
#include <tuple>   


#include "VulkanBackend.h"
class VulkanBackend;


class ModelLoader {
public:

    struct Mesh {
        VkBuffer vertexBuffer;
        VkDeviceMemory vertexMemory;

        VkBuffer indexBuffer;
        VkDeviceMemory indexMemory;

        uint32_t indexCount;
    };


    struct Edge {
        glm::vec3 v1;
        glm::vec3 v2;
        bool cut = false;
    };

    std::vector<Edge> gEdgeList;

    Mesh gMesh;

    bool modelLoaded = false;


    void buildEdges(const aiMesh* mesh);

    Mesh loadMesh(const char* path);

private:



};

*/