#version 330

in vec3 position;
in vec2 uv;
uniform mat4 mvp;	// perspective transform
out vec2 UV;
void main() {
    UV = uv;

    gl_Position = mvp * vec4(position, 1.0);
}