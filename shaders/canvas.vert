#version 450

// VkCanvas: 2D vertices with RGBA8 colors, placed by the batch's transform.

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;

// per-batch data (VkCanvasPushConstants)
layout(push_constant) uniform Canvas {
    mat4 transform;
    vec4 params[2];
    uint textureIndex;
} canvas;

// Vertex colors are sRGB (as picked in an image editor); the swap chain expects
// linear values and converts them back when the image is shown.
vec3 toLinear(vec3 c) {
    return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(vec3(0.04045), c));
}

void main() {
    gl_Position = canvas.transform * vec4(inPosition, 0.0, 1.0);
    fragUV = inUV;
    fragColor = vec4(toLinear(inColor.rgb), inColor.a);
}
