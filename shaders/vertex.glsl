
#version 460

// You shouldn't really touch this file

// Generates a quad without a vertex array

// Input & Output

layout(location = 0) out vec2 uv;

out gl_PerVertex {
    vec4 gl_Position;
};

// Main program

void main() {
    uv.x = (gl_VertexID & 1);
    uv.y = ((gl_VertexID >> 1) & 1);
    gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0);
}
