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


void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);

    outBary   = inBary;
    //outColour = vec3(1, 0.5, 0.0);

    int v = gl_VertexIndex % 3;

    if (v == 0)
        outColour = vec3(1.0, 0.0, 0.0);   // red
    else if (v == 1)
        outColour = vec3(0.0, 1.0, 0.0);   // green
    else
        outColour = vec3(0.0, 0.0, 1.0);   // blue

}
