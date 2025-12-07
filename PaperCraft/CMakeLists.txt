cmake_minimum_required(VERSION 3.15)

project(PaperCraftApp)

set(CMAKE_CXX_STANDARD 17)

# --- Dependencies via vcpkg ---
# These will be resolved automatically if you configured VS with vcpkg toolchain
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glad CONFIG REQUIRED)

find_package(imgui CONFIG REQUIRED)

find_package(tinyfiledialogs CONFIG REQUIRED)

find_package(assimp CONFIG REQUIRED)

find_package(glm CONFIG REQUIRED)


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
        imgui::imgui
        tinyfiledialogs::tinyfiledialogs
        assimp::assimp
        glm::glm
)
