#version 450

const vec2 POSITIONS[3] = vec2[](
    vec2( 0.0,  0.6),  
    vec2(-0.6, -0.6), 
    vec2( 0.6, -0.6) 
);

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;

layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPos, 1.0);
    fragColor = inColor;
}
