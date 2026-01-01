cmake_minimum_required(VERSION 3.15)







project(PaperCraftApp)





set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# --- Dependencies via vcpkg ---
# These will be resolved automatically if you configured VS with vcpkg toolchain
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glad CONFIG REQUIRED)

find_package(imgui CONFIG REQUIRED)

find_package(tinyfiledialogs CONFIG REQUIRED)

find_package(assimp CONFIG REQUIRED)

find_package(glm CONFIG REQUIRED)

#find_package(Vulkan REQUIRED)



# Source files
set(SOURCES
    ${CMAKE_SOURCE_DIR}/PaperCraft/src/main.cpp
    ${CMAKE_SOURCE_DIR}/PaperCraft/src/VulkanBackend.cpp
    ${CMAKE_SOURCE_DIR}/PaperCraft/src/ModelLoader.cpp
)

set(HEADERS
    ${CMAKE_SOURCE_DIR}/PaperCraft/src/VulkanBackend.h
    ${CMAKE_SOURCE_DIR}/PaperCraft/src/ModelLoader.h
    ${CMAKE_SOURCE_DIR}/PaperCraft/src/Mesh.h
    ${CMAKE_SOURCE_DIR}/PaperCraft/src/Edge.h
)

add_executable(PaperCraftApp ${SOURCES} ${HEADERS}  )







# Tell CMake to use vcpkg toolchain if available
# (adjust path if your vcpkg.cmake is elsewhere)
set(CMAKE_TOOLCHAIN_FILE "C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "Vcpkg toolchain file")

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
        glad::glad          # only if using glad
        imgui::imgui
        tinyfiledialogs::tinyfiledialogs
        assimp::assimp
        glm::glm
#        Vulkan::Vulkan
)

# Shader compilation
set(SHADER_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADER_OUT ${CMAKE_BINARY_DIR}/shaders)

file(GLOB SHADERS "${SHADER_DIR}/*.vert" "${SHADER_DIR}/*.frag")

foreach(SHADER ${SHADERS})
    get_filename_component(FILE_NAME ${SHADER} NAME)
    add_custom_command(
        OUTPUT ${SHADER_OUT}/${FILE_NAME}.spv
        COMMAND glslc ${SHADER} -o ${SHADER_OUT}/${FILE_NAME}.spv
        DEPENDS ${SHADER}
    )
    list(APPEND SPV_SHADERS ${SHADER_OUT}/${FILE_NAME}.spv)
endforeach()

add_custom_target(Shaders ALL DEPENDS ${SPV_SHADERS})
add_dependencies(PaperCraftApp Shaders)
