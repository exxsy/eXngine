#version 450

// Simple pass-through vertex shader for a triangle
// Expects 2D positions at location 0
layout(location = 0) in vec2 inPosition;

void main() {
    gl_Position = vec4(inPosition, 0.0, 1.0);
}
