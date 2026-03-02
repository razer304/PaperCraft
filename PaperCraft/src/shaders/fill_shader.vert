#version 450

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


void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);

    outBary   = inBary;
    outColour = vec3(1, 0.5, 0.0);
}
