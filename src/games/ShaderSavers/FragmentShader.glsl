#version 330 core
out vec4 FragColor;

uniform sampler2D prevFrame;
uniform vec2 resolution;

void main() {
  vec2 res = vec2(500, 500);
  res = resolution;
  vec2 pos = gl_FragCoord.xy;

  vec2 uv = pos/ res;
  vec3 lastColor = texture(prevFrame, uv).rgb;

  if (gl_FragCoord.x > 0 && gl_FragCoord.x < 13 && gl_FragCoord.y < 1200 ) {
    FragColor = vec4(1 - lastColor, 1.0);
  } else {
    FragColor = vec4(0.89, 0.05, 0.05, 1.0);
  }
}