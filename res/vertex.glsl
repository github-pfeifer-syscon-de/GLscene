#version 330

in vec3 position;
in vec3 color;
uniform mat4 mvp;	// perspective transform
uniform float lineWidth;// the used line width as the shader seems to have no access to this value
uniform vec2 screen;	// to bring position to display size, as we need to do our calculation in pixel space, as the line width is counted in pixels
// 
out vec3 vertexColor;
smooth out vec4 pos;
out float lineWidth2;
out vec2 ScreenSizeHalf;
void main() {
  vertexColor = color;
  lineWidth2 = lineWidth/2.0;   // do these computations only once (might be optimized anyway ...)
  ScreenSizeHalf = screen/2.0;

  gl_Position = mvp  * vec4(position, 1.0);
  pos = gl_Position;
}