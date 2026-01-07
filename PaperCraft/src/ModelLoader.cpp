
/*
#include "ModelLoader.h"




void ModelLoader::buildEdges(const aiMesh* mesh) {
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



ModelLoader::Mesh ModelLoader::loadMesh(const char* path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_SortByPType);

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



    VkDeviceSize vertexSize = vertices.size() * sizeof(float);

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

    VulkanBackend::createBuffer(vertexSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingMemory);

    void* data;
    vkMapMemory(device, stagingMemory, 0, vertexSize, 0, &data);
    memcpy(data, vertices.data(), vertexSize);
    vkUnmapMemory(device, stagingMemory);

    createBuffer(vertexSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        result.vertexBuffer, result.vertexMemory);

    copyBuffer(stagingBuffer, result.vertexBuffer, vertexSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);


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

*/