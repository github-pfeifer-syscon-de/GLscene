#version 330 core
// with a little help by https://www.opengl.org/discussion_boards/showthread.php/167483-Anti-Aliased-Lines/page3
in vec3 vertexColor;
in float lineWidth2;
in vec2 ScreenSizeHalf;
smooth in vec4 pos;     // the important part that this get interpolated
out vec4 outputColor;
void main() {

  //vec2 m = (pos.xy/pos.w + 1.0) * ScreenSizeHalf;
  // screen sampling work better with less effort
  //float dist = distance(m, gl_FragCoord.xy);    // is the distance in actual pixels
  //float a = dist/lineWidth2;    // scale it 0.0 middle ... 1.0 border
  //float b = cos(a*1.570763);    // make the lines look round, alternative woud be 1.0-a (linear fall off)
  float b =  1.0;
  outputColor = vec4(vertexColor, b);
}