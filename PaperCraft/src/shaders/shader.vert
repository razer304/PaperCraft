#version 450
#extension GL_KHR_vulkan_glsl : enable




layout(location = 0) in vec3 inPosition;
layout(location = 1) flat out uint vSelector;



layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;


layout(set = 0, binding = 1) buffer SelectorBuffer { 
    uint selector[]; 
    };

void main() {
    uint indexID = gl_VertexIndex; // index of this vertex in the index buffer 
    vSelector = selector[indexID]; // pass to fragment shader
    //vSelector = selector[0]; // pass to fragment shader


    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
}


