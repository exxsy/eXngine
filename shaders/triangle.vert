#version 450

const vec2 POSITIONS[3] = vec2[](
    vec2( 0.0,  0.6),  
    vec2(-0.6, -0.6), 
    vec2( 0.6, -0.6) 
);

void main() {
    gl_Position = vec4(POSITIONS[gl_VertexIndex % 3], 0.0, 1.0);
}
