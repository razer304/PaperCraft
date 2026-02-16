#version 450

layout(location = 0) out vec4 outColor;
layout(location = 2) in vec3 inBary;

// Match your descriptor set/binding
layout(set = 0, binding = 1) readonly buffer SelectorBuffer {
    uint selector[];
} selectors;

void main()
{
    vec4 baseColor = vec4(1.0, 0.5, 0.2, 1.0);
    outColor = baseColor;

    float thickness = 0.02;

    bool near01 = inBary.z < thickness;
    bool near12 = inBary.x < thickness;
    bool near20 = inBary.y < thickness;

    // Each triangle has 3 vertices, and your selector buffer is per-index
    uint prim = gl_PrimitiveID;
    uint i0 = prim * 3u + 0u;
    uint i1 = prim * 3u + 1u;
    uint i2 = prim * 3u + 2u;

    uint s0 = selectors.selector[i0];
    uint s1 = selectors.selector[i1];
    uint s2 = selectors.selector[i2];

    bool edge01 = near01 && (s0 == 1u) && (s1 == 1u);
    bool edge12 = near12 && (s1 == 1u) && (s2 == 1u);
    bool edge20 = near20 && (s2 == 1u) && (s0 == 1u);

    if (edge01 || edge12 || edge20)
        outColor = vec4(1, 0, 0, 1);
}
