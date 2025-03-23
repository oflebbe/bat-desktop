#version 300 es
layout(location = 0) in vec3 aPos;
out vec2 TexCoords;

void main() {
    gl_Position = vec4(aPos, 1.0);
    TexCoords = aPos.xy * 0.5 + 0.5; // Normalize coordinates to [0, 1]
}
