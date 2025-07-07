#version 330

in vec3 position;
in vec3 color;
in vec3 normal;
uniform mat4 mvp;	// perspective transform
uniform float lineWidth;// the used line width as the shader seems to have no access to this value
uniform float alpha;    // make this adjustable
uniform vec2 screen;	// to bring position to display size, as we need to do our calculation in pixel space, as the line width is counted in pixels
uniform vec3 light;
out vec4 vertexColor;
smooth out vec4 pos;
out float lineWidth2;
out vec2 ScreenSizeHalf;
void main() {
  float x = dot(light, normal);   // normalize done with glm
  x = (x < 0.2) ? 0.2 : x;
  vec3 colorX = color * x;
  vertexColor = vec4(colorX, alpha);
  lineWidth2 = lineWidth/2.0;   // do these computations only once (might be optimized anyway ...)
  ScreenSizeHalf = screen/2.0;

  gl_Position = mvp  * vec4(position, 1.0);
  pos = gl_Position;
}
