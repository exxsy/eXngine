#version 450

// Simple pass-through vertex shader for a cube
// Expects 3D positions at location 0
layout(location = 0) in vec3 inPosition;

void main() {
    gl_Position = vec4(inPosition, 1.0);
}
