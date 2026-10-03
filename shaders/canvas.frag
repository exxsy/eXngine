#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

// set 1: material textures (Renderer::MAX_TEXTURE_COUNT slots)
layout(set = 1, binding = 0) uniform sampler2D textures[16];

// per-batch data (VkCanvasPushConstants)
layout(push_constant) uniform Canvas {
    mat4 transform;
    vec4 params[2];
    uint textureIndex;
} canvas;

void main() {
    outColor = texture(textures[canvas.textureIndex], fragUV) * fragColor;
}
