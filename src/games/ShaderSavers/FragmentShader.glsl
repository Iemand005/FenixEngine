#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

float hash(float n) {
    return fract(sin(n) * 43758.5453123);
}

vec3 hash3(float n)
{
    return fract(vec3(
        sin(n * 12.989),
        sin(n * 78.233),
        sin(n * 37.719)
    ) * 43758.5453);
}

float lineDist(vec2 p, vec2 a, vec2 b)
{
    vec2 pa = p - a;
    vec2 ba = b - a;

    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

void main()
{
    vec2 frag = gl_FragCoord.xy;
    vec2 uv = (frag - 0.5 * resolution.xy) / resolution.y;

    vec3 col = vec3(0.0);

    float t = time * 3.0; // 🔥 faster global motion

    // camera forward speed (THIS removes the “ring” illusion)
    float zSpeed = 8.0;

    for (int i = 0; i < 180; i++)
    {
        float fi = float(i);

        vec3 star = hash3(fi * 91.7);

        // infinite depth space (no center origin!)
        float z = fract(star.z + t * zSpeed);
        z = mix(0.2, 1.0, z); // keep in front of camera

        // full screen spread (NOT radial)
        vec2 pos = (star.xy * 2.0 - 1.0) * 2.5;

        // perspective projection
        vec2 p = pos / z;

        // previous frame position for streak
        float z2 = fract(star.z + (t - 0.02) * zSpeed);
        z2 = mix(0.2, 1.0, z2);

        vec2 p2 = pos / z2;

        // convert to screen space
        vec2 a = p2;
        vec2 b = p;

        // line thickness in normalized space
        float d = lineDist(uv, a, b);

        float line = step(d, 0.002);

        // brightness increases with speed (depth illusion)
        float brightness = (1.0 - z) * 1.5;

        col += vec3(line * brightness);
    }

    FragColor = vec4(col, 1.0);
}