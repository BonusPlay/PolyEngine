#version 330 core

in vec2 vUV;

uniform sampler2D uImage;
uniform float uBrightThreshold;

out vec4 oColor;

void main()
{
    vec3 inputTex = texture(uImage, vUV).rgb;
    float luminance = dot(inputTex, vec3(0.2126, 0.7152, 0.0722));
    vec4 BrightColor = mix(vec4(0.0, 0.0, 0.0, 1.0), vec4(inputTex, 1.0), step(uBrightThreshold, luminance));
    oColor = BrightColor;
}