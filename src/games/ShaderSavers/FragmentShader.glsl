#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

float hash(float n) {
    return fract(sin(n) * 43758.5453123);
}

vec2 hash2(float n) {
    return vec2(hash(n), hash(n + 17.0)) * 2.0 - 1.0;
}

void main()
{
    vec2 uv = gl_FragCoord.xy / resolution.xy;
    vec2 center = vec2(0.5);

    vec3 col = vec3(0.0);
    float t = time * 0.6;

    for (int i = 0; i < 180; i++)
    {
        float fi = float(i);

        vec2 dir = normalize(hash2(fi * 12.989));

        float seed = hash(fi * 78.233);

        float speed = 0.3 + hash(fi * 91.7) * 1.5;

        float p = fract(seed + t * speed);

        // acceleration outward
        float dist = p * p * 1.8;

        vec2 pos = center + dir * dist * 0.8;

        // convert to screen pixels
        vec2 pixelPos = pos * resolution;
        vec2 fragPos  = gl_FragCoord.xy;

        // distance in pixel space
        vec2 d = abs(fragPos - pixelPos);

        // SINGLE PIXEL HIT TEST (important part)
        float star =
            1.0 - step(0.5, max(d.x, d.y));

        float brightness = 0.7 + hash(fi * 9.1) * 0.6;

        col += vec3(star * brightness);
    }

    FragColor = vec4(col, 1.0);
}