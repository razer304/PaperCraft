#version 450

layout(lines) in;
layout(line_strip, max_vertices = 2) out;

// selector buffer: one int per line segment (primitive)
layout(std430, binding = 1) buffer SelectorBuffer {
    uint selector[];
};


layout(location = 3) out vec3 outColor;

void main() {
    bool s = (selector[gl_PrimitiveIDIn] == 1);

    vec3 color = s ? vec3(1, 0, 0) : vec3(1, 1, 1);

    // emit vertex 0
    gl_Position = gl_in[0].gl_Position;
    outColor = color;
    EmitVertex();

    // emit vertex 1
    gl_Position = gl_in[1].gl_Position;
    outColor = color;
    EmitVertex();

    EndPrimitive();
}
