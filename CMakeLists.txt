cmake_minimum_required(VERSION 3.15)



# Tell CMake to use vcpkg toolchain if available
# (adjust path if your vcpkg.cmake is elsewhere)
set(CMAKE_TOOLCHAIN_FILE "C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")



project(PaperCraftApp)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# --- Dependencies via vcpkg ---
# ---------------------------------------------------------------------------------
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(tinyfiledialogs CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
# ---------------------------------------------------------------------------------







# Source files
# ---------------------------------------------------------------------------------
file(GLOB_RECURSE PROJECT_FILES
    "${CMAKE_SOURCE_DIR}/PaperCraft/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/PaperCraft/src/*.h"
)

add_executable(PaperCraftApp ${PROJECT_FILES})

target_include_directories(PaperCraftApp PRIVATE
    ${CMAKE_SOURCE_DIR}/PaperCraft/src
)
# ---------------------------------------------------------------------------------












# ImGui
# ---------------------------------------------------------------------------------
target_include_directories(PaperCraftApp PUBLIC
    external/imgui
    external/imgui/backends
)

set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/external/imgui")

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

add_library(imgui STATIC ${IMGUI_SOURCES})

target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(imgui PUBLIC glfw Vulkan::Vulkan)
# ---------------------------------------------------------------------------------










# Try to find Vulkan via the official SDK first, then fall back to vcpkg if not found
# ---------------------------------------------------------------------------------
find_package(Vulkan QUIET)

# Check if the LunarG Vulkan SDK is installed
if(DEFINED ENV{VULKAN_SDK})
    message(STATUS "Using LunarG Vulkan SDK at $ENV{VULKAN_SDK}")
    find_package(Vulkan REQUIRED)
    target_link_libraries(PaperCraftApp PRIVATE Vulkan::Vulkan)
else()
    message(WARNING "LunarG Vulkan SDK not found, falling back to vcpkg")
    find_package(vulkan REQUIRED)
    target_link_libraries(PaperCraftApp PRIVATE Vulkan::Vulkan)
endif()
# ---------------------------------------------------------------------------------












# --- Link everything together ---
# ---------------------------------------------------------------------------------
target_link_libraries(PaperCraftApp
    PRIVATE
        glfw
        glad::glad
        tinyfiledialogs::tinyfiledialogs
        assimp::assimp
        glm::glm
        GPUOpen::VulkanMemoryAllocator
        imgui
)
# ---------------------------------------------------------------------------------











# Shader compilation
# ---------------------------------------------------------------------------------
# Shader directory relative to project root
set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/PaperCraft/src/shaders")

target_compile_definitions(PaperCraftApp PRIVATE
    SHADER_DIR="${SHADER_DIR}"
)
# ---------------------------------------------------------------------------------


