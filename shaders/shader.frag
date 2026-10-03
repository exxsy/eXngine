#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

// set 1: material textures (Renderer::MAX_TEXTURE_COUNT slots)
layout(set = 1, binding = 0) uniform sampler2D textures[16];

// per-draw data (VkModelPushConstants)
layout(push_constant) uniform Draw {
    mat4 model;
    uint textureIndex;
} draw;

void main() {
    outColor = texture(textures[draw.textureIndex], fragUV) * vec4(fragColor, 1.0);
}
