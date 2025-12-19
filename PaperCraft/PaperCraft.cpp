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
#include <map>
#include <tuple>   


// for std::tie

// Camera setup
//glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
//glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
//glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

//glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
//glm::mat4 projection = glm::perspective(glm::radians(45.0f),
//    800.0f / 600.0f, // aspect ratio
//    0.1f, 100.0f);

//glm::mat4 viewProjMatrix = projection * view;


float gScale = 1.0f;
float gYaw = 0.0f;   // rotation around Y axis
float gPitch = 0.0f; // rotation around X axis
bool gDragging = false;
double gLastX, gLastY;

float gPanX = 0.0f;
float gPanY = 0.0f;
bool gPanning = false;
double gLastPanX, gLastPanY;

bool modelLoaded = false;

unsigned int shaderProgram;

//how much the mouse has moved to be considered a click or a drag
const double CLICK_THRESHOLD = 5.0;

double xstart;
double ystart;


// the shaders are at out/build/shaders/
const char* vertexshaderpath = "shaders/vertex.glsl";
const char* fragmentshaderpath = "shaders/fragment.glsl";

struct Mesh {
    unsigned int VAO, VBO, EBO;
    unsigned int indexCount;
};

struct Edge {
    glm::vec3 v1;
	glm::vec3 v2;
    bool cut = false;
};

std::vector<Edge> gEdgeList;

Mesh gMesh;

void buildEdges(const aiMesh* mesh) {
    struct EdgeKey {
        unsigned int v1, v2;
        bool operator<(const EdgeKey& other) const {
            return std::tie(v1, v2) < std::tie(other.v1, other.v2);
        }
    };

    std::map<EdgeKey, std::vector<glm::vec3>> edgeNormals;

    // compute normals per face
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        glm::vec3 v0(mesh->mVertices[face.mIndices[0]].x,
            mesh->mVertices[face.mIndices[0]].y,
            mesh->mVertices[face.mIndices[0]].z);
        glm::vec3 v1(mesh->mVertices[face.mIndices[1]].x,
            mesh->mVertices[face.mIndices[1]].y,
            mesh->mVertices[face.mIndices[1]].z);
        glm::vec3 v2(mesh->mVertices[face.mIndices[2]].x,
            mesh->mVertices[face.mIndices[2]].y,
            mesh->mVertices[face.mIndices[2]].z);

        glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

        for (int e = 0; e < 3; e++) {
            unsigned int a = face.mIndices[e];
            unsigned int b = face.mIndices[(e + 1) % 3];
            EdgeKey key{ std::min(a,b), std::max(a,b) };
            edgeNormals[key].push_back(normal);
        }
    }

    gEdgeList.clear();

    // collect crease edges
    for (auto& [key, normals] : edgeNormals) {
        bool isFeature = false;
        if (normals.size() == 1) isFeature = true; // boundary
        else {
            float dotp = glm::dot(normals[0], normals[1]);
            if (dotp < 0.999f) isFeature = true; // not 180°
        }
        if (isFeature) {
            glm::vec3 v1(mesh->mVertices[key.v1].x,
                mesh->mVertices[key.v1].y,
                mesh->mVertices[key.v1].z);
            glm::vec3 v2(mesh->mVertices[key.v2].x,
                mesh->mVertices[key.v2].y,
                mesh->mVertices[key.v2].z);
            gEdgeList.push_back({ v1, v2, false }); // default cut=false
        }
    }
}

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

    buildEdges(mesh);


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
    modelLoaded = true;
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

            //std::cout << "mouse distance: " << dist << std::endl;

            if (dist < CLICK_THRESHOLD && modelLoaded) {
                int picked = 0; 

                std::cout << "gedgelist cut: " << picked << std::endl;
                

                gEdgeList[picked].cut = !gEdgeList[picked].cut;
                // int picked = pickEdge(gLastX, gLastY, 800, 600, viewProjMatrix);
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

int pickEdge(double mouseX, double mouseY, int screenWidth, int screenHeight, glm::mat4 viewProj) {
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

// Compute distance from point p to segment [a,b]
float distanceRayToSegment(const glm::vec3& rayOrigin,
    const glm::vec3& rayDir,
    const glm::vec3& a,
    const glm::vec3& b)
{
    glm::vec3 u = rayDir;
    glm::vec3 v = b - a;
    glm::vec3 w0 = rayOrigin - a;

    float aDot = glm::dot(u, u);
    float bDot = glm::dot(u, v);
    float cDot = glm::dot(v, v);
    float dDot = glm::dot(u, w0);
    float eDot = glm::dot(v, w0);

    float denom = aDot * cDot - bDot * bDot;
    float sc, tc;

    if (denom < 1e-6f) {
        sc = 0.0f;
        tc = (bDot > cDot ? dDot / bDot : eDot / cDot);
    }
    else {
        sc = (bDot * eDot - cDot * dDot) / denom;
        tc = (aDot * eDot - bDot * dDot) / denom;
    }

    tc = glm::clamp(tc, 0.0f, 1.0f); // clamp to segment

    glm::vec3 pointOnRay = rayOrigin + sc * u;
    glm::vec3 pointOnSeg = a + tc * v;

    return glm::length(pointOnRay - pointOnSeg);
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
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        //render object
        //polygon

        glUseProgram(shaderProgram);

        int colorLoc = glGetUniformLocation(shaderProgram, "uColor");
        

        //transforms
        glUseProgram(shaderProgram);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(gPanX, gPanY, 0.0f));
        model = glm::scale(model, glm::vec3(gScale));
        model = glm::rotate(model, glm::radians(gYaw), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(gPitch), glm::vec3(1.0f, 0.0f, 0.0f));


        

        int modelLoc = glGetUniformLocation(shaderProgram, "uModel");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);


        // draw filled mesh (orange)
        glUniform3f(colorLoc, 1.0f, 0.5f, 0.0f); // orange
        glBindVertexArray(gMesh.VAO);
        glDrawElements(GL_TRIANGLES, gMesh.indexCount, GL_UNSIGNED_INT, 0);

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        // draw edges (black)
        glLineWidth(5.0f);
        for (int i = 0; i < gEdgeList.size(); i++) {
            // Pick color based on cut flag
            glm::vec3 color = gEdgeList[i].cut ? glm::vec3(1.0f, 0.0f, 0.0f)   // red if cut
                : glm::vec3(0.0f, 0.0f, 0.0f);  // black otherwise
            glUniform3fv(colorLoc, 1, glm::value_ptr(color));

            // Two vertices for this edge
            glm::vec3 verts[2] = { gEdgeList[i].v1, gEdgeList[i].v2 };

            // Upload directly to GPU each frame
            GLuint tempVBO, tempVAO;
            glGenVertexArrays(1, &tempVAO);
            glGenBuffers(1, &tempVBO);

            glBindVertexArray(tempVAO);
            glBindBuffer(GL_ARRAY_BUFFER, tempVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
            glEnableVertexAttribArray(0);

            // Draw the line
            glDrawArrays(GL_LINES, 0, 2);

            // Cleanup temporary objects
            glDeleteBuffers(1, &tempVBO);
            glDeleteVertexArrays(1, &tempVAO);
        }


        glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);

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
