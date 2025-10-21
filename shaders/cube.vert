#version 450

const vec2 POSITIONS[3] = vec2[](
    vec2(-0.5, -0.5),
    vec2( 0.5, -0.5),
    vec2( 0.5,  0.5)
);

void main() {
    uint idx = uint(gl_VertexIndex) % 6u;
    vec2 p = POSITIONS[idx];
    gl_Position = vec4(p, 0.0, 1.0);
}
