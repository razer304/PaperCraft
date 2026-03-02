#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inBary;

layout(location = 2) out vec3 outBary;
layout(location = 3) out vec3 outColour;

layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

// selector buffer: one uint per line (2 indices)
layout(set = 0, binding = 1) buffer SelectorBuffer {
    uint selectors[];
};


void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);

}
