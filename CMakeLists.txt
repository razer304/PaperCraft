cmake_minimum_required(VERSION 3.15)



# Tell CMake to use vcpkg toolchain if available
# (adjust path if your vcpkg.cmake is elsewhere)
set(CMAKE_TOOLCHAIN_FILE "C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")



project(PaperCraftApp)





set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# --- Dependencies via vcpkg ---
# These will be resolved automatically if you configured VS with vcpkg toolchain
# --- Dependencies via vcpkg ---
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(tinyfiledialogs CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)


#find_package(Vulkan REQUIRED)



# Source files
set(SOURCES
    ${CMAKE_SOURCE_DIR}/PaperCraft/src/main.cpp
    ${CMAKE_SOURCE_DIR}/PaperCraft/src/VulkanBackend.cpp
    ${CMAKE_SOURCE_DIR}/PaperCraft/src/ModelLoader.cpp
    ${CMAKE_SOURCE_DIR}/PaperCraft/src/vk_initializers.cpp

)

set(HEADERS
    ${CMAKE_SOURCE_DIR}/PaperCraft/src/VulkanBackend.h
    ${CMAKE_SOURCE_DIR}/PaperCraft/src/ModelLoader.h
    ${CMAKE_SOURCE_DIR}/PaperCraft/src/Mesh.h
    ${CMAKE_SOURCE_DIR}/PaperCraft/src/Edge.h
    ${CMAKE_SOURCE_DIR}/PaperCraft/src/vk_initializers.h
    ${CMAKE_SOURCE_DIR}/PaperCraft/src/vk_types.h

)











add_executable(PaperCraftApp ${SOURCES} ${HEADERS})

#add_executable(PaperCraftApp ${SOURCES} ${HEADERS} "PaperCraft/src/vk_initializers.cpp" "PaperCraft/src/vk_initializers.h")


target_include_directories(PaperCraftApp PRIVATE
    ${CMAKE_SOURCE_DIR}/PaperCraft/src
)






# Try to find Vulkan via the official SDK first
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










#target_include_directories(fbx PUBLIC "external/FBX/include")



# --- Link everything together ---
target_link_libraries(PaperCraftApp
    PRIVATE
        glfw
        OpenGL::GL
        glad::glad
        imgui::imgui
        #imgui:x64-windows
        tinyfiledialogs::tinyfiledialogs
        assimp::assimp
        glm::glm
        GPUOpen::VulkanMemoryAllocator
)



# Shader compilation

# Shader directory relative to project root
set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/PaperCraft/src/shaders")

target_compile_definitions(PaperCraftApp PRIVATE
    SHADER_DIR="${SHADER_DIR}"
)


#add_dependencies(PaperCraftApp Shaders)
