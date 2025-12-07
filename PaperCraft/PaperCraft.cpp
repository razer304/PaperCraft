// PaperCraft.cpp : Defines the entry point for the application.
//

#include "PaperCraft.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector> 
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "tinyfiledialogs.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>



float gScale = 1.0f;
float gYaw = 0.0f;   // rotation around Y axis
float gPitch = 0.0f; // rotation around X axis
bool gDragging = false;
double gLastX, gLastY;

float gPanX = 0.0f;
float gPanY = 0.0f;
bool gPanning = false;
double gLastPanX, gLastPanY;



unsigned int shaderProgram;


// the shaders are at out/build/shaders/
const char* vertexshaderpath = "shaders/vertex.glsl";
const char* fragmentshaderpath = "shaders/fragment.glsl";



struct Mesh {
    unsigned int VAO, VBO, EBO;
    unsigned int indexCount;
};

Mesh gMesh;

Mesh loadMesh(const char* path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);

    if (!scene || !scene->HasMeshes()) {
        std::cerr << "Failed to load mesh: " << path << std::endl;
        return {};
    }

    aiMesh* mesh = scene->mMeshes[0]; // just take the first mesh

    // Collect vertices
    std::vector<float> vertices;
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        aiVector3D v = mesh->mVertices[i];
        vertices.push_back(v.x);
        vertices.push_back(v.y);
        vertices.push_back(v.z);
    }

    // Collect indices
    std::vector<unsigned int> indices;
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    Mesh result;
    glGenVertexArrays(1, &result.VAO);
    glGenBuffers(1, &result.VBO);
    glGenBuffers(1, &result.EBO);

    glBindVertexArray(result.VAO);

    // Vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, result.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, result.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Vertex attribute (position only)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    result.indexCount = indices.size();
    return result;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    // yoffset > 0 means scroll up, < 0 means scroll down
    gScale += yoffset * 0.1f; // adjust sensitivity
    if (gScale < 0.1f) gScale = 0.1f; // prevent flipping or disappearing
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            gDragging = true;
            glfwGetCursorPos(window, &gLastX, &gLastY);
        }
        else if (action == GLFW_RELEASE) {
            gDragging = false;
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

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
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

        gPanX += dx * 0.01f; // adjust sensitivity
        gPanY -= dy * 0.01f; // minus so dragging up moves object up
    }
}



unsigned int compileShader(unsigned int type, const std::string& source) {
    unsigned int shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation error:\n" << infoLog << std::endl;
    }
    return shader;
}

std::string loadShaderSource(const char* filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << filepath << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

unsigned int createShaderProgram() {

    std::string vertexCode = loadShaderSource(vertexshaderpath);
    std::string fragmentCode = loadShaderSource(fragmentshaderpath);


    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexCode);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentCode);

	if (vertexShader == 0 || fragmentShader == 0) {
        std::cerr << "Shader compilation failed. Cannot create shader program." << std::endl;
        return 0;
    }


    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Shader linking error:\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}


// -------------------- CALLBACKS --------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// -------------------- INITIALIZATION --------------------
bool initGLFW() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    return true;
}

GLFWwindow* createWindow(int width, int height, const char* title) {
    GLFWwindow* window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);
    return window;
}

bool initGLAD() {
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return false;
    }
    return true;
}

bool initImGui(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
	return true;
}


// -------------------- RENDERING --------------------
void renderLoop(GLFWwindow* window) {
    while (!glfwWindowShouldClose(window)) {

		glfwPollEvents();

		//start the ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
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

        // Clear screen
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        //render object
        //polygon

        glUseProgram(shaderProgram);

        // Set color to black
        int colorLoc = glGetUniformLocation(shaderProgram, "uColor");
        glUniform3f(colorLoc, 0.0f, 0.0f, 0.0f);

        //transforms
        glUseProgram(shaderProgram);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(gPanX, gPanY, 0.0f));
        model = glm::scale(model, glm::vec3(gScale));
        model = glm::rotate(model, glm::radians(gYaw), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(gPitch), glm::vec3(1.0f, 0.0f, 0.0f));

        int modelLoc = glGetUniformLocation(shaderProgram, "uModel");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        if (gMesh.VAO) {
            glBindVertexArray(gMesh.VAO);
            glDrawElements(GL_TRIANGLES, gMesh.indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }


        // Reset polygon mode for ImGui (so UI isn’t wireframe)

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

// -------------------- CLEANUP --------------------
void cleanup() {
    glfwTerminate();
}

// -------------------- MAIN --------------------
int main() {


    if (!initGLFW()) return -1;

    std::cout << "Current path: " << std::filesystem::current_path() << std::endl;


    GLFWwindow* window = createWindow(800, 600, "OpenGL Window");
    if (!window) return -1;

    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    if (!initGLAD()) {
        std::cerr << "GLAD failed to initialize\n";
        return -1;
    }


    shaderProgram = createShaderProgram();

    // Set viewport and callback
    glViewport(0, 0, 800, 600);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	//init ImGui
    if (!initImGui(window)) return -1;

    // Run main loop
    renderLoop(window);

    // Cleanup
    cleanup();

    return 0;
}
