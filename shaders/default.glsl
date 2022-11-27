
#version 460

// Input & Output

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 out_color;

// Uniforms

uniform int draw_index;
uniform vec2 resolution;
uniform float time;
uniform vec2 mouse;
uniform sampler2D tex;

// Main program

void main() {
    out_color = texture(tex, uv);
}
