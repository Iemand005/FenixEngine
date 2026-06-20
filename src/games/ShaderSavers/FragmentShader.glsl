#version 330 core

out vec4 FragColor;

uniform sampler2D prevFrame;
uniform vec2 resolution;
uniform float time;

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    vec2 uv = gl_FragCoord.xy / resolution;

    vec3 lastColor = texture(prevFrame, uv).rgb;

    float hue = uv.x + uv.y * 0.5 + time * 0.2;

    vec3 rainbow = hsv2rgb(vec3(hue, 1.0, 1.0));

    vec3 feedback = vec3(1.0) - lastColor;

    vec3 color = mix(rainbow, feedback, 0.25);

    FragColor = vec4(color, 1.0);
}