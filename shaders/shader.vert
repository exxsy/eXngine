#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragUV;

// set 0: per-frame globals
layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

// per-draw data (VkModelPushConstants)
layout(push_constant) uniform Draw {
    mat4 model;
    uint textureIndex;
} draw;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * draw.model * vec4(inPos, 1.0);
    fragColor = inColor;
    fragUV = inUV;
}
