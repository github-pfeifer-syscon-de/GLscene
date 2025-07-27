To make this a bit more useful integrated a sound output capture,
with fft-conversion (but still this is more or less some experiment ...)

- basic build instructions with genericImg
- requires Pulseaudio
- as the output levels may vary, use preferences to adjust display to taste
- the conversion of fft-data may not be to everyones taste (linear is prefered at the moment)

This may also be used for some shader experiment
(use PlaneContext::showSmokeShader (i know the naming is questionable at best...)).

![glscene](glscene.png "glscene")

To vary the display other shaders obtained from:
https://www.shadertoy.com/
The outline is :
1. place the shader source in res
4. the name for the function needs to be main (without parameters)
     ... the rest changes may vary but use e.g. waveshader.glsl vs. source as example
2. add the file-name in glsceneapp.grsources.xml
3. in GlPlaneView::init_shaders ~ line 75 replace the name for the fragment-shader
5. see how it works
6. again in GlPlaneView::draw ~ line 215
7.   ... at  UV res(1.0f, 1.0f) change the value to a nice scale
8.   ... at  float t = (float)((double)time/3.0E6);

