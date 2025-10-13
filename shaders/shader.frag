#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(binding = 1) uniform sampler2D textures[];
layout(location = 0) in vec2 fragTexCoord;
layout(location = 2) in flat uint textureIndex;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(textures[textureIndex], vec2(fragTexCoord.x, 1.0 - fragTexCoord.y));
}
