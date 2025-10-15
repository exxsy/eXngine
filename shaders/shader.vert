
#version 450

// Uniforms ////
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} uniformBufferObject;

// layout(push_constant) uniform ModelPushConstants {
//     int textureIndex;
//     int numTextures;
// } pushConstants;

// Inputs
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

// Outputs
layout(location = 0) out vec2 fragTexCoord;
// layout(location = 1) out flat uint textureIndex;
// layout(location = 2) out flat uint numTextureCount;

void main() {
    gl_Position = uniformBufferObject.proj * uniformBufferObject.view * uniformBufferObject.model * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
    // textureIndex = pushConstants.textureIndex;
    // numTextureCount = pushConstants.numTextures;
}
