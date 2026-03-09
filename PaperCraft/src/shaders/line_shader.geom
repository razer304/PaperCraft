#version 450



layout(lines) in;
layout(line_strip, max_vertices = 2) out;

// selector buffer: one int per line segment (primitive)
layout(std430, binding = 1) buffer SelectorBuffer {
    uint selector[];
};

layout(std430, binding = 2) buffer EdgeBuffer {
    uint edges[];
};

layout(location = 3) out vec3 outColor;

void main() {
    bool s = (selector[gl_PrimitiveIDIn] == 1u);
    bool e = (edges[gl_PrimitiveIDIn] == -1u);

    if (edges[gl_PrimitiveIDIn] < gl_PrimitiveIDIn) {
        EndPrimitive();
        return;
    }

    vec3 color = s ? vec3(1, 0, 0) : vec3(1, 1, 1);
    color = e ? vec3(0, 1, 0) : color;



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
