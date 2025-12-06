cmake_minimum_required(VERSION 3.15)

project(PaperCraftApp)

set(CMAKE_CXX_STANDARD 17)

# --- Dependencies via vcpkg ---
# These will be resolved automatically if you configured VS with vcpkg toolchain
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glad CONFIG REQUIRED)

find_package(imgui CONFIG REQUIRED)



# --- FBX SDK in external folder ---
# Example: if you dropped Autodesk FBX SDK headers/libs into external/fbx
add_library(fbx STATIC IMPORTED)
set_target_properties(fbx PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/external/FBX/lib/x64/debug/libfbxsdk.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/external/FBX/include"
)




#target_include_directories(fbx PUBLIC "external/FBX/include")

# --- Your executable ---
add_executable(PaperCraftApp
    PaperCraft.cpp
)

# --- Link everything together ---
target_link_libraries(PaperCraftApp
    PRIVATE
        glfw
        OpenGL::GL
        glad::glad          # only if using glad
        fbx
        imgui::imgui
)
