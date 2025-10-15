#version 450
#extension GL_EXT_nonuniform_qualifier : enable
const int MAX_TEXTURES = 16;

layout(push_constant) uniform ModelPushConstants {
    int textureIndex;
    int numTextures;
} pushConstants;

layout(binding = 1) uniform sampler2D textures[MAX_TEXTURES];
layout(location = 0) in vec2 fragTexCoord;
// layout(location = 1) in flat uint textureIndex;
// layout(location = 2) in flat uint numTextureCount;
layout(location = 0) out vec4 outColor;

void main() {
    // outColor = texture(textures[textureIndex], vec2(fragTexCoord.x, 1.0 - fragTexCoord.y));
    vec4 color = vec4(0.0);

    for (int layer = 0; layer < pushConstants.numTextures; ++layer) {
        vec4 layerColor = texture(textures[nonuniformEXT(layer)], vec2(fragTexCoord.x, 1.0 - fragTexCoord.y));
        color = mix(color, layerColor, layerColor.a);
    }

    outColor = color;
}
